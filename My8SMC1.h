#ifndef MY8SMC1_H
#define MY8SMC1_H

#pragma warning(disable : 4996) // Disable warnings about some functions in VS 2005
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <tchar.h>
#include <conio.h>
#include <process.h>
#include <string>
#include "USMCDLL.h"
//#include "moduleconfig.h"

#pragma once
class My8SMC1
{
public:
    My8SMC1();
    ~My8SMC1();
    //ModuleConfig *MC;
	USMC_State StateX;
	USMC_State StateY;
	USMC_State StateZ;
    float SpeedX, SpeedY, SpeedZ;
    int TargetX, TargetY, TargetZ;
    float mkmX, mkmY, mkmZ;
    USMC_Parameters PrmsX, PrmsY, PrmsZ;
    USMC_Mode ModeX, ModeY, ModeZ;
    int DevX = 0;
    int DevY = 1;
    int DevZ = 2;
    std::string serX, serY, serZ;
    float ManSpeed = 100.0f;
private:
	USMC_Devices DVS;
	DWORD Dev;
    USMC_State State;
    USMC_StartParameters StPrms;
	USMC_Parameters *Prms_Ptr;
	USMC_Mode Mode;
	USMC_EncoderState EnState;
	float Speed = 2000.0f;
    float HomeSpeed = 100.0f;

public:
	// Init
    int Init(std::string serx, std::string sery, std::string serz);
	int Init();
    //
    int SetMode(DWORD Dev, USMC_Mode Mode);
    // Move
    int MoveDevSync(DWORD Dev, int DestPos);
	int MoveX(int DestPos);
	int MoveY(int DestPos);
	int MoveZ(int DestPos);
    int MoveXSync(int DestPos);
    int MoveYSync(int DestPos);
    int MoveZSync(int DestPos);
	//
	int HomeX(void);
	int HomeY(void);
	int HomeZ(void);
    int WaitDoneAll(void);
	int GetInfo(void);
	int Flash(void);
	int SetPrmsDev(int);
	int GetPrmsDev(int);
	int SetPrmsAll(void);
	int GetPrmsAll(void);
	int SetAccXYZ(float, float, float);
    int SetSpeedXYZ(float, float, float);
    int SetSpeedX(float);
    int SetSpeedY(float);
    int SetSpeedZ(float);
	int SetAccX(float);
	int SetAccY(float);
	int SetAccZ(float);
	int SetDecX(float);
	int SetDecY(float);
	int SetDecZ(float);
    int StopX(void);
    int StopY(void);
    int StopZ(void);
private:
    void SetTargetByDev(DWORD Dev, int DestPos);
	int MoveDev(DWORD Dev, int DestPos);
    int StopDev(DWORD Dev);
	int HomeDev(DWORD Dev);
    int GetPrmsByDev(int InDev, USMC_Parameters **OutPrms);
    float GetSpeedByDev(int InDev);
};

#endif // M
