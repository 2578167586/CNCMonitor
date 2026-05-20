TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    Common \
    NetworkModule \
    StorageModule \
    CoreEngine \
    AppUI \
    CNC_Studio

CNC_Studio.subdir = apps/CNC_Studio

NetworkModule.depends = Common
StorageModule.depends = Common
CoreEngine.depends = Common NetworkModule StorageModule
AppUI.depends = Common
CNC_Studio.depends = Common NetworkModule StorageModule CoreEngine AppUI
