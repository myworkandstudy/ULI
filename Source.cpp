#define INITGUID
//#undef UNICODE

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
//#include "..\include\create.h"
//#include "..\include\pcicmd.h"
//#include "..\include\791cmd.h"
#include "include2\e2010cmd.h"
//#include "..\include\e154cmd.h"

using namespace std;


void     *fdata = NULL, *fdata1 = NULL;
HANDLE   hFile = NULL, hMap = NULL, hFile1 = NULL, hMap1 = NULL;
long     fsize;

void    *data_rbuf;
ULONG   *sync;

LONG   complete;
LONG   stop;

HANDLE  hThread = NULL;
ULONG   Tid;

USHORT IrqStep = 1024;//777-777%7; // половинка буфера кратная числу каналов или не обязательно кратная
USHORT FIFO = 1024;         //
USHORT pages = 128;  // количество страниц IrqStep в кольцевом буфере PC
USHORT multi = 4;    // - количество половинок кольцевого буфера, которое мы хотим собрать и записать на диск
ULONG  pointsize;     // pI->GetParameter(L_POINT_SIZE, &ps) возвращает размер отсчета в байтах (обычно 2, но 791 4 байта)

typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);

CREATEFUNCPTR CreateInstance;

HINSTANCE CallCreateInstance(char* name)
{
    HINSTANCE hComponent = ::LoadLibraryA(name);
	if (hComponent == NULL) { return 0; }

	CreateInstance = (CREATEFUNCPTR)::GetProcAddress(hComponent, "CreateInstance");
	if (CreateInstance == NULL) { return 0; }
	return hComponent;
}

extern ULONG WINAPI ServiceThread(PVOID /*Context*/);

LUnknown* pIUnknown;
IDaqLDevice* pI;
HRESULT hr;
HANDLE hVxd;

int InitE14(void)
{
    //ADC_PAR adcPar;
    //DAC_PAR dacPar;
    PLATA_DESCR_U2 pd;
    SLOT_PAR sl;
    //ULONG slot;
    //USHORT d;
    //ULONG param;
    ULONG   status;
    //int k;

    //argv[2] = new char[10];
    //strcpy(argv[2], "e440");
    setlocale(LC_CTYPE, "");

#ifdef _WIN64
    char *cc = new char[13];
    strcpy(cc, "lcomp64.dll");
    CallCreateInstance(cc);//"lcomp64.dll");
#else
    CallCreateInstance("lcomp.dll");
#endif

//#define M_FAIL(x,s) do { cout << "FAILED  -> " << x << " ERROR: " << s << endl;  } while(0)
//#define M_OK(x,e)   do { cout << "SUCCESS -> " << x << e; } while(0)

    //cout << ".......... Get IUnknown pointer" << endl;
    pIUnknown = CreateInstance(0);//atoi(argv[1]));
    //if (pIUnknown == NULL) { cout << "FAILED  -> CreateInstance" << endl; return 1; }
   //cout << "SUCCESS -> CreateInstance" << endl;

    //cout << ".......... Get IDaqLDevice interface" << endl;
    //IDaqLDevice* pI;
    hr = pIUnknown->QueryInterface(IID_ILDEV, (void**)&pI);
    //if (!SUCCEEDED(hr)) { cout << "FAILED  -> QueryInterface" << endl; return 1; }
    //cout << "SUCCESS -> QueryInterface" << endl;

    status = pIUnknown->Release();
    //M_OK("Release IUnknown", endl);
    //cout << ".......... Ref: " << status << endl;


    hVxd = pI->OpenLDevice(); // открываем устройство
    //if (hVxd == INVALID_HANDLE_VALUE) { M_FAIL("OpenLDevice", hVxd); goto end; }
    //else M_OK("OpenLDevice", endl);
    //cout << ".......... HANDLE: " << hex << hVxd << endl;


    status = pI->GetSlotParam(&sl);
    //if (status != L_SUCCESS) { M_FAIL("GetSlotParam", status); goto end; }
    //else M_OK("GetSlotParam", endl);

    //cout << ".......... Type    " << sl.BoardType << endl;
    //cout << ".......... DSPType " << sl.DSPType << endl;
    //cout << ".......... InPipe MTS" << sl.Dma << endl;
    //cout << ".......... OutPipe MTS" << sl.DmaDac << endl;

    char *bn = new char[5];
    status = pI->LoadBios(bn);//argv[2]); // загружаем биос в модуль
    //if ((status != L_SUCCESS) && (status != L_NOTSUPPORTED)) { M_FAIL("LoadBios", status); goto end; }
    //else M_OK("LoadBios", endl);

    status = pI->PlataTest(); // тестируем успешность загрузки и работоспособность биос
    //if (status != L_SUCCESS) { M_FAIL("PlataTest", status); goto end; }
    //else M_OK("PlataTest", endl);

    status = pI->ReadPlataDescr(&pd); // считываем данные о конфигурации платы/модуля.
                                      // ОБЯЗАТЕЛЬНО ДЕЛАТЬ! (иначе расчеты параметров сбора данных невозможны тк нужна информация о названии модуля и частоте кварца )
    //if (status != L_SUCCESS) { M_FAIL("ReadPlataDescr", status); goto end; }
    //else M_OK("ReadPlataDescr", endl);
    return 0;
}

ULONG GetVal(void)
{
    ASYNC_PAR pp;
    ULONG   status;
    // тест ацп
    pp.s_Type = L_ASYNC_ADC_INP;
    pp.Chn[0] = 0x00; // 0 канал дифф. подключение (в общем случае лог. номер канала)
    status = pI->IoAsync(&pp);
    //cout << ".......... ADC_IN: " << hex << pp.Data[0] << (status ? " FAILED" : " SUCCESS") << "\r";
    return pp.Data[0];
}

int main33(int argc, char *argv[])
{
	ADC_PAR adcPar;
    //DAC_PAR dacPar;
	PLATA_DESCR_U2 pd;
    SLOT_PAR sl;
    //ULONG slot;
    //USHORT d;
    //ULONG param;
	ULONG   status;
	int k;

    //argv[2] = new char[10];
    //strcpy(argv[2], "e440");
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

    //cout << ".......... Get IUnknown pointer" << endl;
    pIUnknown = CreateInstance(0);//atoi(argv[1]));
    //if (pIUnknown == NULL) { cout << "FAILED  -> CreateInstance" << endl; return 1; }
   //cout << "SUCCESS -> CreateInstance" << endl;

    //cout << ".......... Get IDaqLDevice interface" << endl;
    //IDaqLDevice* pI;
    hr = pIUnknown->QueryInterface(IID_ILDEV, (void**)&pI);
    //if (!SUCCEEDED(hr)) { cout << "FAILED  -> QueryInterface" << endl; return 1; }
    //cout << "SUCCESS -> QueryInterface" << endl;

	status = pIUnknown->Release();
    //M_OK("Release IUnknown", endl);
    //cout << ".......... Ref: " << status << endl;


    hVxd = pI->OpenLDevice(); // открываем устройство
    //if (hVxd == INVALID_HANDLE_VALUE) { M_FAIL("OpenLDevice", hVxd); goto end; }
    //else M_OK("OpenLDevice", endl);
    //cout << ".......... HANDLE: " << hex << hVxd << endl;


	status = pI->GetSlotParam(&sl);
    //if (status != L_SUCCESS) { M_FAIL("GetSlotParam", status); goto end; }
    //else M_OK("GetSlotParam", endl);

    //cout << ".......... Type    " << sl.BoardType << endl;
    //cout << ".......... DSPType " << sl.DSPType << endl;
    //cout << ".......... InPipe MTS" << sl.Dma << endl;
    //cout << ".......... OutPipe MTS" << sl.DmaDac << endl;

    char *bn = new char[5];
    status = pI->LoadBios(bn);//argv[2]); // загружаем биос в модуль
    //if ((status != L_SUCCESS) && (status != L_NOTSUPPORTED)) { M_FAIL("LoadBios", status); goto end; }
    //else M_OK("LoadBios", endl);

	status = pI->PlataTest(); // тестируем успешность загрузки и работоспособность биос
    //if (status != L_SUCCESS) { M_FAIL("PlataTest", status); goto end; }
    //else M_OK("PlataTest", endl);

	status = pI->ReadPlataDescr(&pd); // считываем данные о конфигурации платы/модуля. 
									  // ОБЯЗАТЕЛЬНО ДЕЛАТЬ! (иначе расчеты параметров сбора данных невозможны тк нужна информация о названии модуля и частоте кварца )
    //if (status != L_SUCCESS) { M_FAIL("ReadPlataDescr", status); goto end; }
    //else M_OK("ReadPlataDescr", endl);

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

    //cout << endl << ".......... Press any key..." << dec << endl;
    //_getch();
	//goto end;


    //cout << endl << ".......... ADC Async test" << endl;
	ASYNC_PAR pp;
	ULONG step = 0;

	// тест ацп
	while (!kbhit())
	{
		pp.s_Type = L_ASYNC_ADC_INP;
		pp.Chn[0] = 0x00; // 0 канал дифф. подключение (в общем случае лог. номер канала)
		status = pI->IoAsync(&pp);
		cout << ".......... ADC_IN: " << hex << pp.Data[0] << (status ? " FAILED" : " SUCCESS") << "\r";
		//Sleep(100);
	}
	_getch();

	// тест цифровых линий
	cout << endl << ".......... TTL I/O test" << endl;
	pp.s_Type = L_ASYNC_TTL_CFG;
	pp.Mode = 1;
	status = pI->IoAsync(&pp);
	if (status != L_SUCCESS) M_FAIL("IoAsync TTL_CFG", status); else M_OK("IoAsync TTL_CFG", endl);


	while (!kbhit())
	{
		pp.s_Type = L_ASYNC_TTL_OUT;
		pp.Data[0] = 0xA525;
		status = pI->IoAsync(&pp);
		cout << ".......... TTL_OUT: " << hex << pp.Data[0] << (status ? " FAILED" : " SUCCESS");

		Sleep(100);

		pp.s_Type = L_ASYNC_TTL_INP;
		pp.Data[0] = 1;
		status = pI->IoAsync(&pp);
		cout << "   TTL_IN: " << hex << pp.Data[0] << (status ? " FAILED" : " SUCCESS") << " CNT:" << dec << step++ << "\r";
	}

	cout << endl << ".......... Press any key to start ADC stream..." << dec << endl << endl;
	_getch();
	//goto end;


	cout << ".......... Prepare ADC streaming" << endl;

	DWORD tm = 10000000;
	status = pI->RequestBufferStream(&tm, L_STREAM_ADC);
	if (status != L_SUCCESS) { M_FAIL("RequestBufferStream(ADC)", status); goto end; }
	else M_OK("RequestBufferStream(ADC)", endl);
	cout << ".......... Allocated memory size(word): " << tm << endl;

	status = pI->RequestBufferStream(&tm, L_STREAM_ADC);
	if (status != L_SUCCESS) { M_FAIL("RequestBufferStream(ADC)", status); goto end; }
	else M_OK("RequestBufferStream(ADC)", endl);
	cout << ".......... Allocated memory size(word): " << tm << endl;


	cout << endl << ".......... Press any key" << dec << endl << endl;
	_getch();

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
		if (status != L_SUCCESS) { M_FAIL("FillDAQparameters(ADC)", status); goto end; }
		else M_OK("FillDAQparameters(ADC)", endl);

		cout << ".......... Buffer size(word):      " << tm << endl;
		cout << ".......... Pages:                  " << adcPar.t1.Pages << endl;
		cout << ".......... IrqStep:                " << adcPar.t1.IrqStep << endl;
		cout << ".......... FIFO:                   " << adcPar.t1.FIFO << endl;
		cout << ".......... Rate:                   " << adcPar.t1.dRate << endl;
		cout << ".......... Kadr:                   " << adcPar.t1.dKadr << endl << endl;

		status = pI->SetParametersStream(&adcPar.t1, &tm, (void **)&data_rbuf, (void **)&sync, L_STREAM_ADC);
		if (status != L_SUCCESS) { M_FAIL("SetParametersStream(ADC)", status); goto end; }
		else M_OK("SetParametersStream(ADC)", endl);

		cout << ".......... Used buffer size(points): " << tm << endl;
		cout << ".......... Pages:                  " << adcPar.t1.Pages << endl;
		cout << ".......... IrqStep:                " << adcPar.t1.IrqStep << endl;
		cout << ".......... FIFO:                   " << adcPar.t1.FIFO << endl;
		cout << ".......... Rate:                   " << adcPar.t1.dRate << endl;
		cout << ".......... Kadr:                   " << adcPar.t1.dKadr << endl << endl;

		IrqStep = adcPar.t1.IrqStep; // обновили глобальные переменные котрые потом используются в ServiceThread
		pages = adcPar.t1.Pages;

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
		if (status != L_SUCCESS) { M_FAIL("FillDAQparameters(ADC)", status); goto end; }
		else M_OK("FillDAQparameters(ADC)", endl);

		cout << ".......... Buffer size(word):      " << tm << endl;
		cout << ".......... Pages:                  " << adcPar.t2.Pages << endl;
		cout << ".......... IrqStep:                " << adcPar.t2.IrqStep << endl;
		cout << ".......... FIFO:                   " << adcPar.t2.FIFO << endl;
		cout << ".......... Rate:                   " << adcPar.t2.dRate << endl << endl;

		adcPar.t2.dRate = 0;

		status = pI->SetParametersStream(&adcPar.t2, &tm, (void **)&data_rbuf, (void **)&sync, L_STREAM_ADC);
		if (status != L_SUCCESS) { M_FAIL("SetParametersStream(ADC)", status); goto end; }
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
		if (status != L_SUCCESS) { M_FAIL("FillDAQparameters(ADC)", status); goto end; }
		else M_OK("FillDAQparameters(ADC)", endl);

		cout << ".......... Buffer size(word):      " << tm << endl;
		cout << ".......... Pages:                  " << adcPar.t2.Pages << endl;
		cout << ".......... IrqStep:                " << adcPar.t2.IrqStep << endl;
		cout << ".......... FIFO:                   " << adcPar.t2.FIFO << endl;
		cout << ".......... Rate:                   " << adcPar.t2.dRate << endl << endl;

		adcPar.t2.dRate = 0;

		status = pI->SetParametersStream(&adcPar.t2, &tm, (void **)&data_rbuf, (void **)&sync, L_STREAM_ADC);
		if (status != L_SUCCESS) { M_FAIL("SetParametersStream(ADC)", status); goto end; }
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
	if (status != L_SUCCESS) { M_FAIL("GetParameter", status); goto end; }
	else M_OK("GetParameter", endl);
	cout << ".......... Point size:                   " << pointsize << endl << endl;

	cout << endl << ".......... Press any key to start..." << dec << endl << endl;
	_getch();

	cout << ".......... Starting ..." << endl;

	// Создаем файл

	fsize = multi*(pages / 2)*IrqStep; // размер файла

    hFile = CreateFileA("data.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFile == INVALID_HANDLE_VALUE) { M_FAIL("CreateFile(data.dat)", GetLastError()); goto end; }
	else M_OK("CreateFile(data.dat)", endl);


	hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, fsize*pointsize, NULL);
	if (hMap == INVALID_HANDLE_VALUE) { M_FAIL("CreateFileMapping(data.dat)", GetLastError()); goto end; }
	else M_OK("CreateFileMapping(data.dat)", endl);

	fdata = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
	if (fdata == NULL) { M_FAIL("MapViewOfFile(data.dat)", GetLastError()); goto end; }
	else M_OK("MapViewOfFile(data.dat)", endl);

	complete = 0;

	//pI->EnableCorrection(); // можно включить коррекцию данных, если она поддерживается модулем

	status = pI->InitStartLDevice(); // Инициализируем внутренние переменные драйвера
	if (status != L_SUCCESS) { M_FAIL("InitStartLDevice(ADC)", status); goto end; }
	else M_OK("InitStartLDevice(ADC)", endl);

	hThread = CreateThread(0, 0x2000, ServiceThread, 0, 0, &Tid); // Создаем и запускаем поток сбора данных

	status = pI->StartLDevice(); // Запускаем сбор в драйвере
	if (status != L_SUCCESS) { M_FAIL("StartLDevice(ADC)", status); goto end; }
	else M_OK("StartLDevice(ADC)", endl);

	// Печатаем индикатор сбора данных

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
	}

	cout << endl << ".......... Stop." << endl;

	status = pI->StopLDevice(); // Остановили сбор
	if (status != L_SUCCESS) { M_FAIL("StopLDevice(ADC)", status); goto end; }
	else M_OK("StopLDevice(ADC)", endl);

	cout << endl << ".......... Press any key" << dec << endl;
	_getch();

	cout << "Converting ..." << endl;
	// Создаем файл с данными 32 в 16 бит для 791. для других просто копия.

    hFile1 = CreateFileA("data1.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFile == INVALID_HANDLE_VALUE) { M_FAIL("CreateFile(data1.dat)", GetLastError()); goto end; }
	else M_OK("CreateFile(data1.dat)", endl);

	hMap1 = CreateFileMapping(hFile1, NULL, PAGE_READWRITE, 0, fsize * sizeof(short), NULL);
	if (hMap == INVALID_HANDLE_VALUE) { M_FAIL("CreateFileMapping(data1.dat)", GetLastError()); goto end; }
	else M_OK("CreateFileMapping(data1.dat)", endl);

	fdata1 = MapViewOfFile(hMap1, FILE_MAP_WRITE, 0, 0, 0);
	if (fdata == NULL) { M_FAIL("MapViewOfFile(data1.dat)", GetLastError()); goto end; }
	else M_OK("MapViewOfFile(data1.dat)", endl);

	for (k = 0; k<fsize; k++) ((PSHORT)fdata1)[k] = ((PSHORT)fdata)[k*(pointsize >> 1)];

	cout << endl << ".......... Press any key" << dec << endl;
	_getch();

end:

	status = pI->CloseLDevice();
	if (status != L_SUCCESS) { M_FAIL("CloseLDevice", status); /*goto end;*/ }
	else M_OK("CloseLDevice", endl);

	status = pI->Release();
	M_OK("Release IDaqLDevice", endl);
	cout << ".......... Ref: " << status << endl;

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
