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

LIBS += /usr/lib/libdcmdata.a
LIBS += /usr/lib/liboflog.a
LIBS += /usr/lib/libofstd.a
LIBS += -lz

QMAKE_CXXFLAGS += -DENABLE_BUILTIN_DICTIONARY -DENABLE_PRIVATE_TAGS

