//#include "stdafx.h"
#include "My8SMC1.h"

My8SMC1::My8SMC1()
{
    //Init();
}


My8SMC1::~My8SMC1()
{

}

int My8SMC1::Init(std::string serx, std::string sery, std::string serz)
{
    serX = serx;
    serY = sery;
    serZ = serz;
    return Init();
}

// Init
int My8SMC1::Init()
{
	if (USMC_Init(DVS))
        return 1;
    if (DVS.NOD < 3)
        return 2;
    if (strcmp(DVS.Serial[0],DVS.Serial[1])==0){
        if (USMC_Init(DVS))
            return 3;
    }
    if (strcmp(DVS.Serial[0],DVS.Serial[1])==0){
        if (USMC_Close())
            return 4;
        if (USMC_Init(DVS))
            return 5;
    }
    if (strcmp(DVS.Serial[0],DVS.Serial[1])==0){
        if (USMC_Init(DVS))
            return 6;
    }
    if (strcmp(DVS.Serial[0],DVS.Serial[1])==0)
        return 7;
    //если заданы, то по ним создаём оси
    if (!serX.empty() && !serY.empty() &&!serZ.empty()){
        for (DWORD i = 0; i < DVS.NOD; i++)
        {
            if (strcmp(DVS.Serial[i], serX.c_str())==0){
                DevX = i;
            }
            if (strcmp(DVS.Serial[i], serY.c_str())==0){
                DevY = i;
            }
            if (strcmp(DVS.Serial[i], serZ.c_str())==0){
                DevZ = i;
            }
            //printf("Device - %d,\tSerial Number - %.16s,\tVersion - %.4s\n", i + 1, DVS.Serial[i], DVS.Version[i]);
        }
    }
	//Z
	Dev = DevZ;
	if (USMC_GetMode(Dev, Mode))
        return TRUE;
	Mode.EncoderEn = TRUE;
	Mode.RotTrOp = FALSE;
	Mode.ResetRT = TRUE;
	Mode.Tr1En = TRUE;
	Mode.Tr2En = TRUE;
	Mode.Tr1T = FALSE;
	Mode.Tr2T = FALSE;
	Mode.ResetD = FALSE;
	if (USMC_SetMode(Dev, Mode))
		return TRUE;
    ModeZ=Mode;
	//X
	Dev = DevX;
	if (USMC_GetMode(Dev, Mode))
		return TRUE;
    //Mode.EncoderEn = TRUE;
	Mode.RotTrOp = FALSE;
	//Mode. = FALSE;
	Mode.ResetRT = TRUE;
	Mode.Tr1En = TRUE;	
	Mode.Tr2En = TRUE;
	Mode.Tr1T = TRUE;
	Mode.Tr2T = TRUE;
	Mode.ResetD = FALSE;
    Mode.SyncOUTEn = TRUE;
    Mode.SyncCount = 1;
	if (USMC_SetMode(Dev, Mode))
		return TRUE;
    ModeX=Mode;
	//Y
	Dev = DevY;
	if (USMC_GetMode(Dev, Mode))
		return TRUE;
	Mode.EncoderEn = TRUE;
	Mode.RotTrOp = FALSE;
	Mode.ResetRT = TRUE;
	Mode.Tr1En = TRUE;
	Mode.Tr2En = TRUE;
	Mode.Tr1T = TRUE;
	Mode.Tr2T = TRUE;
	Mode.ResetD = FALSE;
	if (USMC_SetMode(Dev, Mode))
		return TRUE;
    ModeY=Mode;
    //
	GetPrmsAll();
	return 0;
}

int My8SMC1::SetMode(DWORD Dev, USMC_Mode Mode)
{
    return USMC_SetMode(Dev, Mode);
}

// MoveX
int My8SMC1::MoveX(int DestPos)
{
	return MoveDev(DevX, DestPos);
}
int My8SMC1::MoveY(int DestPos)
{
	return MoveDev(DevY, DestPos);
}
int My8SMC1::MoveZ(int DestPos)
{
	return MoveDev(DevZ, DestPos);
}

int My8SMC1::MoveDevSync(DWORD Dev, int DestPos)
{
    MoveDev(Dev, DestPos);
    while(DestPos!=State.CurPos){
        if (USMC_GetState(Dev, State))
            return TRUE;
        if ((State.AReset==0) || (State.Trailer1) || (State.Trailer2)){
            break;
        }
        Sleep(10);
    }
    return 0;
}

int My8SMC1::MoveXSync(int DestPos)
{
    //if (DestPos==0) DestPos = 3;
    MoveDev(DevX, DestPos);
    while(DestPos!=StateX.CurPos){
        if (USMC_GetState(DevX, StateX))
            return TRUE;
        Sleep(20);
    }
    return 0;
}

int My8SMC1::MoveYSync(int DestPos)
{
    MoveDev(DevY, DestPos);
    while(DestPos!=StateY.CurPos){
        if (USMC_GetState(DevY, StateY))
            return TRUE;
        Sleep(20);
    }
    return 0;
}

int My8SMC1::MoveZSync(int DestPos)
{
    MoveDev(DevZ, DestPos);
    while(DestPos!=StateZ.CurPos){
        if (USMC_GetState(DevZ, StateZ))
            return TRUE;
        Sleep(20);
    }
    return 0;
}

int My8SMC1::HomeX(void)
{
    return HomeDev(DevX);
}
int My8SMC1::HomeY(void)
{
    return HomeDev(DevY);
}
int My8SMC1::HomeZ(void)
{
    return HomeDev(DevZ);
}

int My8SMC1::WaitDoneAll(void)
{
    while (1){
        if (USMC_GetState(DevX, StateX))
            return TRUE;
        if (StateX.CurPos==TargetX) break;
        Sleep(20);
    };
    while (1){
        if (USMC_GetState(DevY, StateY))
            return TRUE;
        if (StateY.CurPos==TargetY) break;
        Sleep(20);
    };
    while (1){
        if (USMC_GetState(DevZ, StateZ))
            return TRUE;
        if (StateZ.CurPos==TargetZ) break;
        Sleep(20);
    };
    return 0;
}

int My8SMC1::GetInfo(void)
{
    if (USMC_GetState(DevX, StateX))
        return TRUE;
    if (USMC_GetState(DevY, StateY))
        return TRUE;
    if (USMC_GetState(DevZ, StateZ))
        return TRUE;
    //
    if (USMC_GetMode(DevX, ModeX))
        return TRUE;
    if (USMC_GetMode(DevY, ModeY))
        return TRUE;
    if (USMC_GetMode(DevZ, ModeZ))
        return TRUE;
	return 0;
}

int My8SMC1::Flash(void)
{
	Dev = DevX;
	if (USMC_SaveParametersToFlash(Dev))
		return TRUE;
	Dev = DevY;
	if (USMC_SaveParametersToFlash(Dev))
		return TRUE;
	Dev = DevZ;
	if (USMC_SaveParametersToFlash(Dev))
		return TRUE;
	return 0;
}

int My8SMC1::SetPrmsAll(void)
{
	if (SetPrmsDev(DevX))
		return TRUE;
	if (SetPrmsDev(DevY))
		return TRUE;
	if (SetPrmsDev(DevZ))
		return TRUE;
	return 0;
}

int My8SMC1::GetPrmsAll(void)
{
	if (GetPrmsDev(DevX))
		return TRUE;
	if (GetPrmsDev(DevY))
		return TRUE;
	if (GetPrmsDev(DevZ))
		return TRUE;
	return 0;
}

int My8SMC1::SetAccXYZ(float accX, float accY, float accZ)
{
	if (SetAccX(accX))
		return TRUE;
	if (SetAccY(accY))
		return TRUE;
	if (SetAccZ(accZ))
		return TRUE;
	return 0;
}

int My8SMC1::SetSpeedXYZ(float speX, float speY, float speZ)
{
    if (SetSpeedX(speX))
        return TRUE;
    if (SetSpeedY(speY))
        return TRUE;
    if (SetSpeedZ(speZ))
        return TRUE;
    return 0;
}

int My8SMC1::SetSpeedX(float spe)
{
    SpeedX = spe;
    return 0;
}

int My8SMC1::SetSpeedY(float spe)
{
    SpeedY = spe;
    return 0;
}

int My8SMC1::SetSpeedZ(float spe)
{
    SpeedZ = spe;
    return 0;
}

int My8SMC1::SetAccX(float acc)
{
	PrmsX.AccelT = acc;
	if (SetPrmsDev(DevX))
		return TRUE;
	return 0;
}

int My8SMC1::SetAccY(float acc)
{
	PrmsY.AccelT = acc;
	if (SetPrmsDev(DevY))
		return TRUE;
	return 0;
}

int My8SMC1::SetAccZ(float acc)
{
	PrmsZ.AccelT = acc;
	if (SetPrmsDev(DevZ))
		return TRUE;
	return 0;
}

int My8SMC1::SetDecX(float dec)
{
	PrmsX.DecelT = dec;
	if (SetPrmsDev(DevX))
		return TRUE;
	return 0;
}

int My8SMC1::SetDecY(float dec)
{
	PrmsY.DecelT = dec;
	if (SetPrmsDev(DevY))
		return TRUE;
	return 0;
}

int My8SMC1::SetDecZ(float dec)
{
	PrmsZ.DecelT = dec;
	if (SetPrmsDev(DevZ))
		return TRUE;
	return 0;
}

int My8SMC1::SetPrmsDev(int Dev)
{
    if (GetPrmsByDev(Dev, &Prms_Ptr))
		return TRUE;
	if (USMC_SetParameters(Dev, *Prms_Ptr))
		return TRUE;
	return 0;
}

int My8SMC1::GetPrmsDev(int Dev)
{
    //Prms_Ptr = &PrmsY;
    if (GetPrmsByDev(Dev, &Prms_Ptr))
		return TRUE;
	if (USMC_GetParameters(Dev, *Prms_Ptr))
		return TRUE;
	return 0;
}

int My8SMC1::MoveDev(DWORD Dev, int DestPos)
{
    SetTargetByDev(Dev, DestPos);
	if (USMC_GetStartParameters(Dev, StPrms))
		return TRUE;
	StPrms.SlStart = TRUE;
    StPrms.SDivisor = 8;
    Speed = GetSpeedByDev(Dev);
	if (USMC_Start(Dev, DestPos, Speed, StPrms))
		return TRUE;
	//flash
	//Sleep(10);
	//if (USMC_SaveParametersToFlash(Dev))
	//	return TRUE;
	return 0;
}

void My8SMC1::SetTargetByDev(DWORD Dev, int DestPos)
{
    if (Dev==DevX) TargetX = DestPos;
    if (Dev==DevY) TargetY = DestPos;
    if (Dev==DevZ) TargetZ = DestPos;
    //return 0;
}

int My8SMC1::StopDev(DWORD Dev)
{
    if (USMC_Stop(Dev))
        return TRUE;
    return 0;
}

int My8SMC1::StopX(void)
{
    return USMC_Stop(DevX);
}

int My8SMC1::StopY(void)
{
    return USMC_Stop(DevY);
}

int My8SMC1::StopZ(void)
{
    return USMC_Stop(DevZ);
}

int My8SMC1::HomeDev(DWORD Dev)
{
    SetSpeedXYZ(HomeSpeed,HomeSpeed,HomeSpeed);//!
	//phase 1
	do {
		if (USMC_GetState(Dev, State))
			return TRUE;
		if (State.Trailer1) {
            MoveDevSync(Dev, State.CurPos + 100);
		}
		if (State.Trailer2) {
            MoveDevSync(Dev, State.CurPos - 100);
		}
	} while (State.Trailer1 || State.Trailer2);
	//phase 2
	do {
		if (USMC_GetState(Dev, State))
			return TRUE;
		if (!State.Trailer1) {
            MoveDevSync(Dev, State.CurPos - 100);
		}
	} while (!State.Trailer1);
	//
	do {
		if (USMC_GetState(Dev, State))
			return TRUE;
		if (State.Trailer1) {
            MoveDevSync(Dev, State.CurPos + 1);
		}
	} while (State.Trailer1);
    Sleep(30);
    //set 0
    int HomeZero = 0;//!!
	if (USMC_SetCurrentPosition(Dev, HomeZero))
        return TRUE;
//    Sleep(50);
//    if (USMC_GetState(Dev, State))
//        return TRUE;
//    if (State.CurPos != 0){
//        if (USMC_SetCurrentPosition(Dev, HomeZero))
//            return TRUE;
//    }
//    if (MoveDevSync(Dev, 0))
//        return 1;
	//flash
    Sleep(30);
	if (USMC_SaveParametersToFlash(Dev))
		return TRUE;
	return 0;
}

int My8SMC1::GetPrmsByDev(int InDev, USMC_Parameters **OutPrms)
{
    if (InDev==DevX){
        *OutPrms = (USMC_Parameters *)&PrmsX;
    }
    if (InDev==DevY){
        *OutPrms = (USMC_Parameters *)&PrmsY;
    }
    if (InDev==DevZ){
        *OutPrms = (USMC_Parameters *)&PrmsZ;
    }
	return 0;
}

float My8SMC1::GetSpeedByDev(int InDev)
{
    if (InDev==DevX){
        return SpeedX;
    }
    if (InDev==DevY){
        return SpeedY;
    }
    if (InDev==DevZ){
        return SpeedZ;
    }
    return 0;
}
