TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    TestProtocol \
    TestStorage \
    TestNetwork \
    TestEngine \
    TestIntegration

TestProtocol.subdir = TestProtocol
TestStorage.subdir = TestStorage
TestNetwork.subdir = TestNetwork
TestEngine.subdir = TestEngine
TestIntegration.subdir = TestIntegration
