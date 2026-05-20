QT += core network
CONFIG += staticlib c++14 warn_on
TEMPLATE = lib
TARGET = NetworkModule

DESTDIR = ../bin
OBJECTS_DIR = ../build/NetworkModule/obj
MOC_DIR = ../build/NetworkModule/moc
RCC_DIR = ../build/NetworkModule/rcc

INCLUDEPATH += $$PWD ../Common
LIBS += -L../bin -lCommon

HEADERS += \
    Protocol.h \
    TcpClientWorker.h \
    TcpDeviceServer.h

SOURCES += \
    Protocol.cpp \
    TcpClientWorker.cpp \
    TcpDeviceServer.cpp
