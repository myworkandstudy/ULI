#include "mainwindow.h"
#include <QApplication>

extern int main33(int argc, char *argv[]);

int main(int argc, char *argv[])
{

    //main33(3, NULL);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
