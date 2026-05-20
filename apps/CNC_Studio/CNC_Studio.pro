QT += core widgets network sql printsupport
CONFIG += c++14 warn_on
TEMPLATE = app
TARGET = CNC_Studio

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/CNC_Studio/obj
MOC_DIR = ../../build/CNC_Studio/moc
RCC_DIR = ../../build/CNC_Studio/rcc

INCLUDEPATH += $$PWD ../../Common ../../NetworkModule ../../StorageModule ../../CoreEngine ../../AppUI ../../ThirdParty
LIBS += -L../../bin -lCoreEngine -lStorageModule -lNetworkModule -lAppUI -lCommon

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    MonitorPage.cpp \
    SimulatorPage.cpp \
    ../../ThirdParty/qcustomplot.cpp

HEADERS += \
    MainWindow.h \
    MonitorPage.h \
    SimulatorPage.h \
    ../../ThirdParty/qcustomplot.h
