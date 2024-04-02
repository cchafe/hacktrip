QT -= gui

TEMPLATE = lib
DEFINES += LIBPLC_LIBRARY
CONFIG      += link_pkgconfig plugin no_plugin_name_prefix

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../plc/PLC.cpp

HEADERS += \
    ../../plc/PLC.h \
    libplc_global.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    libplc.pri
