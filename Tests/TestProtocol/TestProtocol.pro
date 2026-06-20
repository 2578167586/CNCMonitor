QT += core network testlib
CONFIG += c++14 console testcase
TEMPLATE = app
TARGET = tst_Protocol

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/TestProtocol/obj
MOC_DIR = ../../build/TestProtocol/moc
RCC_DIR = ../../build/TestProtocol/rcc

INCLUDEPATH += $$PWD/../../Common $$PWD/../../NetworkModule
LIBS += -L../../bin -lCommon -lNetworkModule

SOURCES += tst_Protocol.cpp
