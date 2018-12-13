QT += core network concurrent
QT -= gui


CONFIG += c++11
TEMPLATE = app


SOURCES += \
    src/main.cpp \
    src/netdvr.cpp

HEADERS += \
    include/HCNetSDK.h \
    include/netdvr.h

INCLUDEPATH += ./include/

include($$PWD/library/JQLibrary/JQLibrary.pri)



#bin file and dll
DESTDIR = $$PWD/bin
QMAKE_LFLAGS += "-Wl,-rpath=libs:libs/HCNetSDKCom"


unix:!macx: LIBS += -L$$PWD/bin/libs/ -lhcnetsdk

INCLUDEPATH += $$PWD/bin/libs
DEPENDPATH += $$PWD/bin/libs

unix:!macx: LIBS += -L$$PWD/bin/libs/ -lhpr

INCLUDEPATH += $$PWD/bin/libs
DEPENDPATH += $$PWD/bin/libs

unix:!macx: LIBS += -L$$PWD/bin/libs/ -lHCCore

INCLUDEPATH += $$PWD/bin/libs
DEPENDPATH += $$PWD/bin/libs

RESOURCES += \
    $$PWD/key/key.qrc
