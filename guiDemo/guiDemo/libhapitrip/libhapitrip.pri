LIBTARGET = lib
linux-g++ | linux-g++-64 {
LIBS += -L../libhapitrip -lhapitrip
}
win32 {
LIBS += -L../libhapitrip/release -lhapitrip
}

DEPENDPATH += . ../libhapitrip ../librtaudio
INCLUDEPATH += ../../hapitrip

