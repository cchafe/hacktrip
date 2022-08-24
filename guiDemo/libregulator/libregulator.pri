LIBTARGET = lib
linux-g++ | linux-g++-64 {
LIBS += -L../libregulator -lregulator
}
win32 {
LIBS += -L../libregulator/release -lregulator
}

DEPENDPATH += . ../libregulator ../librtaudio
INCLUDEPATH += ../../regulator

