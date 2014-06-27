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
    ys_controlinterface.cpp \
    ys_controlapi.cpp \
    ys_staticconfig.cpp \
    ys_dynamicconfig.cpp \
    ys_log.cpp \
    ys_runtimeaccess.cpp \
    ys_job.cpp \
    ys_process.cpp \
    ys_notificationmail.cpp \
    ys_queue.cpp \
    ys_statistics.cpp \
    ys_mode.cpp

HEADERS += \
    ys_server.h \
    ys_global.h \
    ys_controlinterface.h \
    ys_controlapi.h \
    ys_staticconfig.h \
    ys_dynamicconfig.h \
    ys_log.h \
    ys_runtimeaccess.h \
    ys_job.h \
    ys_process.h \
    ys_notificationmail.h \
    ys_queue.h \
    ys_statistics.h \
    ys_mode.h
