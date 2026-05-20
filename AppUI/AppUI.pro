QT += core widgets printsupport
CONFIG += staticlib c++14 warn_on
TEMPLATE = lib
TARGET = AppUI

DESTDIR = ../bin
OBJECTS_DIR = ../build/AppUI/obj
MOC_DIR = ../build/AppUI/moc
RCC_DIR = ../build/AppUI/rcc

INCLUDEPATH += $$PWD ../Common ../ThirdParty
LIBS += -L../bin -lCommon

HEADERS += \
    RealTimePlotWidget.h \
    ParameterTableModel.h \
    NumericDelegate.h \
    PlotWidget.h \
    StatusColors.h

SOURCES += \
    RealTimePlotWidget.cpp \
    ParameterTableModel.cpp \
    NumericDelegate.cpp \
    PlotWidget.cpp \
    ../ThirdParty/qcustomplot.cpp
