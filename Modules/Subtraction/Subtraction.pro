QT       += core network
QT       -= gui

TARGET = Subtraction
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    external/dcdictbi.cc \
    sub_mainclass.cpp

HEADERS += \
    sub_mainclass.h

DEFINES += HAVE_CONFIG_H
DEFINES += USE_NULL_SAFE_OFSTRING

INCLUDEPATH += /usr/local/include/dcmtk/dcmnet/
INCLUDEPATH += /usr/local/include/dcmtk/config/

LIBS += /usr/local/lib/libdcmdata.a
LIBS += /usr/local/lib/liboflog.a
LIBS += /usr/local/lib/libofstd.a
LIBS += /usr/lib/x86_64-linux-gnu/libicuuc.a
LIBS += /usr/lib/x86_64-linux-gnu/libicudata.a
LIBS += -lz -ldl

QMAKE_CXXFLAGS += -DENABLE_BUILTIN_DICTIONARY -DENABLE_PRIVATE_TAGS
