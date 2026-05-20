QT += core widgets network sql
CONFIG += c++14 warn_on
TEMPLATE = app
TARGET = CNC_Monitor

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/CNC_Monitor/obj
MOC_DIR = ../../build/CNC_Monitor/moc
RCC_DIR = ../../build/CNC_Monitor/rcc

INCLUDEPATH += $$PWD ../../Common ../../NetworkModule ../../StorageModule ../../CoreEngine ../../AppUI
LIBS += -L../../bin -lCoreEngine -lStorageModule -lNetworkModule -lAppUI -lCommon

SOURCES += \
    main.cpp \
    MonitorWindow.cpp

HEADERS += \
    MonitorWindow.h
