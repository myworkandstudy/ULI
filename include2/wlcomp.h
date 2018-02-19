#ifdef __cplusplus
   #define DllExport(type) extern "C" __declspec( dllexport ) type
#else 
   #define DllExport(type) type
#endif
// ^раскомментировать при компиляции DLL

#define CASE_DAC_PAR_0 0
#define CASE_DAC_PAR_1 1
#define CASE_ADC_PAR_0 2
#define CASE_ADC_PAR_1 3

DllExport(ULONG) LoadAPIDLL(char *dllname);
// загружает lcomp.dll
DllExport(ULONG) FreeAPIDLL(PULONG hDll);
// выгружает lcomp.dll
DllExport(LPVOID) CallCreateInstance(PULONG hDll, ULONG slot, PULONG Err); // return handle for new iface
// получает указатель на интерфейс для последующего вызова функций

// все функции возвращают ошибку как и в оригинальной библиотеке
// первый параметр это указатель на интерфейс
// все остальные как в оригинальной библиотеке за исключением параметров структур
// параметры-структуры модифицированы до простых Сишных... посредством Union
// соответсвенно поле в union выбирается дополнительным параметром числом 0 1 2 3
// при написании примеров для правильной компиляции перед всеми include надо определить
// #define LABVIEW_FW

DllExport(ULONG)  inbyte ( PULONG hIfc, ULONG offset, PUCHAR data, ULONG len, ULONG key);
DllExport(ULONG)  inword ( PULONG hIfc, ULONG offset, PUSHORT data, ULONG len, ULONG key);
DllExport(ULONG)  indword( PULONG hIfc, ULONG offset, PULONG data, ULONG len, ULONG key);

DllExport(ULONG)  outbyte ( PULONG hIfc, ULONG offset, PUCHAR data, ULONG len, ULONG key);
DllExport(ULONG)  outword ( PULONG hIfc, ULONG offset, PUSHORT data, ULONG len, ULONG key);
DllExport(ULONG)  outdword( PULONG hIfc, ULONG offset, PULONG data, ULONG len, ULONG key);

   // Working with MEM ports
DllExport(ULONG)  inmbyte ( PULONG hIfc, ULONG offset, PUCHAR data, ULONG len, ULONG key);
DllExport(ULONG)  inmword ( PULONG hIfc, ULONG offset, PUSHORT data, ULONG len, ULONG key);
DllExport(ULONG)  inmdword( PULONG hIfc, ULONG offset, PULONG data, ULONG len, ULONG key);

DllExport(ULONG)  outmbyte ( PULONG hIfc, ULONG offset, PUCHAR data, ULONG len, ULONG key);
DllExport(ULONG)  outmword ( PULONG hIfc, ULONG offset, PUSHORT data, ULONG len, ULONG key);
DllExport(ULONG)  outmdword( PULONG hIfc, ULONG offset, PULONG data, ULONG len, ULONG key);

DllExport(ULONG)  GetWord_DM(PULONG hIfc, USHORT Addr, PUSHORT Data);
DllExport(ULONG)  PutWord_DM(PULONG hIfc, USHORT Addr, USHORT Data);
DllExport(ULONG)  PutWord_PM(PULONG hIfc, USHORT Addr, ULONG Data);
DllExport(ULONG)  GetWord_PM(PULONG hIfc, USHORT Addr, PULONG Data);

DllExport(ULONG)  GetArray_DM(PULONG hIfc, USHORT Addr, ULONG Count, PUSHORT Data);
DllExport(ULONG)  PutArray_DM(PULONG hIfc, USHORT Addr, ULONG Count, PUSHORT Data);
DllExport(ULONG)  PutArray_PM(PULONG hIfc, USHORT Addr, ULONG Count, PULONG Data);
DllExport(ULONG)  GetArray_PM(PULONG hIfc, USHORT Addr, ULONG Count, PULONG Data);

DllExport(ULONG)  SendCommand(PULONG hIfc, USHORT Cmd);

DllExport(ULONG)  PlataTest(PULONG hIfc);

DllExport(ULONG)  GetSlotParam(PULONG hIfc, PSLOT_PAR slPar);

DllExport(ULONG) OpenLDevice(PULONG hIfc);
DllExport(ULONG)  CloseLDevice(PULONG hIfc);

///
DllExport(ULONG)  SetParametersStream(PULONG hIfc, LPVOID sp, ULONG sp_type, ULONG *UsedSize, void** Data, void** Sync, ULONG StreamId);

DllExport(ULONG)  RequestBufferStream(PULONG hIfc, ULONG *Size, ULONG StreamId); //in words
DllExport(ULONG)  FillDAQparameters(PULONG hIfc, LPVOID sp, ULONG sp_type);
///

DllExport(ULONG)  InitStartLDevice(PULONG hIfc);
DllExport(ULONG)  StartLDevice(PULONG hIfc);
DllExport(ULONG)  StopLDevice(PULONG hIfc);

DllExport(ULONG)  LoadBios(PULONG hIfc, char *FileName);


DllExport(ULONG)  IoAsync(PULONG hIfc, PWASYNC_PAR sp);  // collect all async io operations

DllExport(ULONG)  ReadPlataDescr(PULONG hIfc, LPVOID pd);
DllExport(ULONG)  WritePlataDescr(PULONG hIfc, LPVOID pd, USHORT Ena);
DllExport(ULONG)  ReadFlashWord(PULONG hIfc, USHORT FlashAddress, PUSHORT Data);
DllExport(ULONG)  WriteFlashWord(PULONG hIfc, USHORT FlashAddress, USHORT Data);
DllExport(ULONG)  EnableFlashWrite(PULONG hIfc, USHORT Flag);

DllExport(ULONG)  EnableCorrection(PULONG hIfc, USHORT Ena);

DllExport(ULONG)  GetParameter(PULONG hIfc, ULONG name, PULONG param);
DllExport(ULONG)  SetParameter(PULONG hIfc, ULONG name, PULONG param);


DllExport(ULONG)  SetLDeviceEvent(PULONG hIfc, ULONG hEvent, ULONG EventId);


// For LabView

DllExport(ULONG) GetSyncData(PULONG hIfc, ULONG SyncPtr, ULONG Offset, ULONG *Sync);
// читает из адреса описываемого 32 битным числом SyncPtr значение в переменную Sync

DllExport(ULONG) GetDataFromBuffer(PULONG hIfc, ULONG DataPtr, LPVOID DataArray, ULONG size, ULONG mask);
// копирует данные из буфера аналогично

DllExport(ULONG) PutDataToBuffer(PULONG hIfc, ULONG DataPtr, LPVOID DataArray, ULONG size);
// копирует данные в буфер аналогично

DllExport(LPVOID) Get_LDEV2_Interface(PULONG hIfc, PULONG Err); // return handle for new iface

DllExport(ULONG)  InitStartLDeviceEx(PULONG hIfc, ULONG StreamId);

DllExport(ULONG)  StartLDeviceEx(PULONG hIfc, ULONG StreamId);

DllExport(ULONG)  StopLDeviceEx(PULONG hIfc, ULONG StreamId);

DllExport(ULONG)  Release_LDEV2_Interface(PULONG hIfc);
