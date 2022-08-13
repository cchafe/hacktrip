QT -= gui
QT += network
#include(../rtaudio/rtaudio.pri)
#include(../coreApp/coreApp.pri)

# build needs environment set
# export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
# or equiv in Projects : Build : Environment
# in ../rtaudio
# ./autogen.sh
# make, make install
# and needs
# QMAKE_LFLAGS += '-Wl,-rpath,\'\$//usr/local/lib\''
#CONFIG += link_pkgconfig
#PKGCONFIG += rtaudio
#QMAKE_LFLAGS += '-Wl,-rpath,\'\$//usr/local/lib\''


TEMPLATE = lib
DEFINES += HAPITRIP_LIBRARY

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    hapitrip.cpp

HEADERS += \
    hapitrip_global.h \
    hapitrip.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    hapitrip.pri
