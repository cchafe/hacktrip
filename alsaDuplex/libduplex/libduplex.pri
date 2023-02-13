LIBTARGET = lib
linux-g++ | linux-g++-64 {
LIBS += -L../libduplex -llibduplex
}
win32 {
LIBS += -L../libduplex/release -llibduplex
}

DEPENDPATH += . ../libduplex
INCLUDEPATH += ../libduplex

