QT += core sql
CONFIG += staticlib c++14 warn_on
TEMPLATE = lib
TARGET = StorageModule

DESTDIR = ../bin
OBJECTS_DIR = ../build/StorageModule/obj
MOC_DIR = ../build/StorageModule/moc
RCC_DIR = ../build/StorageModule/rcc

INCLUDEPATH += $$PWD ../Common
LIBS += -L../bin -lCommon

HEADERS += \
    StorageWorker.h

SOURCES += \
    StorageWorker.cpp
