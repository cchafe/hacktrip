TEMPLATE = lib
DEFINES += RTAUDIO_LIBRARY
CONFIG      += link_pkgconfig plugin no_plugin_name_prefix

# Input
HEADERS += ../../rtaudio/RtAudio.h \
     ../../rtaudio/include/dsound.h \
     ../../rtaudio/include/soundcard.h
SOURCES += ../../rtaudio/RtAudio.cpp

#cc from rtaudio/Makefile after rtaudio/autogen.sh default =
# -D__LINUX_ALSA__ -D__LINUX_PULSE__ -D__UNIX_JACK__
linux-g++ | linux-g++-64 {
  QMAKE_CXXFLAGS += -D__LINUX_ALSA__
  LIBS += -lasound -lpthread
}
macx {
  QMAKE_CXXFLAGS += -D__MACOSX_CORE__
}
win32 {
QMAKE_CXXFLAGS += -D__WINDOWS_WASAPI__
HEADERS +=   ../../rtaudio/include/asio.h \
../../rtaudio/include/asiodrivers.h \
../../rtaudio/include/asiodrvr.h \
../../rtaudio/include/asiolist.h \
../../rtaudio/include/functiondiscoverykeys_devpkey.h \
../../rtaudio/include/ginclude.h \
../../rtaudio/include/iasiodrv.h \
../../rtaudio/include/iasiothiscallresolver.h \
../../rtaudio/include/asiosys.h
SOURCES += ../../rtaudio/include/asio.cpp \
../../rtaudio/include/asiodrivers.cpp \
../../rtaudio/include/asiolist.cpp \
../../rtaudio/include/iasiothiscallresolver.cpp

LIBS += -lOle32 -ldsound -lmfplat -lmfuuid \
                 -lksuser -lwmcodecdspuuid -lwinmm
                 }
DISTFILES += \
    include/asioinfo.txt \
    librtaudio.pri
