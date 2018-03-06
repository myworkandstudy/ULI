#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QLineSeries>
#include <QLabel>

QT_CHARTS_BEGIN_NAMESPACE
class QChartView;
class QChart;
QT_CHARTS_END_NAMESPACE

typedef QPair<QPointF, QString> Data;
typedef QList<Data> DataList;
typedef QList<DataList> DataTable;

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QLabel* lstatus;
private slots:

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_10_clicked();

    void on_pushButton_11_clicked();

    void on_toolButton_clicked();

    void updateTime();
    void updateTimeDeb();
    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_clicked();

    void on_pushButton_12_clicked();

    void on_pushButton_13_clicked();

    void on_pushButton_14_clicked();

    void on_pushButton_15_clicked();

    void on_pushButton_16_clicked();

    void on_pushButton_17_clicked();

    void on_pushButton_19_clicked();

    void on_pushButton_18_clicked();

    void on_pushButton_20_clicked();

    void on_pushButton_6_clicked();

    void on_verticalSlider_2_valueChanged(int value);

    void on_spinBox_4_valueChanged(int arg1);

private:
    DataTable m_dataTable;
    Ui::MainWindow *ui;
    QLineSeries *series0;
    QLineSeries *series1;
    QChart *chart;
    QChartView *chartView;
    QChart *createLineChart() const;
    QTimer *tmr;
    QTimer *tmrDeb;
    int arrS[10000];
    int arrSidx;
    int lastArrS;
};

#endif // MAINWINDOW_H
