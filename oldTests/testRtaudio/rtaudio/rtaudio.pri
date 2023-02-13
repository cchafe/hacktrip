LIBTARGET = rtaudio

#linux_sound = pipewire

linux-g++ | linux-g++-64 {
contains( linux_sound, pipewire ) {
    message( "Configuring for pipewire..." )
    LIBS += -L../rtaudio -lrtaudio  -L/usr/lib64/pipewire-0.3/jack -ljack -lpulse-simple -lasound -lpulse
} else {
message( "Configuring for native jack..." )
LIBS += -L../rtaudio -lrtaudio -ljack -lpulse-simple -lasound -lpulse
}
}


win32 {
LIBS += -L../rtaudio/release -lrtaudio
}
DEPENDPATH += . ../rtaudio
INCLUDEPATH += ../../rtaudio

