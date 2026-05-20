QT += core widgets network
CONFIG += c++14 warn_on
TEMPLATE = app
TARGET = CNC_Simulator

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/CNC_Simulator/obj
MOC_DIR = ../../build/CNC_Simulator/moc
RCC_DIR = ../../build/CNC_Simulator/rcc

INCLUDEPATH += $$PWD ../../Common ../../NetworkModule ../../AppUI
LIBS += -L../../bin -lNetworkModule -lAppUI -lCommon

SOURCES += \
    main.cpp \
    SimulatorWindow.cpp

HEADERS += \
    SimulatorWindow.h
