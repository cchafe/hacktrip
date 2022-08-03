TEMPLATE = lib
DEFINES += RTAUDIO_LIBRARY

#CONFIG += c++17
#CONFIG += debug_and_release staticlib

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
              QMAKE_CXXFLAGS += -D__WINDOWS_WASAPI__
HEADERS +=   include/asio.h \
              include/asiodrivers.h \
              include/asiodrvr.h \
              include/asiolist.h \
              include/functiondiscoverykeys_devpkey.h \
              include/ginclude.h \
              include/iasiodrv.h \
              include/iasiothiscallresolver.h \
              include/asiosys.h
SOURCES += include/asio.cpp \
              include/asiodrivers.cpp \
              include/asiolist.cpp \
              include/iasiothiscallresolver.cpp

LIBS += -lOle32 -ldsound -lmfplat -lmfuuid \
                 -lksuser -lwmcodecdspuuid -lwinmm}
DISTFILES += \
    include/asioinfo.txt \
    rtaudio.pri
# however, the .pri file isn't from Marcin, it's copied from ../lib
