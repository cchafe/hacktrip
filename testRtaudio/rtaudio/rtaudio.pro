# created by Marcin Pączkowski
# configuration for building RtAudio library using qmake

QT -= gui

TEMPLATE = lib
DEFINES += RTAUDIO_LIBRARY

CONFIG += c++17
CONFIG += debug_and_release staticlib

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += ../../rtaudio/RtAudio.h \
    ../../rtaudio/include/dsound.h \
    ../../rtaudio/include/soundcard.h
SOURCES += ../../rtaudio/RtAudio.cpp

#cc from rtaudio/Makefile after rtaudio/autogen.sh default =
# -D__LINUX_ALSA__ -D__LINUX_PULSE__ -D__UNIX_JACK__
linux-g++ | linux-g++-64 {
  QMAKE_CXXFLAGS += -D__LINUX_ALSA__ -D__UNIX_JACK__
}
macx {
  QMAKE_CXXFLAGS += -D__MACOSX_CORE__
}
win32 {
  QMAKE_CXXFLAGS += -D__WINDOWS_ASIO__ -D__WINDOWS_WASAPI__
  INCLUDEPATH += ../../rtaudio/include
  HEADERS += ../../rtaudio/include/asio.h \
             ../../rtaudio/include/asiodrivers.h \
             ../../rtaudio/include/asiolist.h \
             ../../rtaudio/include/asiodrvr.h \
             ../../rtaudio/include/asiosys.h \
             ../../rtaudio/include/ginclude.h \
             ../../rtaudio/include/iasiodrv.h \
             ../../rtaudio/include/iasiothiscallresolver.h \
             ../../rtaudio/include/functiondiscoverykeys_devpkey.h
  SOURCES += ../../rtaudio/include/asio.cpp \
             ../../rtaudio/include/asiodrivers.cpp \
             ../../rtaudio/include/asiolist.cpp \
             ../../rtaudio/include/iasiothiscallresolver.cpp
}

# Default rules for deployment.
#unix {
#    target.path = /usr/lib
#}
#!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    include/asioinfo.txt \
    rtaudio.pri
# however, the .pri file isn't from Marcin, it's copied from ../lib
