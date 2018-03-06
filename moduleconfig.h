#ifndef MODULECONFIG_H
#define MODULECONFIG_H

#include "My8SMC1.h"
#include "adcread.h"
#include <windows.h>

typedef struct InterpStri {
    int TargetSpeedTic;
    ULONG StartPos;
    ULONG EndPos;
    double AccT;
    double DecT;
    double MkmPerFTic;
    int Divisor;
    ULONG64 StartByteNum;
    ULONG64 EndByteNum;
    double y;
    double z;
} TInterpStri;

#define SETTINGSFILENAME "settings.json"

class ModuleConfig
{
public:
    LONG   complete;
    HANDLE  hThread;
    ULONG   Tid;
public:
    ModuleConfig();
    int Load(void);
    int Save(void);
    int Start(My8SMC1 *, ADCRead *PADC);
    int Stop(My8SMC1 *);
    int LoadStriFromFile2();/*ULONG in_pos1, ULONG in_pos2, UINT16 *Out_ArrValue,
                         int Out_TargetSpeedTic, int Out_StartPos, int Out_EndPos,
                         ULONG64 Out_StartByteNum, ULONG64 Out_EndByteNum,
                         double Out_AccT, double Out_DecT, double Out_MkmPerFTic, int Out_Divisor);*/
    int LoadDataFromFile(TInterpStri *, UINT16 *&);
    int MakeDataFile();
    int CalcInterpolAndWrite(UINT16 *ArrValue, TInterpStri*PStri, FILE* file);
    int FixStart(ULONG64 ByteNum, int pos, int speed, float acc, float dec, double MkmPerFTic, int divisor, double y, double z);
    int FixStop(ULONG64 ByteNum, int pos);
    int WriteArrToFile2();
    int CalcParam(TInterpStri *PStri);
    int WriteToFile3(TInterpStri *PStri, ULONG *mArrPos, UINT16 *mArrValue);
public:
    //from file
    double TelikW, TelikH, TelikYStep;
    double AccX,AccY,AccZ;
    int StartX,StartY,StartZ;
    float SpeedX, SpeedY, SpeedZ;
    float mkmX, mkmY, mkmZ;
    std::string ConfigFilePath;
    std::wstring ExperFileName;
    int mystate;
    //ca
private:
    ULONG Freq = 100000;
    int MinSpeedFTic = 300, MaxSpeedFTic = 5000;
    //!
    double MkmPerTic, MkmPerMs;
    double Aacc,Bacc,Cacc,Adec,Bdec,Cdec;
    int CureArrIdx;
    TInterpStri arrData[1000];
};

#endif // MODULECONFIG_H
