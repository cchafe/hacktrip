LIBTARGET = rtaudio

linux_sound = pipewire

linux-g++ | linux-g++-64 {
contains( linux_sound, pipewire ) {
    message( "Configuring for pipewire..." )
    LIBS += -L../librtaudio -lrtaudio  -L/usr/lib64/pipewire-0.3/jack -ljack -lpulse-simple -lasound -lpulse
} else {
message( "Configuring for native jack..." )
LIBS += -L../librtaudio -lrtaudio -ljack -lpulse-simple -lasound -lpulse
}
}

# windows seems to insist on "liblibrtaudio" even with the no_plugin_name_prefix CONFIG in the .pro
win32 {
LIBS += -L../librtaudio/release -llibrtaudio
}
DEPENDPATH += . ../librtaudio
INCLUDEPATH += ../rtaudio

