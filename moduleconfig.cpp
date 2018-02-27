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
        //Start Write file

        //Move X
        pari = !pari;
        if (pari){
            if (PStanda->MoveXSync(xpos2))
                return 1;
        } else {
            if (PStanda->MoveXSync(xpos1))
                return 2;
        }
        //Stop Write file

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


int ModuleConfig::Start(My8SMC1 *PStanda)
{
    void** ctx;
    ctx = new PVOID [2];
    ctx[0] = PStanda;
    ctx[1] = this;
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

int ModuleConfig::CalcInterpol()
{
    StartByteNum;
    ArrValue[];
    Freq = 100000;
    CurePos = StartPos;
    MkmPerTic =;
    //Accel
    CureSpeedTic = StartSpeedTic;
    //do while
    NextTicInFq = Freq/CureSpeedTic;
    for (int f=0;f<NextTicInFq;f++){
        ByteNum = StartByteNum + f;
        CalcPos = CurePos;
        ArrPos[ByteNum] =
    }
    CurePos += CureSpeedTic*
    CureSpeedTic +=
    //while CureSpeed < Target
}
