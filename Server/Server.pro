#-------------------------------------------------
#
# Project created by QtCreator 2014-04-23T00:38:52
#
#-------------------------------------------------

QT       += core network
QT       -= gui

TARGET = YarraServer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    ys_server.cpp \
    ys_global.cpp \
    ys_controlinterface.cpp

HEADERS += \
    ys_server.h \
    ys_global.h \
    ys_controlinterface.h
