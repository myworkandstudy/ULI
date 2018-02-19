#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "USMCDLL.h"
#include "../../source/repos/ConsoleApplication1/ConsoleApplication1/My8SMC1.h"
#include <QDebug>
#include <QTimer>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QValueAxis>

#include <QFileDialog>

QT_CHARTS_USE_NAMESPACE

USMC_Devices DVS;
DWORD Dev;
My8SMC1 Standa;
int xG = 0;

extern int InitE14(void);
extern ULONG GetVal(void);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    series0(new QLineSeries()),
    series1(new QLineSeries())
{
    ui->setupUi(this);

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

        //
        tmr = new QTimer(this); // Создаем объект класса QTimer и передаем адрес переменной
        tmr->setInterval(100); // Задаем интервал таймера
        connect(tmr, SIGNAL(timeout()), this, SLOT(updateTime())); // Подключаем сигнал таймера к нашему слоту
        tmr->start(); // Запускаем таймер
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

void MainWindow::on_pushButton_2_clicked()
{
    Standa.MoveZ(ui->verticalSlider->value());
}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
    ui->lineEdit->setText(QString::number(value));
}

void MainWindow::on_pushButton_7_clicked()
{
    Standa.HomeX();
}

void MainWindow::on_pushButton_8_clicked()
{
    Standa.HomeY();
}

void MainWindow::on_pushButton_9_clicked()
{
    Standa.HomeZ();
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
    ui->lineEdit_4->setText(QFileDialog::getOpenFileName(0, "Open Dialog", "", "*.cfg *.config"));
}

void MainWindow::updateTime()
{
    Standa.GetInfo();
    //
    ui->label_3->setText(QString::number(Standa.StateX.CurPos));
    ui->label_4->setText(QString::number(Standa.StateY.CurPos));
    ui->label_5->setText(QString::number(Standa.StateZ.CurPos));
    //
    //!ui->label_6->setText(QString::number(GetVal()));
    //
    xG+=100;
    if (xG>=32767) {
        xG=0;
        series0->clear();
    }
    //!series0->append(xG, GetVal());
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
    Standa.MoveX(ui->spinBox->value());
    Standa.MoveY(ui->spinBox_2->value());
    Standa.MoveZ(ui->spinBox_3->value());
}
