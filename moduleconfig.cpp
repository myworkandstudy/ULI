#include "moduleconfig.h"
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

ULONG WINAPI SynThread(PVOID /*Context*/);


ModuleConfig::ModuleConfig()
{
    hThread = NULL;
}

int ModuleConfig::Load(void)
{
    try{
        QString val;
        QFile file;
        file.setFileName("C:\\Users\\102324\\source\\repos\\ConsoleApplication1\\ConsoleApplication1\\config1.json");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        val = file.readAll();
        file.close();
        QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
        qWarning() << d.object()["Telik"].toObject()["W"].toInt();

        //
        TelikW = d.object()["Telik"].toObject()["W"].toDouble();
        TelikH = d.object()["Telik"].toObject()["H"].toDouble();
        TelikYStep = d.object()["Telik"].toObject()["YStep"].toDouble();
        //
        AccX = d.object()["Acceleration"].toObject()["X"].toDouble();
        AccY = d.object()["Acceleration"].toObject()["Y"].toDouble();
        AccZ = d.object()["Acceleration"].toObject()["Z"].toDouble();
        //
        StartX = d.object()["StartPos"].toObject()["X"].toInt();
        StartY = d.object()["StartPos"].toObject()["Y"].toInt();
        StartZ = d.object()["StartPos"].toObject()["Z"].toInt();
        //
        SpeedX = d.object()["Speed"].toObject()["X"].toInt();
        SpeedY = d.object()["Speed"].toObject()["Y"].toInt();
        SpeedZ = d.object()["Speed"].toObject()["Z"].toInt();
    } catch (...){
        return 1;
    }
    return 0;
}


// Поток для синхронного перемещения
ULONG WINAPI SynThread(PVOID stk/*Context*/)
{
    void** ctx = (void**)stk;
    My8SMC1 *PStanda = (My8SMC1*)ctx[0];
    ModuleConfig *MC = (ModuleConfig*)ctx[1];
    ADCRead *PADC = (ADCRead*)ctx[2];
    //
    //Move to start position
    PStanda->MoveX(MC->StartX);
    PStanda->MoveY(MC->StartY);
    PStanda->MoveZ(MC->StartZ);
    //Ожидаем выполнения до целей
    PStanda->WaitDoneAll();
    //if (PStanda->GetInfo()) return 4;
    //Start Telik
    int xpos1 = PStanda->StateX.CurPos;
    int xpos2 = xpos1 + MC->TelikW;
    int pari = 0;
    int ystep = (int) MC->TelikYStep;
    int ystart = PStanda->StateY.CurPos + ystep;
    for (int ypos=ystart; ypos<=MC->TelikH; ypos+=ystep){
        //Move X
        MC->FixStart(PADC->GetCureByteNum(), PStanda->StateX.CurPos, PStanda->SpeedX, PStanda->PrmsX.AccelT, PStanda->PrmsX.DecelT, MC->mkmX, PStanda->StateX.SDivisor, ypos, PStanda->StateZ.CurPos);
        pari = !pari;
        if (pari){
            if (PStanda->MoveXSync(xpos2))
                return 1;
        } else {
            if (PStanda->MoveXSync(xpos1))
                return 2;
        }
        MC->FixStop(PADC->GetCureByteNum(), PStanda->StateX.CurPos);
        //
        if (PStanda->MoveYSync(ypos))
            return 3;
    }
    return 0;
    //
    //ModuleConfig *MC = (ModuleConfig*)stk;
    if (!MC)
        return TRUE;

    for (int i = 0; i<10; i++)                         // Цикл по необходимомму количеству половинок
    {
        while (1)
        {
            if (InterlockedCompareExchange(&MC->complete, 3, 3))
                return 0;
        }

        Sleep(0);                                     // если собираем медленно то можно и спать больше
    }
    return 0;                                         // Вышли
}


int ModuleConfig::Start(My8SMC1 *PStanda, ADCRead *PADC)
{
    void** ctx;
    ctx = new PVOID [2];
    ctx[0] = PStanda;
    ctx[1] = this;
    ctx[2] = PADC;
    //
    CureArrIdx=0;
    //
    hThread = CreateThread(0, 0x2000, SynThread, ctx, 0, &Tid); // Создаем и запускаем поток сбора данных
    //
    return 0;
}

int ModuleConfig::Stop(My8SMC1 *PStanda)
{
    if (hThread) CloseHandle(hThread);
    PStanda->StopX();
    PStanda->StopY();
    PStanda->StopZ();
    return 0;
}

int ModuleConfig::FixStart(ULONG64 ByteNum, int pos, int speed, float acc, float dec, double MkmPerFTic, int divisor, double y, double z)
{
    arrData[CureArrIdx].StartByteNum = ByteNum;
    arrData[CureArrIdx].StartPos = pos;
    arrData[CureArrIdx].TargetSpeedTic = speed;
    arrData[CureArrIdx].AccT = acc;
    arrData[CureArrIdx].DecT = dec;
    arrData[CureArrIdx].MkmPerFTic = MkmPerFTic;
    arrData[CureArrIdx].Divisor = divisor;
    arrData[CureArrIdx].y = y;
    arrData[CureArrIdx].z = z;
    return 0;
}

int ModuleConfig::FixStop(ULONG64 ByteNum, int pos)
{
    arrData[CureArrIdx].EndByteNum = ByteNum;
    arrData[CureArrIdx].EndPos = pos;
    CureArrIdx++;
    if (CureArrIdx>=1000)
        CureArrIdx=0;
    return 0;
}

int ModuleConfig::WriteArrToFile2()
{
    //char fileName[100] = ; // Путь к файлу для записи
    FILE* file = fopen("data2.txt", "w");
    if (file) // если есть доступ к файлу,
    {
        for (int i=0;i<CureArrIdx;i++){
            fprintf_s(file,"%d %lld %lld %ld %ld %lf %lf %lf %d %lf %lf\n", i, arrData[i].StartByteNum, arrData[i].EndByteNum, arrData[i].StartPos, arrData[i].EndPos,
                                            arrData[i].AccT, arrData[i].DecT, arrData[i].MkmPerFTic, arrData[i].Divisor, arrData[i].y, arrData[i].z);
        }
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
    }
    fclose(file);
    return 0;
}

int ModuleConfig::LoadStriFromFile2()
        //ULONG in_pos1, ULONG in_pos2, UINT16 *Out_ArrValue,                                   int Out_TargetSpeedTic, int Out_StartPos, int Out_EndPos,                                   ULONG64 Out_StartByteNum, ULONG64 Out_EndByteNum,                                   double Out_AccT, double Out_DecT, double Out_MkmPerFTic, int Out_Divisor)
{
    //char fileName[100] = ; // Путь к файлу для записи
    FILE* file = fopen("data2.txt", "w");
    if (file) // если есть доступ к файлу,
    {
        CureArrIdx = 0;
        int i,read_ok;
        do {
            i = CureArrIdx;
            read_ok = fscanf_s(file,"%d %lld %lld %ld %ld %lf %lf %lf %d %lf %lf\n", &i, &arrData[i].StartByteNum, &arrData[i].EndByteNum, &arrData[i].StartPos, &arrData[i].EndPos,
                               &arrData[i].AccT, &arrData[i].DecT, &arrData[i].MkmPerFTic, &arrData[i].Divisor, &arrData[i].y, &arrData[i].z);
            if (read_ok){
                CureArrIdx++;
            }
        } while (read_ok);
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
    }
    fclose(file);
    return 0;
}

int ModuleConfig::LoadDataFromFile(TInterpStri *PStri, UINT16 *out_arrData)
{
    //
    out_arrData = NULL;
    return 0;
}

int ModuleConfig::CalcParam(TInterpStri *PStri)
{
    MkmPerTic = PStri->MkmPerFTic/PStri->Divisor;
    MkmPerMs = PStri->TargetSpeedTic*MkmPerTic/1000;

    Aacc = MinSpeedFTic - MaxSpeedFTic;
    Bacc = PStri->AccT - 0;
    Cacc = 0 - PStri->AccT*MinSpeedFTic;
    Adec = MaxSpeedFTic - MinSpeedFTic;
    Bdec = PStri->DecT - 0;
    Cdec = 0 - PStri->DecT*MaxSpeedFTic;
    return 0;
}

int ModuleConfig::MakeDataFile()
{
    UINT16 *ArrValue = NULL;
    ULONG *ArrPos;    //TInterpStri Stri;
    LoadStriFromFile2();
    FILE* file = fopen("data3.csv", "w"); fclose(file); //create empty file3
    for (int i=0;i<CureArrIdx;i++){//цикл по строкам
        LoadDataFromFile(&arrData[i], ArrValue);
        CalcParam(&arrData[i]);
        ULONG DataSize = arrData[i].EndByteNum - arrData[i].StartByteNum;
        ArrPos = new ULONG[DataSize];//Make Arr
        CalcInterpol(ArrPos, &arrData[i]);
        WriteToFile3(&arrData[i], ArrPos, ArrValue);
        delete [] ArrPos;//Del Arr
    }
    return 0;
}

int ModuleConfig::WriteToFile3(TInterpStri *PStri, ULONG *mArrPos, UINT16 *mArrValue)
{
    FILE* file = fopen("data3.csv", "a");
    if (file) // если есть доступ к файлу,
    {
        ULONG DataSize = PStri->EndByteNum - PStri->StartByteNum;
        for (UINT i=0;i<DataSize;i++){
            //              x   y  z  val
            fprintf_s(file,"%ld %d %d %d\n", mArrPos[i], (int)PStri->y, (int)PStri->z, mArrValue[i]);
        }
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
    }
    fclose(file);
    return 0;
}

int ModuleConfig::CalcInterpol(ULONG *ArrPos, TInterpStri*PStri)
{
    ULONG CurePos = PStri->StartPos;
    int CureSpeedTic = (PStri->TargetSpeedTic < MinSpeedFTic ? MinSpeedFTic : PStri->TargetSpeedTic);
    int ByteNum = 0;
    ULONG NextTicInFq;
    double TargetPosM;
    //---Accel---
    if (PStri->TargetSpeedTic > MinSpeedFTic){
        do{
            NextTicInFq = Freq/CureSpeedTic;
            for (UINT f=0;f<NextTicInFq;f++){
                ArrPos[ByteNum + f] = CurePos;
            }
            CurePos += 1;//MkmPerTic;
            ByteNum += NextTicInFq;
            CureSpeedTic += 1;
        } while (CureSpeedTic < PStri->TargetSpeedTic);
        TargetPosM = PStri->EndPos - (PStri->DecT - (((0-Cdec)-(Bdec*PStri->TargetSpeedTic))/Adec))*MkmPerMs;//!это не точная позиция замедления, надо по fq
    } else {
        TargetPosM = PStri->EndPos;
    }
    //---Moving---
    do{
        NextTicInFq = Freq/CureSpeedTic;
        for (UINT f=0;f<NextTicInFq;f++){
            ArrPos[ByteNum + f] = CurePos;
        }
        CurePos += 1;//MkmPerTic;
        ByteNum += NextTicInFq;
    } while (CurePos < TargetPosM);
    //---Decel---
    if (PStri->TargetSpeedTic > MinSpeedFTic){
        do{
            NextTicInFq = Freq/CureSpeedTic;
            for (UINT f=0;f<NextTicInFq;f++){
                ArrPos[ByteNum + f] = CurePos;
            }
            CurePos += 1;//MkmPerTic;
            ByteNum += NextTicInFq;
        } while (CurePos < PStri->EndPos);
    }
    return 0;
}
