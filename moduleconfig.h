#ifndef MODULECONFIG_H
#define MODULECONFIG_H

#include "My8SMC1.h"
#include <windows.h>

class ModuleConfig
{
public:
    LONG   complete;
    HANDLE  hThread;
    ULONG   Tid;
public:
    ModuleConfig();
    int Load(void);
    int Start(My8SMC1 *);
    int Stop(My8SMC1 *);
public:
    double TelikW, TelikH, TelikYStep;
    double AccX,AccY,AccZ;
    int StartX,StartY,StartZ;
    float SpeedX, SpeedY, SpeedZ;
};

#endif // MODULECONFIG_H
