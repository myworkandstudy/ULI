#include "stdafx.h"
#include "My8SMC1.h"


My8SMC1::My8SMC1()
{
	Init();
}


My8SMC1::~My8SMC1()
{

}

// Init
int My8SMC1::Init()
{
	if (USMC_Init(DVS))
		return 1;
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
	//X
	Dev = DevX;
	if (USMC_GetMode(Dev, Mode))
		return TRUE;
	Mode.EncoderEn = TRUE;
	Mode.RotTrOp = FALSE;
	//Mode. = FALSE;
	Mode.ResetRT = TRUE;
	Mode.Tr1En = TRUE;	
	Mode.Tr2En = TRUE;
	Mode.Tr1T = TRUE;
	Mode.Tr2T = TRUE;
	Mode.ResetD = FALSE;
	if (USMC_SetMode(Dev, Mode))
		return TRUE;
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
	//
	GetPrmsAll();
	return 0;
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
	HomeDev(DevX);
	return 0;
}
int My8SMC1::HomeY(void)
{
	HomeDev(DevY);
	return 0;
}
int My8SMC1::HomeZ(void)
{
	HomeDev(DevZ);
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
	if (USMC_GetStartParameters(Dev, StPrms))
		return TRUE;
	StPrms.SlStart = TRUE;
	if (USMC_Start(Dev, DestPos, Speed, StPrms))
		return TRUE;
	//flash
	//Sleep(10);
	//if (USMC_SaveParametersToFlash(Dev))
	//	return TRUE;
	return 0;
}

int My8SMC1::HomeDev(DWORD Dev)
{
	//phase 1
	do {
		if (USMC_GetState(Dev, State))
			return TRUE;
		if (State.Trailer1) {
			MoveDev(Dev, State.CurPos + 100);
			Sleep(100);
		}
		if (State.Trailer2) {
			MoveDev(Dev, State.CurPos - 100);
			Sleep(100);
		}
	} while (State.Trailer1 || State.Trailer2);
	//phase 2
	do {
		if (USMC_GetState(Dev, State))
			return TRUE;
		if (!State.Trailer1) {
			MoveDev(Dev, State.CurPos - 100);
			Sleep(100);
		}
	} while (!State.Trailer1);
	//
	do {
		if (USMC_GetState(Dev, State))
			return TRUE;
		if (State.Trailer1) {
			MoveDev(Dev, State.CurPos + 1);
			Sleep(10);
		}
	} while (State.Trailer1);
	//set 0
	Sleep(10);
	int HomeZero = 0;
	if (USMC_SetCurrentPosition(Dev, HomeZero))
		return TRUE;	
	//flash
	Sleep(10);
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
