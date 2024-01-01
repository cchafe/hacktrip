TEMPLATE = lib
DEFINES += REGULATOR_LIBRARY
CONFIG      += link_pkgconfig plugin no_plugin_name_prefix

CONFIG += c++17
QT -= gui
QT += network
#include(../libhapitrip/libhapitrip.pri)


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../regulator/regulator.cpp

HEADERS += \
    ../../regulator/WaitFreeFrameBuffer.h \
    ../../regulator/WaitFreeRingBuffer.h \
    ../../regulator/regulator_global.h \
    ../../regulator/regulator.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    libregulator.pri
