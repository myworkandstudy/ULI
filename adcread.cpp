#define INITGUID

#include "adcread.h"

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <clocale>
#include <iostream>
#include <iomanip>
#include <objbase.h>
#include <math.h>

#include "include2\ioctl.h"
#include "include2\ifc_ldev.h"
#include "include2\e2010cmd.h"

//#include <QTextStream>
//QTextStream cout(stdout);
//QTextStream cin(stdin);

ULONG GetVal(void);
typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);

LUnknown* pIUnknown;
IDaqLDevice* pI;

void     *fdata = NULL, *fdata1 = NULL;
HANDLE   hFile = NULL, hMap = NULL, hFile1 = NULL, hMap1 = NULL;
long     fsize;

void    *data_rbuf;
ULONG   *sync;

LONG   complete;
LONG   stop;

HANDLE  hThread = NULL;
ULONG   Tid;

USHORT IrqStep=4096;    //777-777%7; // половинка буфера кратная числу каналов или не обязательно кратная
//USHORT FIFO=1024;         //!4096??
USHORT pages=32; // количество страниц IrqStep в кольцевом буфере PC
USHORT multi=4;    // - количество половинок кольцевого буфера, которое мы хотим собрать и записать на диск
ULONG  pointsize;     // pI->GetParameter(L_POINT_SIZE, &ps) возвращает размер отсчета в байтах (обычно 2, но 791 4 байта)

using namespace std;

ULONG   status;

ADCRead::ADCRead()
{
    IsStarted = 0;
    sett_dRate_kHz = 100.0;
    //Init();
}

ADCRead::~ADCRead()
{
    End();
}

CREATEFUNCPTR CreateInstance;

HINSTANCE CallCreateInstance(char* name)
{
    HINSTANCE hComponent = ::LoadLibraryA(name);
    if (hComponent == NULL) { return 0; }

    CreateInstance = (CREATEFUNCPTR)::GetProcAddress(hComponent, "CreateInstance");
    if (CreateInstance == NULL) { return 0; }
    return hComponent;
}

// Поток в котором осуществляется сбор данных
ULONG WINAPI ServiceThread(PVOID ctx)
{
    ULONG halfbuffer = IrqStep*pages / 2;              // Собираем половинками кольцевого буфера
    //ULONG s;
    ADCRead *myADC = (ADCRead*)ctx;

    InterlockedExchange(&myADC->CureS, *sync);
    ULONG fl2, fl1 = fl2 = (myADC->CureS <= halfbuffer) ? 0 : 1;  // Настроили флаги
    void *tmp1;//*tmp
    ULONG BytesWritten;
    myADC->CureByteNum = 0;

    ULONG i=0, val, val2;
    int cidx;
    while (1)
    {
        while (fl2 == fl1)
        {
            if (InterlockedCompareExchange(&complete, 3, 3))
                return 0;
            InterlockedExchange(&myADC->CureS, *sync);
            if (myADC->CureS){
                fl2 = (myADC->CureS <= halfbuffer) ? 0 : 1; // Ждем заполнения половинки буфера
                //
                tmp1 = ((char*)data_rbuf + (myADC->CureS-1)*pointsize);
                cidx = myADC->CureS;
                if (myADC->CureS < 4)
                    cidx = (2*halfbuffer) + myADC->CureS;
                if (myADC->CureS % 4){
                    val = ((UINT16*)tmp1)[-2];
                    val2 = ((UINT16*)tmp1)[-1];
                } else {
                    val = ((UINT16*)tmp1)[-1];
                    val2 = ((UINT16*)tmp1)[-2];
                }
                if (myADC->CureS==0){
                    myADC->CureS = 0;
                }
                if (val==0){
                    val = 0;
                } else {
                    val = val + 1;
                }
                InterlockedExchange(&myADC->CureVal, val);
                InterlockedExchange(&myADC->CureVal2, val2);
                //
                if (InterlockedCompareExchange(&stop, 1, 1)){
                    //
                    tmp1 = ((char*)data_rbuf + myADC->CureS*pointsize);
                    WriteFile(hFile, tmp1, halfbuffer*pointsize, &BytesWritten, NULL);
                    return 0;
                }
            } else {
                val = val;
            }
        }
        //tmp = ((char *)fdata + (halfbuffer*i)*pointsize);                     // Настраиваем указатель в файле
        tmp1 = ((char*)data_rbuf + (halfbuffer*fl1)*pointsize);                   // Настраиваем указатель в кольцевом буфере
        //myADC->CureVal = ((UINT16*)tmp1)[0];
        ////myADC->CureBIdxFull += ((halfbuffer)*pointsize);
        InterlockedAdd64(&myADC->CureBIdxFull,(halfbuffer*fl1)*pointsize);
        //memcpy(tmp, tmp1, halfbuffer*pointsize);   // Записываем данные в файл
        WriteFile(hFile, tmp1, halfbuffer*pointsize, &BytesWritten, NULL);
        if (BytesWritten < halfbuffer*pointsize)
            return 1;
        //InterlockedExchange(&myADC->CureS, *sync);
        fl1 = (myADC->CureS <= halfbuffer) ? 0 : 1;                 // Обновляем флаг
        //
        if (InterlockedCompareExchange(&stop, 1, 1))
            break;
        //
        Sleep(0);                                     // если собираем медленно то можно и спать больше
        i++;
    }
    return 0;                                         // Вышли
}

ULONG GetVal(void)
{
    ASYNC_PAR pp;
    ULONG   status;
    //
    if (!pI) {
        return 0;
    }
    pp.s_Type = L_ASYNC_ADC_INP;
    pp.Chn[0] = 0x00; // 0 канал дифф. подключение (в общем случае лог. номер канала)
    status = pI->IoAsync(&pp);
    //cout << ".......... ADC_IN: " << hex << pp.Data[0] << (status ? " FAILED" : " SUCCESS") << "\r";
    return pp.Data[0];
}

ULONG GetVal2(void)
{
    ASYNC_PAR pp;
    ULONG   status;
    //
    if (!pI) {
        return 0;
    }
    pp.s_Type = L_ASYNC_ADC_INP;
    pp.NCh = 2;
    pp.Chn[0] = 0x01;
    status = pI->IoAsync(&pp);
    return pp.Data[0];
}

int ADCRead::Init(void)
{
    ADC_PAR adcPar;
    PLATA_DESCR_U2 pd;
    SLOT_PAR sl;
    ULONG   status;
    //int k;
try{
    setlocale(LC_CTYPE, "");

#ifdef _WIN64
    char *cc = new char[13];
    strcpy(cc, "lcomp64.dll");
    CallCreateInstance(cc);//"lcomp64.dll");
#else
    CallCreateInstance("lcomp.dll");
#endif

#define M_FAIL(x,s) do { cout << "FAILED  -> " << x << " ERROR: " << s << endl;  } while(0)
#define M_OK(x,e)   do { cout << "SUCCESS -> " << x << e; } while(0)

    pIUnknown = CreateInstance(0);
    if (!pIUnknown){
        M_FAIL("CreateInstance(0)", 123);
        return 2;
    }
    HRESULT hr = pIUnknown->QueryInterface(IID_ILDEV, (void**)&pI);
    if (!SUCCEEDED(hr)) { cout << "FAILED  -> QueryInterface" << endl; return 1; }
    cout << "SUCCESS -> QueryInterface" << endl;

    status = pIUnknown->Release();

    HANDLE hVxd = pI->OpenLDevice(); // открываем устройство
    if (hVxd == INVALID_HANDLE_VALUE) { M_FAIL("OpenLDevice", hVxd); return 11; }
    else M_OK("OpenLDevice", endl);
    cout << ".......... HANDLE: " << hex << hVxd << endl;

    status = pI->GetSlotParam(&sl);
    if (status != L_SUCCESS) { M_FAIL("GetSlotParam", status); return 11; }
    else M_OK("GetSlotParam", endl);

    cout << ".......... Type    " << sl.BoardType << endl;
    cout << ".......... DSPType " << sl.DSPType << endl;
    cout << ".......... InPipe MTS" << sl.Dma << endl;
    cout << ".......... OutPipe MTS" << sl.DmaDac << endl;

    //char *bn = new char[6];
    char bn[6] = "E440\0";// E140 ne nado;
    status = pI->LoadBios(bn);
    if ((status != L_SUCCESS) && (status != L_NOTSUPPORTED)) { M_FAIL("LoadBios", status); return 11; }
    else M_OK("LoadBios", endl);

    status = pI->PlataTest(); // тестируем успешность загрузки и работоспособность биос
    if (status != L_SUCCESS) { M_FAIL("PlataTest", status); return 11; }
    else M_OK("PlataTest", endl);

    status = pI->ReadPlataDescr(&pd); // считываем данные о конфигурации платы/модуля.
                                      // ОБЯЗАТЕЛЬНО ДЕЛАТЬ! (иначе расчеты параметров сбора данных невозможны тк нужна информация о названии модуля и частоте кварца )
    if (status != L_SUCCESS) { M_FAIL("ReadPlataDescr", status); return 11; }
    else M_OK("ReadPlataDescr", endl);

    switch (sl.BoardType)
    {
    case PCIA:
    case PCIB:
    case PCIC: {
        cout << ".......... SerNum       " << pd.t1.SerNum << endl;
        cout << ".......... BrdName      " << pd.t1.BrdName << endl;
        cout << ".......... Rev          " << pd.t1.Rev << endl;
        cout << ".......... DspType      " << pd.t1.DspType << endl;
        cout << ".......... IsDacPresent " << pd.t1.IsDacPresent << endl;
        cout << ".......... Quartz       " << dec << pd.t1.Quartz << endl;
    } break;

    case E140: {
        cout << ".......... SerNum       " << pd.t5.SerNum << endl;
        cout << ".......... BrdName      " << pd.t5.BrdName << endl;
        cout << ".......... Rev          " << pd.t5.Rev << endl;
        cout << ".......... DspType      " << pd.t5.DspType << endl;
        cout << ".......... IsDacPresent " << pd.t5.IsDacPresent << endl;
        cout << ".......... Quartz       " << dec << pd.t5.Quartz << endl;
    } break;

    case E440: {
        cout << ".......... SerNum       " << pd.t4.SerNum << endl;
        cout << ".......... BrdName      " << pd.t4.BrdName << endl;
        cout << ".......... Rev          " << pd.t4.Rev << endl;
        cout << ".......... DspType      " << pd.t4.DspType << endl;
        cout << ".......... IsDacPresent " << pd.t4.IsDacPresent << endl;
        cout << ".......... Quartz       " << dec << pd.t4.Quartz << endl;
    } break;
    case E2010B:
    case E2010: {
        cout << ".......... SerNum       " << pd.t6.SerNum << endl;
        cout << ".......... BrdName      " << pd.t6.BrdName << endl;
        cout << ".......... Rev          " << pd.t6.Rev << endl;
        cout << ".......... DspType      " << pd.t6.DspType << endl;
        cout << ".......... IsDacPresent " << pd.t6.IsDacPresent << endl;
        cout << ".......... Quartz       " << dec << pd.t6.Quartz << endl;
    } break;
    case E154: {
        cout << ".......... SerNum       " << pd.t7.SerNum << endl;
        cout << ".......... BrdName      " << pd.t7.BrdName << endl;
        cout << ".......... Rev          " << pd.t7.Rev << endl;
        cout << ".......... DspType      " << pd.t7.DspType << endl;
        cout << ".......... IsDacPresent " << pd.t7.IsDacPresent << endl;
        cout << ".......... Quartz       " << dec << pd.t7.Quartz << endl;
    } break;
    case L791: {
        cout << ".......... SerNum       " << pd.t3.SerNum << endl;
        cout << ".......... BrdName      " << pd.t3.BrdName << endl;
        cout << ".......... Rev          " << pd.t3.Rev << endl;
        cout << ".......... DspType      " << pd.t3.DspType << endl;
        cout << ".......... IsDacPresent " << pd.t3.IsDacPresent << endl;
        cout << ".......... Quartz       " << dec << pd.t3.Quartz << endl;
    }
    }

    DWORD tm = 10000000;
    status = pI->RequestBufferStream(&tm, L_STREAM_ADC);
    if (status != L_SUCCESS) { M_FAIL("RequestBufferStream(ADC)", status); End(); return 11; }
    else M_OK("RequestBufferStream(ADC)", endl);
    cout << ".......... Allocated memory size(word): " << tm << endl;

    // настроили параметрыы сбора
    switch (sl.BoardType)
    {
    case PCIA:
    case PCIB:
    case PCIC:
    case E440:
    case E140:
    case E154:
    {
        adcPar.t1.s_Type = L_ADC_PARAM;
        adcPar.t1.AutoInit = 1;//1
        adcPar.t1.dRate = sett_dRate_kHz;//100.0;
        adcPar.t1.dKadr = 0;
        adcPar.t1.dScale = 0;
        adcPar.t1.SynchroType = 3; //3
        if (sl.BoardType == E440 || sl.BoardType == E140 || sl.BoardType == E154) adcPar.t1.SynchroType = 0;//0
        adcPar.t1.SynchroSensitivity = 0;
        adcPar.t1.SynchroMode = 0;
        adcPar.t1.AdChannel = 0;
        adcPar.t1.AdPorog = 0;
        adcPar.t1.NCh = 2;
        adcPar.t1.Chn[0] = 0x0;
        adcPar.t1.Chn[1] = 0x1;
        adcPar.t1.Chn[2] = 0x2;
        adcPar.t1.Chn[3] = 0x3;
        adcPar.t1.FIFO = 1024;
        adcPar.t1.IrqStep = 1024;
        adcPar.t1.Pages = 128;
        if (sl.BoardType == E440 || sl.BoardType == E140 || sl.BoardType == E154)
        {
            adcPar.t1.FIFO = 4096;
            adcPar.t1.IrqStep = 4096;
            adcPar.t1.Pages = 32;
        }
        adcPar.t1.IrqEna = 1;
        adcPar.t1.AdcEna = 1;

        status = pI->FillDAQparameters(&adcPar.t1);
        if (status != L_SUCCESS) { M_FAIL("FillDAQparameters(ADC)", status); End(); return 11; }
        else M_OK("FillDAQparameters(ADC)", endl);

        cout << ".......... Buffer size(word):      " << tm << endl;
        cout << ".......... Pages:                  " << adcPar.t1.Pages << endl;
        cout << ".......... IrqStep:                " << adcPar.t1.IrqStep << endl;
        cout << ".......... FIFO:                   " << adcPar.t1.FIFO << endl;
        cout << ".......... Rate:                   " << adcPar.t1.dRate << endl;
        cout << ".......... Kadr:                   " << adcPar.t1.dKadr << endl << endl;

        status = pI->SetParametersStream(&adcPar.t1, &tm, (void **)&data_rbuf, (void **)&sync, L_STREAM_ADC);
        if (status != L_SUCCESS) { M_FAIL("SetParametersStream(ADC)", status); End(); return 11; }
        else M_OK("SetParametersStream(ADC)", endl);

        cout << ".......... Used buffer size(points): " << tm << endl;
        cout << ".......... Pages:                  " << adcPar.t1.Pages << endl;
        cout << ".......... IrqStep:                " << adcPar.t1.IrqStep << endl;
        cout << ".......... FIFO:                   " << adcPar.t1.FIFO << endl;
        cout << ".......... Rate:                   " << adcPar.t1.dRate << endl;
        cout << ".......... Kadr:                   " << adcPar.t1.dKadr << endl << endl;

        IrqStep = adcPar.t1.IrqStep; // обновили глобальные переменные котрые потом используются в ServiceThread
        pages = adcPar.t1.Pages;
        databufsize = tm;

    } break;

    case E2010B:
    case E2010:
    {
        adcPar.t2.s_Type = L_ADC_PARAM;
        adcPar.t2.AutoInit = 1;
        adcPar.t2.dRate = 1000.0;
        adcPar.t2.dKadr = 0.0;
        adcPar.t2.SynchroType = 0x01;//0x84;//0x01;
        adcPar.t2.SynchroSrc = 0x00;
        adcPar.t2.AdcIMask = SIG_0 | SIG_1 | GND_2 | GND_3;

        adcPar.t2.NCh = 2;
        adcPar.t2.Chn[0] = 0x0;
        adcPar.t2.Chn[1] = 0x1;
        adcPar.t2.Chn[2] = 0x2;
        adcPar.t2.Chn[3] = 0x3;
        adcPar.t2.FIFO = 32768;
        adcPar.t2.IrqStep = 32768;
        adcPar.t2.Pages = 32;
        adcPar.t2.IrqEna = 1;
        adcPar.t2.AdcEna = 1;

        // extra sync mode

        adcPar.t2.StartCnt = 0;
        adcPar.t2.StopCnt = 0;
        adcPar.t2.DM_Ena = 0;
        adcPar.t2.SynchroMode = 0;//A_SYNC_UP_EDGE | CH_0; // 0
        adcPar.t2.AdPorog = 0;

        status = pI->FillDAQparameters(&adcPar.t2);
        if (status != L_SUCCESS) { M_FAIL("FillDAQparameters(ADC)", status); End(); return 11; }
        else M_OK("FillDAQparameters(ADC)", endl);

        cout << ".......... Buffer size(word):      " << tm << endl;
        cout << ".......... Pages:                  " << adcPar.t2.Pages << endl;
        cout << ".......... IrqStep:                " << adcPar.t2.IrqStep << endl;
        cout << ".......... FIFO:                   " << adcPar.t2.FIFO << endl;
        cout << ".......... Rate:                   " << adcPar.t2.dRate << endl << endl;

        adcPar.t2.dRate = 0;

        status = pI->SetParametersStream(&adcPar.t2, &tm, (void **)&data_rbuf, (void **)&sync, L_STREAM_ADC);
        if (status != L_SUCCESS) { M_FAIL("SetParametersStream(ADC)", status); End(); return 11; }
        else M_OK("SetParametersStream(ADC)", endl);

        cout << ".......... Used buffer size(points): " << tm << endl;
        cout << ".......... Pages:                  " << adcPar.t2.Pages << endl;
        cout << ".......... IrqStep:                " << adcPar.t2.IrqStep << endl;
        cout << ".......... FIFO:                   " << adcPar.t2.FIFO << endl;
        cout << ".......... Rate:                   " << adcPar.t2.dRate << endl << endl;

        IrqStep = adcPar.t2.IrqStep; // обновили глобальные переменные котрые потом используются в ServiceThread
        pages = adcPar.t2.Pages;

    } break;

    case L791:
    {
        adcPar.t2.s_Type = L_ADC_PARAM;
        adcPar.t2.AutoInit = 1;
        adcPar.t2.dRate = 200.0;
        adcPar.t2.dKadr = 0;

        adcPar.t2.SynchroType = 0;
        adcPar.t2.SynchroSrc = 0;

        adcPar.t2.NCh = 4;
        adcPar.t2.Chn[0] = 0x0;
        adcPar.t2.Chn[1] = 0x1;
        adcPar.t2.Chn[2] = 0x2;
        adcPar.t2.Chn[3] = 0x3;
        adcPar.t2.FIFO = 128;
        adcPar.t2.IrqStep = 1024;
        adcPar.t2.Pages = 128;

        adcPar.t2.IrqEna = 1;
        adcPar.t2.AdcEna = 1;


        status = pI->FillDAQparameters(&adcPar.t2);
        if (status != L_SUCCESS) { M_FAIL("FillDAQparameters(ADC)", status); End(); return 11; }
        else M_OK("FillDAQparameters(ADC)", endl);

        cout << ".......... Buffer size(word):      " << tm << endl;
        cout << ".......... Pages:                  " << adcPar.t2.Pages << endl;
        cout << ".......... IrqStep:                " << adcPar.t2.IrqStep << endl;
        cout << ".......... FIFO:                   " << adcPar.t2.FIFO << endl;
        cout << ".......... Rate:                   " << adcPar.t2.dRate << endl << endl;

        adcPar.t2.dRate = 0;

        status = pI->SetParametersStream(&adcPar.t2, &tm, (void **)&data_rbuf, (void **)&sync, L_STREAM_ADC);
        if (status != L_SUCCESS) { M_FAIL("SetParametersStream(ADC)", status); End(); return 11; }
        else M_OK("SetParametersStream(ADC)", endl);

        cout << ".......... Used buffer size(points): " << tm << endl;
        cout << ".......... Pages:                  " << adcPar.t2.Pages << endl;
        cout << ".......... IrqStep:                " << adcPar.t2.IrqStep << endl;
        cout << ".......... FIFO:                   " << adcPar.t2.FIFO << endl;
        cout << ".......... Rate:                   " << adcPar.t2.dRate << endl << endl;

        IrqStep = adcPar.t2.IrqStep; // обновили глобальные переменные котрые потом используются в ServiceThread
        pages = adcPar.t2.Pages;

    }

    }

    pI->GetParameter(L_POINT_SIZE, &pointsize);
    if (status != L_SUCCESS) { M_FAIL("GetParameter", status); End(); return 11; }
    else M_OK("GetParameter", endl);

    cout << ".......... Starting ..." << endl;

    // Создаем файл
    multi = 8;
    fsize = multi*(pages / 2)*IrqStep; // размер файла

    //hFile = CreateFileA("data.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
    //if (hFile == INVALID_HANDLE_VALUE) { M_FAIL("CreateFile(data.dat)", GetLastError()); End(); return 11; }
    //else M_OK("CreateFile(data.dat)", endl);

    //поток 32бит*100 000Гц== 3.2Мбит/с(~400КБ/с). У ЖД скорость записи от 50МБ/с (то есть нам достаточно, без задержек)
//    hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, fsize*pointsize, NULL);
//    if (hMap == INVALID_HANDLE_VALUE) { M_FAIL("CreateFileMapping(data.dat)", GetLastError()); End(); return 11; }
//    else M_OK("CreateFileMapping(data.dat)", endl);

//    fdata = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
//    if (fdata == NULL) { M_FAIL("MapViewOfFile(data.dat)", GetLastError()); End(); return 11; }
//    else M_OK("MapViewOfFile(data.dat)", endl);

    complete = 0;
    stop = 0;

    //pI->EnableCorrection(); // можно включить коррекцию данных, если она поддерживается модулем

} catch (...){
    return 3;
}
    return 0;
}

ULONG ADCRead::GetValue0()
{
    if (IsStarted){
        return CureVal;
    } else {
        return GetVal();
    }
}

ULONG ADCRead::GetValue1()
{
    if (IsStarted){
        return CureVal2;
    } else {
        return GetVal2();
    }
}

int ADCRead::StartGetData()
{
    CureArrIdx = 0;
    IsStarted = 1;
    complete = 0;

    //if (hFile) (hFile);
    hThread = CreateThread(0, 0x2000, ServiceThread, this, 0, &Tid); // Создаем и запускаем поток сбора данных

    hFile = CreateFileA("data.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) { M_FAIL("CreateFile(data.dat)", GetLastError()); End(); return 11; }
    else M_OK("CreateFile(data.dat)", endl);

    if (!pI){
        return 2;
    }

    status = pI->InitStartLDevice(); // Инициализируем внутренние переменные драйвера
    if (status != L_SUCCESS) { M_FAIL("InitStartLDevice(ADC)", status); End(); return 11; }
    else M_OK("InitStartLDevice(ADC)", endl);

    status = pI->StartLDevice(); // Запускаем сбор в драйвере
    if (status != L_SUCCESS) { M_FAIL("StartLDevice(ADC)", status); End(); return 11; }
    else M_OK("StartLDevice(ADC)", endl);

    /*// Печатаем индикатор сбора данных

    while (!kbhit())
    {
        ULONG s;
        InterlockedExchange(&s, *sync);
        cout << ".......... " << setw(6) << s << "\r";
        if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, 0)) { complete = 1; break; }
        Sleep(20);
    }

    if (!complete)
    {
        cout << endl << ".......... Wait for thread completition..." << endl;
        InterlockedBitTestAndSet(&complete, 0); //complete=1
        WaitForSingleObject(hThread, INFINITE);
    }*/

    return 0;
}

int ADCRead::End()
{
    stop = 1;
    if (!pI){
        return 2;
    }
    status = pI->CloseLDevice();
    if (status != L_SUCCESS) { M_FAIL("CloseLDevice", status); /*goto end;*/ }
    else M_OK("CloseLDevice", endl);

    status = pI->Release();
    M_OK("Release IDaqLDevice", endl);
    cout << ".......... Ref: " << status << endl;

    if (hThread) CloseHandle(hThread);

    //if (fdata) UnmapViewOfFile(fdata);
    //if (hMap) CloseHandle(hMap);
    //if (hFile) CloseHandle(hFile);

    //if (fdata1) UnmapViewOfFile(fdata1);
    //if (hMap1) CloseHandle(hMap1);
    //if (hFile1) CloseHandle(hFile1);

    cout << ".......... Exit." << endl;
    return 0;
}

int ADCRead::StopGetData()
{
    IsStarted = 0;

    if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, 1000)) {
        complete = 1;
    }
    if (!complete)
    {
        cout << endl << ".......... Wait for thread completition..." << endl;
        InterlockedBitTestAndSet(&complete, 0); //complete=1
        InterlockedBitTestAndSet(&complete, 1); //complete=3
        WaitForSingleObject(hThread, INFINITE);
    }

    if (!pI){
        return 2;
    }
    status = pI->StopLDevice(); // Остановили сбор
    if (status != L_SUCCESS) { M_FAIL("StopLDevice(ADC)", status); End(); return 11; }
    else M_OK("StopLDevice(ADC)", endl);

    if (hFile) {
        CloseHandle(hFile);
        hFile = NULL;
    }
    //
    //End();
    //Init();
    //
    return 0;
}

ULONG64 ADCRead::GetCureByteNum()
{
    ULONG64 sCureBIdxFull;
    InterlockedExchange(&sCureS, CureS);
    InterlockedExchange(&sCureBIdxFull, CureBIdxFull);
    if (sCureS == 0)
        sCureS = 2;
    CureByteNum = (sCureBIdxFull + (sCureS - 2))*pointsize;
    return CureByteNum;
}

int ADCRead::WriteToFile2()
{
    return 0;
}
