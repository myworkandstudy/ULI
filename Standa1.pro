#-------------------------------------------------
#
# Project created by QtCreator 2017-11-03T15:02:39
#
#-------------------------------------------------

QT       += core gui
QT += charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Standa1
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    ../../source/repos/ConsoleApplication1/ConsoleApplication1/My8SMC1.cpp \
    Source.cpp

HEADERS += \
        mainwindow.h \
    USMCDLL.h \
    ../../source/repos/ConsoleApplication1/ConsoleApplication1/My8SMC1.h \
    include2/e2010cmd.h \
    include2/ifc_ldev.h \
    include2/ioctl.h

FORMS += \
        mainwindow.ui

#LIBS += -lUSMCDLL
#win32:LIBS += C:/Program Files/MicroSMCx64/USMCDLL.dll
#LIBS += -LC:/Program Files/MicroSMCx64/ -lUSMCDLL
LIBS += $$PWD/libUSMCDLL.lib

#CONFIG += console

DISTFILES += \
    ../../source/repos/ConsoleApplication1/ConsoleApplication1/config1.json \
    ../../source/repos/ConsoleApplication1/ConsoleApplication1/settings.json

