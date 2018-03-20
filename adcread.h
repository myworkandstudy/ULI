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
    ULONG GetValue1(void);
    int StartGetData(void);
    int StopGetData(void);
    int End(void);
    int FixS(void);
    ULONG64 GetCureByteNum();
    int WriteToFile2(void);
    //
    ULONG CureVal, CureVal2;
    ULONG databufsize;
    ULONG CureS;
    ULONG sCureS;
    ULONG64 CureByteNum;
    volatile LONG64 CureBIdxFull;
    int CureArrIdx;
    ULONG64 arrDataIdx[1000];
    int IsStarted;
    double sett_dRate_kHz;
};

#endif // ADCREAD_H
