QT += core network sql testlib
CONFIG += c++14 console testcase
TEMPLATE = app
TARGET = tst_Integration

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/TestIntegration/obj
MOC_DIR = ../../build/TestIntegration/moc
RCC_DIR = ../../build/TestIntegration/rcc

INCLUDEPATH += $$PWD/../../Common $$PWD/../../NetworkModule $$PWD/../../StorageModule $$PWD/../../CoreEngine
LIBS += -L../../bin -lCommon -lNetworkModule -lStorageModule -lCoreEngine

SOURCES += tst_Integration.cpp
