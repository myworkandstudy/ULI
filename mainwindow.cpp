#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "USMCDLL.h"
#include "My8SMC1.h"
#include "moduleconfig.h"
#include "adcread.h"
#include <QDebug>
#include <QTimer>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QValueAxis>

#include <QFileDialog>
#include <QMessageBox>

QT_CHARTS_USE_NAMESPACE

USMC_Devices DVS;
DWORD Dev;
My8SMC1 Standa;
int xG = 0;
ModuleConfig MConf;
ADCRead ADC;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    series0(new QLineSeries()),
    series1(new QLineSeries())
{

    //------------------------------------
    ui->setupUi(this);
    //
    ui->label_10->hide();
    ui->label_11->hide();
    ui->label_12->hide();
    ui->label_13->hide();
    ui->label_14->hide();
    ui->label_15->hide();


    //![1]
        //QLineSeries *series0 = new QLineSeries();
        //QLineSeries *series1 = new QLineSeries();
    //![1]

    //![2]
        *series0 << QPointF(1, 5) << QPointF(3, 7) << QPointF(7, 6) << QPointF(9, 7) << QPointF(12, 6)
                 << QPointF(16, 7) << QPointF(18, 5);
        *series1 << QPointF(1, 3) << QPointF(3, 4) << QPointF(7, 3) << QPointF(8, 2) << QPointF(12, 3)
                 << QPointF(16, 4) << QPointF(18, 3);
    //![2]

    //![3]
        //QAreaSeries *series = new QAreaSeries(series0, series1);
        //series->setName("Batman");
        //QPen pen(0x059605);
        //pen.setWidth(3);
        //series->setPen(pen);
        series0->setName("По оси X");
        series1->setName("По оси Y");

        //QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
        //gradient.setColorAt(0.0, 0x3cc63c);
        //gradient.setColorAt(1.0, 0x26f626);
        //gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        //series->setBrush(gradient);
    //![3]

    //![4]
        chart = new QChart();
        chart->addSeries(series0);
        chart->addSeries(series1);
        chart->setTitle("Текущее измерение");
        chart->createDefaultAxes();
        chart->axisX()->setRange(0, 32767);
        chart->axisY()->setRange(0, 65535);
    //![4]

    //![5]
        chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setMinimumSize(640, 480);
    //![5]

    //![6]
        //chartView = new QChartView(createLineChart());
        ui->formLayout->addWidget(chartView);

        //
        //!InitE14();
        //ADC.Init();

        //
        tmr = new QTimer(this); // Создаем объект класса QTimer и передаем адрес переменной
        tmr->setInterval(100); // Задаем интервал таймера
        connect(tmr, SIGNAL(timeout()), this, SLOT(updateTime())); // Подключаем сигнал таймера к нашему слоту
        tmr->start(); // Запускаем таймер
        //
        MConf.Load();
        ui->spinBox_4->setValue(Standa.ManSpeed);
        ui->lineEdit_4->setText(QString::fromStdString(MConf.ConfigFilePath));
        //ui->statusBar->addWidget();
        lstatus=new QLabel(this);
        statusBar()->addWidget(lstatus);
}

MainWindow::~MainWindow()
{
    Standa.Flash();
    delete ui;
}

QChart *MainWindow::createLineChart() const
{
    QChart *chart = new QChart();
    chart->setTitle("Line chart");

    QString name("Series ");
    int nameIndex = 0;
    for (const DataList &list : m_dataTable) {
        QLineSeries *series = new QLineSeries(chart);
        for (const Data &data : list)
            series->append(data.first);
        series->setName(name + QString::number(nameIndex));
        nameIndex++;
        chart->addSeries(series);
    }
    chart->createDefaultAxes();

    return chart;
}


void MainWindow::on_pushButton_7_clicked()
{
    if (QMessageBox::warning(0,tr("Подтверждение"),tr("Подтвердите действие"),QMessageBox::Ok|QMessageBox::Cancel)==QMessageBox::Ok){
        Standa.HomeX();
    }
}

void MainWindow::on_pushButton_8_clicked()
{
    if (QMessageBox::warning(0,tr("Подтверждение"),tr("Подтвердите действие"),QMessageBox::Ok|QMessageBox::Cancel)==QMessageBox::Ok){
        Standa.HomeY();
    }
}

void MainWindow::on_pushButton_9_clicked()
{
    if (QMessageBox::warning(0,tr("Подтверждение"),tr("Подтвердите действие"),QMessageBox::Ok|QMessageBox::Cancel)==QMessageBox::Ok){
        Standa.HomeZ();
    }
}

void MainWindow::on_pushButton_10_clicked()
{
    qreal x = ui->lineEdit_3->text().toDouble();
    qreal y = ui->lineEdit_5->text().toDouble();
    series0->append(x, y);
}

void MainWindow::on_pushButton_11_clicked()
{
    chart->scroll(ui->lineEdit_6->text().toDouble(), 0);
}

void MainWindow::on_toolButton_clicked()
{
    ui->lineEdit_4->setText(QFileDialog::getOpenFileName(0, "Open Dialog", "", "*.cfg *.config *.json"));
    MConf.ConfigFilePath = ui->lineEdit_4->text().toStdString();
    MConf.Save();
}

void MainWindow::updateTime()
{
    Standa.GetInfo();
    //
    ui->label_10->setVisible(Standa.StateX.Trailer1);
    ui->label_11->setVisible(Standa.StateX.Trailer2);
    ui->label_12->setVisible(Standa.StateY.Trailer1);
    ui->label_13->setVisible(Standa.StateY.Trailer2);
    ui->label_14->setVisible(Standa.StateZ.Trailer1);
    ui->label_15->setVisible(Standa.StateZ.Trailer2);
    //
    ui->pushButton_18->setChecked(!Standa.ModeX.ResetD);
    ui->pushButton_19->setChecked(!Standa.ModeY.ResetD);
    ui->pushButton_20->setChecked(!Standa.ModeZ.ResetD);
    //
    ui->label_3->setText(QString::number(Standa.StateX.CurPos));
    ui->label_4->setText(QString::number(Standa.StateY.CurPos));
    ui->label_5->setText(QString::number(Standa.StateZ.CurPos));
    //
    ULONG val = ADC.GetValue0();
    ui->label_6->setText(QString::number(val));
    //
    if (MConf.mystate == 2) lstatus->setText("Текущее состояние программы: Выставляем начальные позиции");
    if (MConf.mystate == 3) lstatus->setText("Текущее состояние программы: Ожидаем выставления начальных позиций");
    if (MConf.mystate == 4) lstatus->setText("Текущее состояние программы: Перемещение и сохранение данных");
    if (MConf.mystate == 5) lstatus->setText("Текущее состояние программы: Сохраняем строки в файл 2");
    if (MConf.mystate == 6) lstatus->setText("Текущее состояние программы: Сохраняем результат в файл 3");
    if (MConf.mystate == 7) lstatus->setText("Текущее состояние программы: Готово");
    //
    xG+=100;
    if (xG>=32767) {
        xG=0;
        series0->clear();
    }
    //!series0->append(xG, ADC.GetValue0());
    //!
}

void MainWindow::updateTimeDeb()
{
    arrSidx<10000-5 ? arrSidx++ : 0;
    arrS[arrSidx] = ADC.CureS;
    arrSidx++;
    arrS[arrSidx] = ADC.CureS - lastArrS;
    lastArrS = ADC.CureS;
    arrSidx++;
    arrS[arrSidx] = 1;
}

void MainWindow::on_pushButton_3_clicked()
{
    ui->spinBox->setValue(ui->label_3->text().toInt());
}

void MainWindow::on_pushButton_4_clicked()
{
    ui->spinBox_2->setValue(ui->label_4->text().toInt());
}

void MainWindow::on_pushButton_5_clicked()
{
    ui->spinBox_3->setValue(ui->label_5->text().toInt());
}

void MainWindow::on_pushButton_clicked()
{
    Standa.SetSpeedXYZ(Standa.ManSpeed,Standa.ManSpeed,Standa.ManSpeed);
    Standa.MoveX(ui->spinBox->value());
    Standa.MoveY(ui->spinBox_2->value());
    Standa.MoveZ(ui->spinBox_3->value());
}

void MainWindow::on_pushButton_12_clicked()
{
    lstatus->setText("Текущее состояние программы: Выполняется загрузка конфигурации");
    MConf.ExperFileName = ui->lineEdit_2->text().toStdWString();
    //C:/Users/102324/Documents/build-Standa1-Desktop_Qt_5_9_2_MSVC2017_64bit-D
    //------------------------------------
    //Load Config from JSON file
    if (MConf.Load()){
        QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не удалось загрузить файлы конфига.\n Нарушена структура или нет файлов.)"));
        return;
    }
    //Set params to Standa Driver
    //Standa.SetAccXYZ(MConf.AccX, MConf.AccY, MConf.AccZ);
    //
    Standa.SetSpeedXYZ(MConf.SpeedX, MConf.SpeedY, MConf.SpeedZ);
    //
    //Standa.HomeX();
    //Standa.HomeY();
    Standa.GetPrmsAll();
    Standa.GetInfo();
    //
    lstatus->setText("Текущее состояние программы: Выполняется задание");
    MConf.Start(&Standa, &ADC);
}

void MainWindow::on_pushButton_13_clicked()
{
    lstatus->setText("Текущее состояние программы: Остановка процесса");
    MConf.Stop(&Standa);
    lstatus->setText("Текущее состояние программы: Процесс остановлен");
}

void MainWindow::on_pushButton_14_clicked()
{
    ADC.Init();
}

void MainWindow::on_pushButton_15_clicked()
{
    ADC.StartGetData();
    //
    for (int i=0;i<10000;i++) {
        arrS[i] = 0;
    }
    arrSidx = 0;
    lastArrS = 0;
    //
    tmrDeb = new QTimer(this); // Создаем объект класса QTimer и передаем адрес переменной
    tmrDeb->setInterval(100); // Задаем интервал таймера
    connect(tmrDeb, SIGNAL(timeout()), this, SLOT(updateTimeDeb())); // Подключаем сигнал таймера к нашему слоту
    tmrDeb->start(); // Запускаем таймер
}

void MainWindow::on_pushButton_16_clicked()
{
    ADC.StopGetData();
    tmrDeb->stop();

    MConf.WriteArrToFile2();

    //HANDLE hFile2;
    //Write to file
    //hFile2 = CreateFileA("data2.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
    //if (hFile2 == INVALID_HANDLE_VALUE) { return ; }
    //WriteFile(hFile2, tmp1, halfbuffer*pointsize, &BytesWritten, NULL);
}

void MainWindow::on_pushButton_17_clicked()
{
    //ADC.FixS();
    //ui->label_10->show();
    int ret = QMessageBox::information(0,tr("information"),tr("No tabs for adding"),QMessageBox::Ok|QMessageBox::Cancel);
    ret = 0;
}

void MainWindow::on_pushButton_18_clicked()
{
    //Standa.StateX.Power
    Standa.ModeX.ResetD = !Standa.ModeX.ResetD;
    Standa.SetMode(Standa.DevX, Standa.ModeX);
}

void MainWindow::on_pushButton_19_clicked()
{
    Standa.ModeY.ResetD = !Standa.ModeY.ResetD;
    Standa.SetMode(Standa.DevY, Standa.ModeY);
}

void MainWindow::on_pushButton_20_clicked()
{
    Standa.ModeZ.ResetD = !Standa.ModeZ.ResetD;
    Standa.SetMode(Standa.DevZ, Standa.ModeZ);
}

void MainWindow::on_pushButton_6_clicked()
{

}

void MainWindow::on_verticalSlider_2_valueChanged(int value)
{
    ui->spinBox_4->setValue(value);
}

void MainWindow::on_spinBox_4_valueChanged(int arg1)
{
    Standa.ManSpeed = arg1;
    if (arg1 <= ui->verticalSlider_2->maximum()){
        ui->verticalSlider_2->setValue(arg1);
    }
}
