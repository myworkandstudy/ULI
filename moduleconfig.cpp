#include "moduleconfig.h"
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <chrono>

ULONG WINAPI SynThread(PVOID /*Context*/);


ModuleConfig::ModuleConfig()
{
    hThread = NULL;
    mystate = 1;
    TelikStop = 0;
}

ModuleConfig::~ModuleConfig()
{
    if (hThread) CloseHandle(hThread);
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
        if (d.isEmpty() || d.isNull()) {
            return 4;
        }
        //
        mkmX = d.object()["MkmFTic"].toObject()["X"].toDouble();
        mkmY = d.object()["MkmFTic"].toObject()["Y"].toDouble();
        mkmZ = d.object()["MkmFTic"].toObject()["Z"].toDouble();

        serX = d.object()["SerialDev"].toObject()["X"].toString().toStdString();
        serY = d.object()["SerialDev"].toObject()["Y"].toString().toStdString();
        serZ = d.object()["SerialDev"].toObject()["Z"].toString().toStdString();

        ConfigFilePath = d.object()["SavedData"].toObject()["ConfigFilePath"].toString().toStdString();
        if (d.object()["SavedData"].toObject()["MakeFileWriteCoef"].isNull()){
            MakeFileWriteCoef = 0.000005;
        } else{
            MakeFileWriteCoef = d.object()["SavedData"].toObject()["MakeFileWriteCoef"].toDouble();
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
        TelikLZ = d.object()["Telik"].toObject()["LZ"].toDouble();
        TelikYStep = d.object()["Telik"].toObject()["YStep"].toDouble();
        TelikZStep = d.object()["Telik"].toObject()["ZStep"].toDouble();
        if (TelikFreq = d.object()["Telik"].toObject()["Freq"].isNull()){
            TelikFreq = 100000.0;
        } else {
            TelikFreq = d.object()["Telik"].toObject()["Freq"].toDouble();
        }
        TelikFilt = d.object()["Telik"].toObject()["Filt"].toInt();
        TelikWithRet = d.object()["Telik"].toObject()["WithRet"].toInt();
        if (TelikBackSpeedX = d.object()["Telik"].toObject()["BackSpeedX"].isNull()){
            TelikBackSpeedX = 1000.0;
        } else {
            TelikBackSpeedX = d.object()["Telik"].toObject()["BackSpeedX"].toDouble();
        }
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
        //Divisor = d.object()["Speed"].toObject()["Divisor"].toInt();
    } catch (...){
        return 1;
    }
    CalcTimeLeft();
    return 0;
}

int ModuleConfig::CalcTimeLeft(void)
{
    //время движения
    double DFSize;
    TimeLeft = (double)(TelikW/SpeedX)*(double)(TelikH/TelikYStep);
    TimeLeft += (double)(TelikH/SpeedY);
    TimeLeft = TimeLeft * 1.05;
    if (TelikZStep>0){
        TimeLeft = TimeLeft * (double)(TelikLZ/TelikZStep);
    }
    //время формирования файла
    DFSize = TimeLeft * TelikFreq * 2.0;
    TimeLeft += DFSize*MakeFileWriteCoef;
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
    if (MC->TelikZStep){
        PStanda->MoveZ(MC->StartZ);
    } else {
        PStanda->MoveZ(PStanda->StateZ.CurPos);
    }
    //Ожидаем выполнения до целей
    MC->mystate = 3;
    PStanda->WaitDoneAll();
    //if (PStanda->GetInfo()) return 4;
    //Start Telik
    int xpos1 = PStanda->StateX.CurPos;
    int xpos2 = xpos1 + MC->TelikW;
    int pari = 0;
    int ystep = (int) MC->TelikYStep;
    int ystart = MC->StartY;
    int zstep = (int) MC->TelikZStep;
    int zstart = MC->StartZ;
    ULONG telikstop=0;
    //PADC->Init();
    //Sleep(50);
    int flagBreak = 0;
    for (int zpos=zstart; zpos<=MC->StartZ+MC->TelikLZ; zpos+=zstep){
        if (PStanda->MoveZSync(zpos)) break;
        //Начали считывать данные
        MC->mystate = 4;
        PADC->StartGetData();
        auto start = std::chrono::high_resolution_clock::now();
        for (int ypos=ystart; ypos<=MC->StartY+MC->TelikH; ypos+=ystep){
            flagBreak = 1;
            if (PStanda->MoveYSync(ypos)) break;//return 3;
            //Sleep(50);
            //Move X
            InterlockedExchange(&MC->TelikStringTrig, 1);
            Sleep(50);
            auto end = std::chrono::high_resolution_clock::now();
            auto dur = end - start;
            auto i_mks = std::chrono::duration_cast<std::chrono::microseconds>(dur);
            MC->FixStart(i_mks.count(),
                         PStanda->StateX.CurPos, PStanda->SpeedX,
                         PADC->CureBIdxFull/*PStanda->PrmsX.AccelT*/, PADC->CureS/*PStanda->PrmsX.DecelT*/,
                         MC->mkmX, PStanda->StateX.SDivisor, PStanda->StateY.CurPos, PStanda->StateZ.CurPos);
            Sleep(50);
            if (MC->TelikWithRet){
                if (PStanda->MoveXSync(xpos2)) break;//return 1;
                Sleep(50);
                end = std::chrono::high_resolution_clock::now();
                dur = end - start;
                i_mks = std::chrono::duration_cast<std::chrono::microseconds>(dur);
                MC->FixStop(i_mks.count(), PStanda->StateX.CurPos);
                PStanda->SetSpeedX(MC->TelikBackSpeedX);
                if (PStanda->MoveXSync(xpos1)) break;//return 2;
                //Sleep(50);
                PStanda->SetSpeedX(MC->SpeedX);
            } else {
                pari = !pari;
                if (pari){
                    if (PStanda->MoveXSync(xpos2)) break;//return 1;
                } else {
                    if (PStanda->MoveXSync(xpos1)) break;//return 2;
                }
                Sleep(50);
                end = std::chrono::high_resolution_clock::now();
                dur = end - start;
                i_mks = std::chrono::duration_cast<std::chrono::microseconds>(dur);
                MC->FixStop(i_mks.count(), PStanda->StateX.CurPos);
            }
            InterlockedExchange(&telikstop, MC->TelikStop);
            if (telikstop)
                break;
            flagBreak = 0;
        }
        Sleep(50);
        PADC->StopGetData();
        MC->mystate = 5;
        MC->WriteArrToFile2();
        MC->mystate = 6;
        MC->MakeDataFile();
        if (flagBreak || zstep==0) break;
    }
    MC->mystate = 7;
    return 0;
}


int ModuleConfig::Start(My8SMC1 *PStanda, ADCRead *PADC)
{
    if ((mystate >= 2) && (mystate != 7))
        return 1;
    mystate = 2;
    void** ctx;
    ctx = new PVOID [2];
    ctx[0] = PStanda;
    ctx[1] = this;
    ctx[2] = PADC;
    //
    CureArrIdx=0;
    //
    InterlockedExchange(&TelikStop, 0);
    hThread = CreateThread(0, 0x2000, SynThread, ctx, 0, &Tid); // Создаем и запускаем поток сбора данных
    //
    return 0;
}

int ModuleConfig::Stop(My8SMC1 *PStanda)
{
    //
    //WaitForSingleObject(hThread, 1000);
    //if (hThread) CloseHandle(hThread);
    InterlockedExchange(&TelikStop, 1);
    WaitForSingleObject(hThread, INFINITE);
    mystate = 1;
    PStanda->StopX();
    PStanda->StopY();
    PStanda->StopZ();
    myfilename.clear();
    return 0;
}

int ModuleConfig::FixStart(LONG64 in_time, int pos, int speed, float acc, float dec, double MkmPerFTic, int divisor, double y, double z)
{

    arrData[CureArrIdx].StartTime = in_time;// = ByteNum;
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

int ModuleConfig::FixStop(LONG64 in_time, int pos)
{
    arrData[CureArrIdx].EndTime = in_time; // = ByteNum;
    arrData[CureArrIdx].EndPos = pos;
    CureArrIdx++;
    if (CureArrIdx>=100000)
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
            fprintf_s(file,"%d %lld %lld %ld %ld %lf %lf %lf %d %lf %lf\n", i, arrData[i].StartTime, arrData[i].EndTime, arrData[i].StartPos, arrData[i].EndPos,
                                            arrData[i].AccT, arrData[i].DecT, arrData[i].MkmPerFTic, arrData[i].Divisor, arrData[i].y, arrData[i].z);
        }
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
        return 1;
    }
    fclose(file);
    return 0;
}

int ModuleConfig::LoadStriFromFile2() 
{
    //char fileName[100] = ; // Путь к файлу для записи
    FILE* file = fopen("data2.txt", "r");
    if (file) // если есть доступ к файлу,
    {
        CureArrIdx = 0;
        int i,read_ok, dummy;
        do {
            i = CureArrIdx;
            read_ok = fscanf_s(file,"%d %lld %lld %ld %ld %lf %lf %lf %d %lf %lf\n", &dummy, &arrData[i].StartTime, &arrData[i].EndTime, &arrData[i].StartPos, &arrData[i].EndPos,
                               &arrData[i].AccT, &arrData[i].DecT, &arrData[i].MkmPerFTic, &arrData[i].Divisor, &arrData[i].y, &arrData[i].z);
            if (read_ok>0){
                CureArrIdx++;
            }
        } while (read_ok>0);
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
        return 1;
    }
    fclose(file);
    return 0;
}

inline ULONG64 make_even(ULONG64 number)
{
    number = (number / 4) * 4;//2*number;//(number / 2) * 2;//n - n % 2;
    return number*2;
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
    ULONG64 StartByteNum = make_even((double)PStri->StartTime*((double)TelikFreq/(double)1000000));
    DWORD dwPtr = SetFilePointer( hFile, StartByteNum, NULL, FILE_BEGIN );
    if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
    {
        DWORD dwError = GetLastError() ;// Obtain the error code.
        return (int)dwError;
    }
    //Read
    ULONG64 EndByteNum = make_even((double)PStri->EndTime*((double)TelikFreq/(double)1000000));
    ULONG ByteCount = EndByteNum - StartByteNum;
    out_arrData = new UINT16 [ByteCount];
    DWORD dwTemp;
    ReadFile(hFile, out_arrData, ByteCount, &dwTemp, NULL);
    if(ByteCount != dwTemp) {
        for (int i=dwTemp;i<ByteCount;i++){
            out_arrData[i] = 0;
        }
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
    //
    LoadStriFromFile2();
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::wstring filename;
    wchar_t filename2[1000];
    FILE* file;
    if (myfilename.size()>0){
        file = _wfopen(filename.c_str(), L"a");
        filename = myfilename;
    } else {
        swprintf(filename2, 1000, L"_%d-%02d-%02d_%02d-%02d-%02d.csv", st.wYear, st.wMonth, st.wDay,
                 st.wHour, st.wMinute,st.wSecond);
        filename = ExperFileName + filename2;
        file = _wfopen(filename.c_str(), L"w"); //create empty file3
        myfilename = filename;
    }
    if (!file) {
        qWarning("Не удалось создать файл 3");
        return 1;
    }
    for (int i=0;i<CureArrIdx;i++){//цикл по строкам
        LoadDataFromFile(&arrData[i], ArrValue);
        CalcParam(&arrData[i]);
        //ULONG DataSize = arrData[i].EndByteNum - arrData[i].StartByteNum;
        //ArrPos = new ULONG[DataSize];//Make Arr
        //!CalcInterpolAndWrite(ArrValue, &arrData[i], file);
        Calc2AndWrite(ArrValue, &arrData[i], file);
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
        ULONG64 StartByteNum = make_even((double)PStri->StartTime*((double)TelikFreq/(double)1000000));
        ULONG64 EndByteNum = make_even((double)PStri->EndTime*((double)TelikFreq/(double)1000000));
        ULONG DataSize = EndByteNum - StartByteNum;
        for (UINT i=0;i<DataSize;i++){
            //              x   y  z  val
            fprintf_s(file,"%ld; %d; %d; %d\n", mArrPos[i], (int)PStri->y, (int)PStri->z, mArrValue[i]);
        }
    } else {
        std::cout << "Нет доступа к файлу!" << endl;
        return 1;
    }
    fclose(file);
    return 0;
}

int ModuleConfig::CalcInterpolAndWrite(UINT16 *ArrValue, TInterpStri*PStri, FILE* file)
{
    ULONG CurePos = PStri->StartPos;
    ULONG lastCurePos;    
    ULONG AverageSum = 0, AverageCount = 0;
    double TargetPosM;
    //---Moving2---
    TargetPosM = PStri->EndPos;
    ULONG64 StartByteNum = make_even((double)PStri->StartTime*((double)TelikFreq/(double)1000000));
    ULONG64 EndByteNum = make_even((double)PStri->EndTime*((double)TelikFreq/(double)1000000));
    for (ULONG i=StartByteNum; i<EndByteNum;i+=2){
        CurePos = (double)((double)PStri->EndPos - (double)PStri->StartPos)*(((double)i - (double)StartByteNum)/((double)EndByteNum - (double)StartByteNum)) + (double)PStri->StartPos;
        if (!TelikFilt){
            fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, ArrValue[(i-StartByteNum)/2]);
        } else {
            AverageSum += ArrValue[(i-StartByteNum)/2];
            AverageCount++;
            if ((i==StartByteNum) || (lastCurePos != CurePos)){
                fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, AverageSum/AverageCount);
                AverageSum = 0;
                AverageCount = 0;
                lastCurePos = CurePos;
            }
        }
    }
    return 0;
}

int ModuleConfig::Calc2AndWrite(UINT16 *ArrValue, TInterpStri*PStri, FILE* file)
{
    ULONG CurePos = PStri->StartPos;
    ULONG lastCurePos;
    LONG AverageSum = 0, AverageCount = 0;
    double TargetPosM;
    //---Moving2---
    TargetPosM = PStri->EndPos;
    ULONG64 StartByteNum = make_even((double)PStri->StartTime*((double)TelikFreq/(double)1000000));
    ULONG64 EndByteNum = make_even((double)PStri->EndTime*((double)TelikFreq/(double)1000000));
    CurePos = (double)PStri->StartPos;
    ULONG i=0;
    int flagFront=1;
    int fBack = PStri->StartPos > PStri->EndPos;
    while ((CurePos<PStri->EndPos && !fBack)
           || (CurePos>PStri->EndPos && fBack)){
        //StartByteNum
        if ( (i+1) > (EndByteNum - StartByteNum))
            break;
        //Pulse detected
        if ((INT16)ArrValue[i + 1] < 2500){
            if (flagFront){
                flagFront = 0;
                fBack ? CurePos-- : CurePos++;
            }
        } else {
            flagFront = 1;
        }
        if (!TelikFilt){
            fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, (INT16)ArrValue[i]);
        } else {
            AverageSum += (INT16)ArrValue[i];
            AverageCount++;
            if ((i==0) || (lastCurePos != CurePos)){
                fprintf_s(file,"%ld; %d; %d; %d\n", CurePos, (int)PStri->y, (int)PStri->z, AverageSum/AverageCount);
                AverageSum = 0;
                AverageCount = 0;
                lastCurePos = CurePos;
            }
        }
        i+=2;
    }
    return 0;
}
