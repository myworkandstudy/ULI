#ifndef ADCREAD_H
#define ADCREAD_H

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
