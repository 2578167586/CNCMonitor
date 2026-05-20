QT += core network sql
CONFIG += staticlib c++14 warn_on
TEMPLATE = lib
TARGET = CoreEngine

DESTDIR = ../bin
OBJECTS_DIR = ../build/CoreEngine/obj
MOC_DIR = ../build/CoreEngine/moc
RCC_DIR = ../build/CoreEngine/rcc

INCLUDEPATH += $$PWD ../Common ../NetworkModule ../StorageModule
LIBS += -L../bin -lStorageModule -lNetworkModule -lCommon

HEADERS += \
    DeviceStatePool.h \
    MonitorEngine.h

SOURCES += \
    DeviceStatePool.cpp \
    MonitorEngine.cpp
