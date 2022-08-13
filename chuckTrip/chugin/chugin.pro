QT -= gui
QT += core network
#include(../hapitrip/hapitrip.pri)
#include(../coreApp/coreApp.pri)

CONFIG += c++17 console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = chucktrip.chug

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
SOURCES += chucktrip.cpp \
../hapitrip/hapitrip.cpp

HEADERS += ../hapitrip/hapitrip.h
INCLUDEPATH += ../hapitrip


INCLUDEPATH += .
INCLUDEPATH += chuck/include/
DEFINES += "__LINUX_ALSA__"
DEFINES += "__PLATFORM_LINUX__"
QMAKE_LFLAGS += "-shared -lstdc++"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# extra from ssr.pro
#QMAKE_CLEAN += $(TARGET)
#QMAKE_DISTCLEAN += $(TARGET)
#QMAKE_DISTCLEAN +=  .qmake.stash
#QMAKE_DISTCLEAN +=  Makefile

