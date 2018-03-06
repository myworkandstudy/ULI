#include "mainwindow.h"
#include <QApplication>

extern int main33(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    QStringList paths = QCoreApplication::libraryPaths();
    paths.append(".");
    paths.append("imageformats");
    paths.append("platforms");
    paths.append("sqldrivers");
    QCoreApplication::setLibraryPaths(paths);

    //main33(3, NULL);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
