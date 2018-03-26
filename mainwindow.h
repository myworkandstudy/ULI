#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QLineSeries>
#include <QLabel>
#include <QValueAxis>

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

    void on_HomeXButton_clicked();

    void on_HomeYButton_clicked();

    void on_HomeZButton_clicked();

    void on_toolButton_clicked();

    void updateTime();
    void updateTimeGraph();
    void updateTimeDeb();

    void on_CureCoordXButton_clicked();

    void on_CureCoordYButton_clicked();

    void on_CureCoordZButton_clicked();

    void on_StartButton_clicked();

    void on_StopButton_clicked();

    void on_PwrXButton_clicked();

    void on_PwrYButton_clicked();

    void on_PwrZButton_clicked();

    void on_verticalSlider_2_valueChanged(int value);

    void on_spinBox_4_valueChanged(int arg1);

    void on_lineEdit_7_textChanged(const QString &arg1);

    void on_lineEdit_8_textChanged(const QString &arg1);

    void on_lineEdit_9_textChanged(const QString &arg1);

    void on_MoveAllButton_clicked();

    void on_pushButton_14_clicked();

    void on_DebugButton1_clicked();

private:
    DataTable m_dataTable;
    Ui::MainWindow *ui;
    QLineSeries *series0;
    QLineSeries *series1;
    QValueAxis *m_axisX, *m_axisY;
    QChart *chart;
    QChartView *chartView;
    QChart *createLineChart() const;
    QTimer *tmr;
    QTimer *tmrG;
    QTimer *tmrDeb;
    int arrS[10000];
    int arrSidx;
    int lastArrS;
    QTime *time1;
};

#endif // MAINWINDOW_H
