#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "USMCDLL.h"
#include "My8SMC1.h"
#include "moduleconfig.h"
#include "adcread.h"
#include <QDebug>
#include <QTimer>
#include <QTime>

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
    series1(new QLineSeries()),
    m_axisX(new QValueAxis()),
    m_axisY(new QValueAxis()),
    time1(new QTime())
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
    ui->DebugButton1->hide();


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
        series0->setName("Значение, мВ");
        //series1->setName("Канал 2, мВ");
        //series1->setName("Значение, мВ");

        //QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
        //gradient.setColorAt(0.0, 0x3cc63c);
        //gradient.setColorAt(1.0, 0x26f626);
        //gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        //series->setBrush(gradient);
    //![3]

    //![4]
        chart = new QChart();
        chart->addSeries(series0);
        //chart->addSeries(series1);
        chart->setTitle("Текущее измерение");
        chart->createDefaultAxes();
        //chart->axisX()->setRange(0, 10000);
        chart->setAxisX(m_axisX,series0);
        //chart->setAxisX(m_axisX,series1);
        m_axisX->setRange(0, 10000);
        m_axisX->setTitleText("Время, мс");
        //chart->axisY()->setRange(0, 2000);
        chart->setAxisY(m_axisY,series0);
        //chart->setAxisY(m_axisY,series1);
        m_axisY->setRange(0, 2000);
    //![4]

    //![5]
        chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setMinimumSize(800, 600);
    //![5]

    //![6]
        //chartView = new QChartView(createLineChart());
        ui->formLayout->addWidget(chartView);

        //
        tmr = new QTimer(this); // Создаем объект класса QTimer и передаем адрес переменной
        tmr->setInterval(100); // Задаем интервал таймера
        connect(tmr, SIGNAL(timeout()), this, SLOT(updateTime())); // Подключаем сигнал таймера к нашему слоту
        tmr->start(); // Запускаем таймер
        //
        tmrG = new QTimer(this);
        tmrG->setInterval(10);
        connect(tmrG, SIGNAL(timeout()), this, SLOT(updateTimeGraph()));
        tmrG->start();
        //
        if (MConf.Load()){
            QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не удалось загрузить файлы конфига.\n Нарушена структура или нет файлов."));
            return;
        }
        ADC.sett_dRate_kHz = MConf.TelikFreq/(double)1000.0;
        if (ADC.Init()) {
            QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не удалось инициализировать L-Card.\nВставьте устройство в порт и перезапустите программу."));
        }
        if (Standa.Init(MConf.serX,MConf.serY,MConf.serZ))
            QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не удалось инициализировать драйвер шагового двигателя 8SMC1.\nВставьте устройство в порт и перезапустите программу."));
        ui->spinBox_4->setValue(Standa.ManSpeed);
        ui->lineEdit_4->setText(QString::fromStdString(MConf.ConfigFilePath));
        //ui->statusBar->addWidget();
        lstatus=new QLabel(this);
        statusBar()->addWidget(lstatus);
        //debug
        ui->DebugButton1->setVisible(0);
        ui->label_ADC2->setVisible(0);
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


void MainWindow::on_HomeXButton_clicked()
{
    if (QMessageBox::warning(0,tr("Подтверждение"),tr("Подтвердите действие"),QMessageBox::Ok|QMessageBox::Cancel)==QMessageBox::Ok){
        if (Standa.HomeX()){
            Standa.StopX();
            QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не удалось найти 0.\n Повторите."));
        }

    }
}

void MainWindow::on_HomeYButton_clicked()
{
    if (QMessageBox::warning(0,tr("Подтверждение"),tr("Подтвердите действие"),QMessageBox::Ok|QMessageBox::Cancel)==QMessageBox::Ok){
        Standa.HomeY();
    }
}

void MainWindow::on_HomeZButton_clicked()
{
    if (QMessageBox::warning(0,tr("Подтверждение"),tr("Подтвердите действие"),QMessageBox::Ok|QMessageBox::Cancel)==QMessageBox::Ok){
        Standa.HomeZ();
    }
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
    ui->PwrXButton->setChecked(!Standa.ModeX.ResetD);
    ui->PwrYButton->setChecked(!Standa.ModeY.ResetD);
    ui->PwrZButton->setChecked(!Standa.ModeZ.ResetD);
    //
    ui->label_3->setText(QString::number(Standa.StateX.CurPos));
    ui->label_4->setText(QString::number(Standa.StateY.CurPos));
    ui->label_5->setText(QString::number(Standa.StateZ.CurPos));
    //
    ULONG val = ADC.GetValue0();
    ULONG val2 = ADC.GetValue1();
    ui->label_6->setText(QString::number((LONG)val));
    ui->label_ADC2->setText(QString::number((LONG)val2));
    //
    if (MConf.mystate == 2) lstatus->setText("Текущее состояние программы: Выставляем начальные позиции");
    if (MConf.mystate == 3) lstatus->setText("Текущее состояние программы: Ожидаем выставления начальных позиций");
    if (MConf.mystate == 4) lstatus->setText("Текущее состояние программы: Перемещение и сохранение данных");
    if (MConf.mystate == 5) lstatus->setText("Текущее состояние программы: Сохраняем строки в файл 2");
    if (MConf.mystate == 6) lstatus->setText("Текущее состояние программы: Сохраняем результат в файл 3");
    if (MConf.mystate == 7){
        lstatus->setText("Текущее состояние программы: Готово");
        //
        ui->PwrXButton->setEnabled(1);
        ui->PwrYButton->setEnabled(1);
        ui->PwrZButton->setEnabled(1);
        ui->HomeXButton->setEnabled(1);
        ui->HomeYButton->setEnabled(1);
        ui->HomeZButton->setEnabled(1);
        ui->MoveAllButton->setEnabled(1);
        ui->spinBox_4->setEnabled(1);
        ui->verticalSlider_2->setEnabled(1);
        ui->StartButton->setEnabled(1);
    }
    //
    if ((MConf.mystate>1) && (MConf.mystate != 7)){
        double perc = (double)100.0 / (MConf.TimeLeft*(double)1000.0) * (double)time1->elapsed();
        if (perc > 100) perc = 100;
        double minleft = (double)(MConf.TimeLeft*(double)1000.0 - (double)time1->elapsed())/(double)60000.0;
        if (minleft < 1) {
            ui->TextProgressBar_label->setText("Выполнено " + QString::number(perc,0,1)
                                               + "% (Осталось меньше минуты)");
        } else {
            ui->TextProgressBar_label->setText("Выполнено " + QString::number(perc,0,1)
                                               + "% (Осталось " + QString::number(minleft,0,1) + " мин)");
        }
        ui->progressBar->setValue(perc);
    } else if (MConf.mystate == 7){
        ui->progressBar->setValue(100);
        ui->TextProgressBar_label->setText("Готово");
    } else {
        ui->progressBar->setValue(0);
        double TimeLeft_minutes = MConf.TimeLeft / (double)60.0;
        ui->TextProgressBar_label->setText("Требуется минут: " + QString::number(TimeLeft_minutes, 0, 1));
    }
}

void MainWindow::updateTimeGraph()
{
    //
    xG+=10;
    if ((MConf.TelikStringTrig && MConf.mystate>1) || (xG >= m_axisX->max() && MConf.mystate==1)) {
        MConf.TelikStringTrig = 0;
        xG=0;
        series0->clear();
        //series1->clear();
    }
    series0->append(xG, (INT16)ADC.GetValue0());
    //series1->append(xG, (INT16)ADC.GetValue1());
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

void MainWindow::on_CureCoordXButton_clicked()
{
    ui->spinBox->setValue(ui->label_3->text().toInt());
}

void MainWindow::on_CureCoordYButton_clicked()
{
    ui->spinBox_2->setValue(ui->label_4->text().toInt());
}

void MainWindow::on_CureCoordZButton_clicked()
{
    ui->spinBox_3->setValue(ui->label_5->text().toInt());
}

void MainWindow::on_StartButton_clicked()
{
    time1->start();
    //!ui->StartButton->setVisible(0);
    lstatus->setText("Текущее состояние программы: Выполняется загрузка конфигурации");
    MConf.ExperFileName = ui->lineEdit_2->text().toStdWString();
    //------------------------------------
    //Load Config from JSON file
    if (MConf.Load()){
        QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не удалось загрузить файлы конфига.\n Нарушена структура или нет файлов."));
        return;
    }
    //Set params to Standa Driver (!Не работает!)
    //Standa.SetAccXYZ(MConf.AccX, MConf.AccY, MConf.AccZ);
    //
    Standa.SetSpeedXYZ(MConf.SpeedX, MConf.SpeedY, MConf.SpeedZ);
    //
    Standa.GetPrmsAll();
    Standa.GetInfo();
    //
    lstatus->setText("Текущее состояние программы: Выполняется задание");
    double maxrange = (double)MConf.TelikW/(MConf.SpeedX/Standa.StateX.SDivisor)*(double)1000.0;
    m_axisX->setRange(0, maxrange);
    MConf.Start(&Standa, &ADC);
    //
    ui->PwrXButton->setDisabled(1);
    ui->PwrYButton->setDisabled(1);
    ui->PwrZButton->setDisabled(1);
    ui->HomeXButton->setDisabled(1);
    ui->HomeYButton->setDisabled(1);
    ui->HomeZButton->setDisabled(1);
    ui->MoveAllButton->setDisabled(1);
    ui->spinBox_4->setDisabled(1);
    ui->verticalSlider_2->setDisabled(1);
    ui->StartButton->setDisabled(1);
}

void MainWindow::on_StopButton_clicked()
{
    lstatus->setText("Текущее состояние программы: Остановка процесса");
    MConf.Stop(&Standa);
    lstatus->setText("Текущее состояние программы: Процесс остановлен");
    //
    ui->PwrXButton->setEnabled(1);
    ui->PwrYButton->setEnabled(1);
    ui->PwrZButton->setEnabled(1);
    ui->HomeXButton->setEnabled(1);
    ui->HomeYButton->setEnabled(1);
    ui->HomeZButton->setEnabled(1);
    ui->MoveAllButton->setEnabled(1);
    ui->spinBox_4->setEnabled(1);
    ui->verticalSlider_2->setEnabled(1);
    ui->StartButton->setEnabled(1);
}

void MainWindow::on_PwrXButton_clicked()
{
    //Standa.StateX.Power
    Standa.ModeX.ResetD = !Standa.ModeX.ResetD;
    Standa.SetMode(Standa.DevX, Standa.ModeX);
}

void MainWindow::on_PwrYButton_clicked()
{
    Standa.ModeY.ResetD = !Standa.ModeY.ResetD;
    Standa.SetMode(Standa.DevY, Standa.ModeY);
}

void MainWindow::on_PwrZButton_clicked()
{
    Standa.ModeZ.ResetD = !Standa.ModeZ.ResetD;
    Standa.SetMode(Standa.DevZ, Standa.ModeZ);
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


void MainWindow::on_lineEdit_7_textChanged(const QString &arg1)
{
    if (arg1.toDouble() > m_axisY->min()){
        chart->axisY()->setMax(arg1.toDouble());
    }
}

void MainWindow::on_lineEdit_8_textChanged(const QString &arg1)
{
    if (arg1.toDouble() < m_axisY->max()){
        chart->axisY()->setMin(arg1.toDouble());
    }
}

void MainWindow::on_lineEdit_9_textChanged(const QString &arg1)
{
    m_axisX->setRange(0, arg1.toDouble());
}

void MainWindow::on_MoveAllButton_clicked()
{
    Standa.SetSpeedXYZ(Standa.ManSpeed,Standa.ManSpeed,Standa.ManSpeed);
    Standa.MoveX(ui->spinBox->value());
    Standa.MoveY(ui->spinBox_2->value());
    Standa.MoveZ(ui->spinBox_3->value());
}

void MainWindow::on_pushButton_14_clicked()
{
    on_StopButton_clicked();
}

void MainWindow::on_DebugButton1_clicked()
{
    if (QMessageBox::warning(0,tr("Подтверждение"),tr("Подтвердите действие"),QMessageBox::Ok|QMessageBox::Cancel)==QMessageBox::Ok){
        MConf.MakeDataFile();
    }
}
