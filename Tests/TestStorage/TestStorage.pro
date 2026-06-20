QT += core sql testlib
CONFIG += c++14 console testcase
TEMPLATE = app
TARGET = tst_Storage

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/TestStorage/obj
MOC_DIR = ../../build/TestStorage/moc
RCC_DIR = ../../build/TestStorage/rcc

INCLUDEPATH += $$PWD/../../Common $$PWD/../../StorageModule
LIBS += -L../../bin -lCommon -lStorageModule

SOURCES += tst_Storage.cpp
