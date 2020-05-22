#-------------------------------------------------
#
# Project created by QtCreator 2020-05-20T11:25:23
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HTYServer
TEMPLATE = app

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp\
        qserver.cpp \
        qclientthread.cpp

HEADERS += \
        mainwindow.h\
      qserver.h \
      qclientthread.h


FORMS += \
        mainwindow.ui

RESOURCES += \
    res.qrc