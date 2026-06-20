QT += core network testlib
CONFIG += c++14 console testcase
TEMPLATE = app
TARGET = tst_Network

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/TestNetwork/obj
MOC_DIR = ../../build/TestNetwork/moc
RCC_DIR = ../../build/TestNetwork/rcc

INCLUDEPATH += $$PWD/../../Common $$PWD/../../NetworkModule
LIBS += -L../../bin -lCommon -lNetworkModule

SOURCES += tst_Network.cpp
