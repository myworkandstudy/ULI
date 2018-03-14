#ifndef ADCREAD_H
#define ADCREAD_H

#include <windows.h>

class ADCRead
{
public:
    ADCRead();
    ~ADCRead();
    int Init(void);
    ULONG GetValue0(void);
    int StartGetData(void);
    int StopGetData(void);
    int End(void);
    int FixS(void);
    ULONG64 GetCureByteNum();
    int WriteToFile2(void);
    //
    ULONG CureVal;
    ULONG databufsize;
    ULONG CureS;
    ULONG sCureS;
    ULONG64 CureByteNum;
    ULONG64 CureBIdxFull;
    int CureArrIdx;
    ULONG64 arrDataIdx[1000];
    int IsStarted;
};

#endif // ADCREAD_H
