TEMPLATE = lib
DEFINES += HAPITRIP_LIBRARY
CONFIG      += link_pkgconfig plugin no_plugin_name_prefix

CONFIG += c++17
QT -= gui
QT += network
include(../librtaudio/librtaudio.pri)
include(../libregulator/libregulator.pri)


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../hapitrip/hapitrip.cpp

HEADERS += \
    ../../hapitrip/hapitrip_global.h \
    ../../hapitrip/hapitrip.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    libhapitrip.pri
