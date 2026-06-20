TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    Common \
    NetworkModule \
    StorageModule \
    CoreEngine \
    AppUI \
    CNC_Studio \
    Tests

CNC_Studio.subdir = apps/CNC_Studio
Tests.subdir = Tests

NetworkModule.depends = Common
StorageModule.depends = Common
CoreEngine.depends = Common NetworkModule StorageModule
AppUI.depends = Common
CNC_Studio.depends = Common NetworkModule StorageModule CoreEngine AppUI
Tests.depends = Common NetworkModule StorageModule CoreEngine
