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
    mystate = 1;
}

int ModuleConfig::Load(void)
{
    try{
        QString val;
        QFile file;
        file.setFileName(SETTINGSFILENAME);//C:\\Users\\102324\\source\\repos\\ConsoleApplication1\\ConsoleApplication1\\settings.json");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        val = file.readAll();
        file.close();
        QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
        //
        mkmX = d.object()["MkmFTic"].toObject()["X"].toDouble();
        mkmY = d.object()["MkmFTic"].toObject()["Y"].toDouble();
        mkmZ = d.object()["MkmFTic"].toObject()["Z"].toDouble();

        ConfigFilePath = d.object()["SavedData"].toObject()["ConfigFilePath"].toString().toStdString();
        if (d.isEmpty() || d.isNull()) {
            return 4;
        }

        //QString val;
        //QFile file;
        file.setFileName(QString::fromStdString(ConfigFilePath));//C:\\Users\\102324\\source\\repos\\ConsoleApplication1\\ConsoleApplication1\\config1.json");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        val = file.readAll();
        file.close();
        d = QJsonDocument::fromJson(val.toUtf8());
        if (d.isEmpty() || d.isNull()) {
            return 4;
        }
        //qWarning() << d.object()["Telik"].toObject()["W"].toInt();
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

int ModuleConfig::Save(void)
{
    try{
        QString val;
        QFile jsonFile;
        jsonFile.setFileName(SETTINGSFILENAME);
        jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
        val = jsonFile.readAll();
        jsonFile.close();
        QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
        if (d.isEmpty() || d.isNull()) {
            return 4;
        }
        jsonFile.open(QFile::WriteOnly);
        d.object()["SavedData"].toObject()["ConfigFilePath"] = QString::fromStdString(ConfigFilePath);
        QJsonObject root_obj = d.object();
        QJsonObject o = root_obj["SavedData"].toObject();
        o["ConfigFilePath"] = QString::fromStdString(ConfigFilePath);
        root_obj["SavedData"] = o;
        d.setObject(root_obj);
        jsonFile.write(d.toJson());
        jsonFile.close();
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
    MC->mystate = 2;
    //Move to start position
    PStanda->MoveX(MC->StartX);
    PStanda->MoveY(MC->StartY);
    PStanda->MoveZ(PStanda->StateZ.CurPos);
    //Ожидаем выполнения до целей
    MC->mystate = 3;
    PStanda->WaitDoneAll();
    //if (PStanda->GetInfo()) return 4;
    //Start Telik
    int xpos1 = PStanda->StateX.CurPos;
    int xpos2 = xpos1 + MC->TelikW;
    int pari = 0;
    int ystep = (int) MC->TelikYStep;
    int ystart = PStanda->StateY.CurPos + ystep;
    //PADC->Init();
    //Sleep(50);
    //Начали считывать данные
    MC->mystate = 4;
    PADC->StartGetData();
    for (int ypos=ystart; ypos<=MC->StartY+MC->TelikH; ypos+=ystep){
        //Move X
        MC->FixStart(PADC->GetCureByteNum(), PStanda->StateX.CurPos, PStanda->SpeedX, PStanda->PrmsX.AccelT, PStanda->PrmsX.DecelT, MC->mkmX, PStanda->StateX.SDivisor, ypos, PStanda->StateZ.CurPos);
        pari = !pari;
        if (pari){
            //PStanda->StateX.CurPos = xpos2;
            //Sleep(50);
            if (PStanda->MoveXSync(xpos2))
                return 1;
        } else {
            //PStanda->StateX.CurPos = xpos1;
            //Sleep(60);
            if (PStanda->MoveXSync(xpos1))
                return 2;
        }
        MC->FixStop(PADC->GetCureByteNum(), PStanda->StateX.CurPos);
        //
        //PStanda->StateY.CurPos = ypos;
        //Sleep(5);
        if (PStanda->MoveYSync(ypos))
            return 3;
    }
    PADC->StopGetData();
    MC->mystate = 5;
    MC->WriteArrToFile2();
    MC->mystate = 6;
    MC->MakeDataFile();
    MC->mystate = 7;
    return 0;
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
    FILE* file = fopen("data2.txt", "r");
    if (file) // если есть доступ к файлу,
    {
        CureArrIdx = 0;
        int i,read_ok, dummy;
        do {
            i = CureArrIdx;
            read_ok = fscanf_s(file,"%d %lld %lld %ld %ld %lf %lf %lf %d %lf %lf\n", &dummy, &arrData[i].StartByteNum, &arrData[i].EndByteNum, &arrData[i].StartPos, &arrData[i].EndPos,
                               &arrData[i].AccT, &arrData[i].DecT, &arrData[i].MkmPerFTic, &arrData[i].Divisor, &arrData[i].y, &arrData[i].z);
            if (read_ok>0){
                CureArrIdx++;
            }
        } while (read_ok>0);
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
    }
    fclose(file);
    return 0;
}

int ModuleConfig::LoadDataFromFile(TInterpStri *PStri, UINT16 *&out_arrData)
{
    //
    HANDLE hFile;
    //Open
    hFile = CreateFileA("data.dat", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    //if (hFile == INVALID_HANDLE_VALUE) { M_FAIL("CreateFile(data.dat)", GetLastError()); End(); return 11; }
    //else M_OK("CreateFile(data.dat)", endl);
    if (hFile == INVALID_HANDLE_VALUE){
        DWORD dwError = GetLastError() ;// Obtain the error code.
        return (int)dwError;
    }
    //Seek
    DWORD dwPtr = SetFilePointer( hFile, PStri->StartByteNum, NULL, FILE_BEGIN );
    if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
    {
        DWORD dwError = GetLastError() ;// Obtain the error code.
        return (int)dwError;
    }
    //Read
    ULONG ByteCount = PStri->EndByteNum - PStri->StartByteNum;
    out_arrData = new UINT16 [ByteCount];
    DWORD dwTemp;
    ReadFile(hFile, out_arrData, ByteCount, &dwTemp, NULL);
    if(ByteCount != dwTemp) {
        CloseHandle(hFile);
        return 2;
    }
    CloseHandle(hFile);
    //out_arrData = out_arr;
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
    //ULONG *ArrPos;    //TInterpStri Stri;
    WriteArrToFile2();
    //LoadStriFromFile2();
    std::wstring filename;
    filename = ExperFileName + L"_data3.csv";
    FILE* file = _wfopen(filename.c_str(), L"w"); //create empty file3
    if (!file) {
        qWarning("Не удалось создать файл 3");
        return 1;
    }
    for (int i=0;i<CureArrIdx;i++){//цикл по строкам
        LoadDataFromFile(&arrData[i], ArrValue);
        CalcParam(&arrData[i]);
        //ULONG DataSize = arrData[i].EndByteNum - arrData[i].StartByteNum;
        //ArrPos = new ULONG[DataSize];//Make Arr
        CalcInterpolAndWrite(ArrValue, &arrData[i], file);
        //WriteToFile3(&arrData[i], ArrPos, ArrValue);
        //delete [] ArrPos;//Del Arr
        delete [] ArrValue;
    }
    fclose(file);
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
            fprintf_s(file,"%ld; %d; %d; %d\n", mArrPos[i], (int)PStri->y, (int)PStri->z, mArrValue[i]);
        }
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
    }
    fclose(file);
    return 0;
}

int ModuleConfig::CalcInterpolAndWrite(UINT16 *ArrValue, TInterpStri*PStri, FILE* file)
{
    ULONG CurePos = PStri->StartPos;
    int CureSpeedTic = (PStri->TargetSpeedTic > MaxSpeedFTic ? MaxSpeedFTic : PStri->TargetSpeedTic);//(PStri->TargetSpeedTic < MinSpeedFTic*PStri->Divisor ? PStri->TargetSpeedTic : MinSpeedFTic*PStri->Divisor);
    int ByteNum = 0;
    ULONG NextTicInFq;
    double TargetPosM;
    //---Moving2---
    TargetPosM = PStri->EndPos;
    for (ULONG i=PStri->StartByteNum; i<PStri->EndByteNum;i+=2){
        CurePos = (double)((double)PStri->EndPos - (double)PStri->StartPos)*(((double)i - (double)PStri->StartByteNum)/((double)PStri->EndByteNum - (double)PStri->StartByteNum)) + (double)PStri->StartPos;
        fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, ArrValue[(i-PStri->StartByteNum)/2]);
    }
    return 0;

    //!дальнейшая часть не работает корректно
    //---Accel---
    if (PStri->TargetSpeedTic > MinSpeedFTic){
        do{
            NextTicInFq = Freq/CureSpeedTic;
            for (UINT f=0;f<NextTicInFq;f++){
                fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, ArrValue[ByteNum + f]);
                //ArrPos[ByteNum + f] = CurePos;
            }
            if (PStri->StartPos < PStri->EndPos){
                CurePos += 1;//MkmPerTic;
            } else {
                CurePos -= 1;
            }
            ByteNum += NextTicInFq;
            //CureSpeedTic += 1;
            CureSpeedTic = (double)((((double)0-Cacc)-(Aacc*((double)ByteNum/(double)Freq)*1000))/Bacc)*(double)PStri->Divisor;
        } while (CureSpeedTic < PStri->TargetSpeedTic);
        TargetPosM = PStri->EndPos - (PStri->DecT - (((0-Cdec)-(Bdec*PStri->TargetSpeedTic))/Adec))*MkmPerMs;//!это не точная позиция замедления, надо по fq
    } else {
        TargetPosM = PStri->EndPos;
    }
    CureSpeedTic = PStri->TargetSpeedTic;
    //---Moving---
    do{
        NextTicInFq = Freq/CureSpeedTic;
        for (UINT f=0;f<NextTicInFq;f++){
            fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, ArrValue[ByteNum + f]);
        }
        ByteNum += NextTicInFq;
        if (PStri->StartPos < PStri->EndPos){
            CurePos += 1;//MkmPerTic;
            if (CurePos > TargetPosM){
                break;
            }
        } else {
            CurePos -= 1;
            if (CurePos < TargetPosM){
                break;
            }
        }
    } while (1);
    //---Decel---
    if (PStri->TargetSpeedTic > MinSpeedFTic*PStri->Divisor){
        do{
            NextTicInFq = Freq/CureSpeedTic;
            for (UINT f=0;f<NextTicInFq;f++){
                fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, ArrValue[ByteNum + f]);
            }
            ByteNum += NextTicInFq;
            if (PStri->StartPos < PStri->EndPos){
                CurePos += 1;//MkmPerTic;
                if (CurePos > PStri->EndPos){
                    break;
                }
            } else {
                CurePos -= 1;
                if (CurePos < PStri->EndPos){
                    break;
                }
            }
        } while (1);
    }
    return 0;
}
