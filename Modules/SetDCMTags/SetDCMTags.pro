#-------------------------------------------------
#
# Project created by QtCreator 2014-04-24T17:27:13
#
#-------------------------------------------------

QT       += core network
QT       -= gui

TARGET = SetDCMTags
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

SOURCES += main.cpp \
    sdt_mainclass.cpp

HEADERS += \
    sdt_mainclass.h

DEFINES += HAVE_CONFIG_H
DEFINES += USE_NULL_SAFE_OFSTRING

INCLUDEPATH += /usr/local/include/dcmtk/dcmnet/
INCLUDEPATH += /usr/local/include/dcmtk/config/

LIBS += -L/usr/local/lib/ -ldcmnet -ldcmdata -loflog -lofstd -lz
