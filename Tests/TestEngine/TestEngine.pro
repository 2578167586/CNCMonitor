QT += core network sql testlib
CONFIG += c++14 console testcase
TEMPLATE = app
TARGET = tst_Engine

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/TestEngine/obj
MOC_DIR = ../../build/TestEngine/moc
RCC_DIR = ../../build/TestEngine/rcc

INCLUDEPATH += $$PWD/../../Common $$PWD/../../NetworkModule $$PWD/../../StorageModule $$PWD/../../CoreEngine
LIBS += -L../../bin -lCommon -lNetworkModule -lStorageModule -lCoreEngine

SOURCES += tst_Engine.cpp
