LIBTARGET = rtaudio

linux-g++ | linux-g++-64 {
# pipewire
#LIBS += -L../rtaudio -lrtaudio  -L/usr/lib64/pipewire-0.3/jack -ljack -lpulse-simple -lasound -lpulse
# real jack
LIBS += -L../rtaudio -lrtaudio -ljack -lpulse-simple -lasound -lpulse
}
win32 {
LIBS += -L../rtaudio/release -lrtaudio
}
DEPENDPATH += . ../rtaudio
INCLUDEPATH += ../../rtaudio

