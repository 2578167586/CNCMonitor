QT += core
CONFIG += staticlib c++14 warn_on
TEMPLATE = lib
TARGET = Common

DESTDIR = ../bin
OBJECTS_DIR = ../build/Common/obj
MOC_DIR = ../build/Common/moc
RCC_DIR = ../build/Common/rcc

INCLUDEPATH += $$PWD

HEADERS += \
    Types.h \
    Logger.h

SOURCES += \
    Types.cpp \
    Logger.cpp
