#-------------------------------------------------
#
# Project created by QtCreator 2014-08-08T09:01:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = graphs
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    utils.cpp \
    plotfile.cpp \
    dialog.cpp

HEADERS  += mainwindow.h \
    utils.h \
    plotfile.h \
    dialog.h

FORMS    += mainwindow.ui \
    dialog.ui

QMAKE_CXXFLAGS += -std=c++11


CONFIG += qwt
INCLUDEPATH +="/usr/include"
LIBS += -L/usr/lib -lqwt
