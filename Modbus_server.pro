
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT += serialbus serialport widgets

TARGET = Modbus_server
TEMPLATE = app
CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
        settingsdialog.cpp

HEADERS  += mainwindow.h settingsdialog.h

FORMS    += mainwindow.ui settingsdialog.ui

