#include "adcread.h"

//ф-ции пока лежат в Source.cpp
extern int InitE14(void);
extern ULONG GetVal(void);
typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);

extern LUnknown* pIUnknown;
extern IDaqLDevice* pI;
extern HRESULT hr;
extern HANDLE hVxd;

extern void     *fdata, *fdata1;
extern HANDLE   hFile, hMap, hFile1, hMap1;
extern long     fsize;

extern void    *data_rbuf;
extern ULONG   *sync;

extern LONG   complete;
LONG   stop;

extern HANDLE  hThread;
extern ULONG   Tid;

extern USHORT IrqStep;//777-777%7; // половинка буфера кратная числу каналов или не обязательно кратная
extern USHORT FIFO;         //
extern USHORT pages;  // количество страниц IrqStep в кольцевом буфере PC
extern USHORT multi;    // - количество половинок кольцевого буфера, которое мы хотим собрать и записать на диск
extern ULONG  pointsize;     // pI->GetParameter(L_POINT_SIZE, &ps) возвращает размер отсчета в байтах (обычно 2, но 791 4 байта)

using namespace std;

ULONG   status;

ADCRead::ADCRead()
{
    IsStarted = 0;
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

    ULONG i=0, val;
    while (1)
    {
        while (fl2 == fl1)
        {
            if (InterlockedCompareExchange(&complete, 3, 3))
                return 0;
            InterlockedExchange(&myADC->CureS, *sync);
            fl2 = (myADC->CureS <= halfbuffer) ? 0 : 1; // Ждем заполнения половинки буфера
            //
            tmp1 = ((char*)data_rbuf + (myADC->CureS)*pointsize);
            val = ((UINT16*)tmp1)[0];
            InterlockedExchange(&myADC->CureVal, val);
            //
            if (InterlockedCompareExchange(&stop, 1, 1)){
                //
                tmp1 = ((char*)data_rbuf + myADC->CureS*pointsize);
                WriteFile(hFile, tmp1, halfbuffer*pointsize, &BytesWritten, NULL);
                return 0;
            }
        }
        //tmp = ((char *)fdata + (halfbuffer*i)*pointsize);                     // Настраиваем указатель в файле
        tmp1 = ((char*)data_rbuf + (halfbuffer*fl1)*pointsize);                   // Настраиваем указатель в кольцевом буфере
        //myADC->CureVal = ((UINT16*)tmp1)[0];
        myADC->CureBIdxFull += ((2*halfbuffer*fl1)*pointsize);
        //memcpy(tmp, tmp1, halfbuffer*pointsize);   // Записываем данные в файл
        WriteFile(hFile, tmp1, halfbuffer*pointsize, &BytesWritten, NULL);
        if (BytesWritten < halfbuffer*pointsize)
            return 1;
        InterlockedExchange(&myADC->CureS, *sync);
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
        return 2;
    }
    hr = pIUnknown->QueryInterface(IID_ILDEV, (void**)&pI);
    status = pIUnknown->Release();
    hVxd = pI->OpenLDevice(); // открываем устройство
    status = pI->GetSlotParam(&sl);

    char *bn = new char[5];
    status = pI->LoadBios(bn);
    status = pI->PlataTest(); // тестируем успешность загрузки и работоспособность биос
    status = pI->ReadPlataDescr(&pd); // считываем данные о конфигурации платы/модуля.
                                      // ОБЯЗАТЕЛЬНО ДЕЛАТЬ! (иначе расчеты параметров сбора данных невозможны тк нужна информация о названии модуля и частоте кварца )
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
        adcPar.t1.dRate = 100.0;
        adcPar.t1.dKadr = 0;
        adcPar.t1.dScale = 0;
        adcPar.t1.SynchroType = 3; //3
        if (sl.BoardType == E440 || sl.BoardType == E140 || sl.BoardType == E154) adcPar.t1.SynchroType = 0;//0
        adcPar.t1.SynchroSensitivity = 0;
        adcPar.t1.SynchroMode = 0;
        adcPar.t1.AdChannel = 0;
        adcPar.t1.AdPorog = 0;
        adcPar.t1.NCh = 1;
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

    status = pI->InitStartLDevice(); // Инициализируем внутренние переменные драйвера
    if (status != L_SUCCESS) { M_FAIL("InitStartLDevice(ADC)", status); End(); return 11; }
    else M_OK("InitStartLDevice(ADC)", endl);

    hThread = CreateThread(0, 0x2000, ServiceThread, this, 0, &Tid); // Создаем и запускаем поток сбора данных
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

int ADCRead::StartGetData()
{
    CureArrIdx = 0;
    IsStarted = 1;

    if (hFile) CloseHandle(hFile);

    hFile = CreateFileA("data.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) { M_FAIL("CreateFile(data.dat)", GetLastError()); End(); return 11; }
    else M_OK("CreateFile(data.dat)", endl);

    if (!pI){
        return 2;
    }
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
    if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, 1000)) {
        complete = 1;
        //break;
    }
    if (!complete)
    {
        cout << endl << ".......... Wait for thread completition..." << endl;
        InterlockedBitTestAndSet(&complete, 0); //complete=1
        WaitForSingleObject(hThread, INFINITE);
    }
    if (!pI){
        return 2;
    }
    status = pI->CloseLDevice();
    if (status != L_SUCCESS) { M_FAIL("CloseLDevice", status); /*goto end;*/ }
    else M_OK("CloseLDevice", endl);

    status = pI->Release();
    M_OK("Release IDaqLDevice", endl);
    cout << ".......... Ref: " << status << endl;

    InterlockedBitTestAndSet(&complete, 0); //complete=1
    InterlockedBitTestAndSet(&complete, 1); //complete=3
    WaitForSingleObject(hThread, 2000);
    if (hThread) CloseHandle(hThread);

    if (fdata) UnmapViewOfFile(fdata);
    if (hMap) CloseHandle(hMap);
    if (hFile) CloseHandle(hFile);

    if (fdata1) UnmapViewOfFile(fdata1);
    if (hMap1) CloseHandle(hMap1);
    if (hFile1) CloseHandle(hFile1);

    cout << ".......... Exit." << endl;
    return 0;
}

int ADCRead::StopGetData()
{
    IsStarted = 0;

    if (!pI){
        return 2;
    }
    status = pI->StopLDevice(); // Остановили сбор
    if (status != L_SUCCESS) { M_FAIL("StopLDevice(ADC)", status); End(); return 11; }
    else M_OK("StopLDevice(ADC)", endl);

    if (hFile) CloseHandle(hFile);
    //
    //End();
    //
    return 0;
}

ULONG64 ADCRead::GetCureByteNum()
{
    InterlockedExchange(&sCureS, CureS);
    CureByteNum = CureBIdxFull + sCureS;
    return CureByteNum;
}

int ADCRead::WriteToFile2()
{
    return 0;
}
