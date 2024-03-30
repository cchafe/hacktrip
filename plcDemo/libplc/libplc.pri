LIBTARGET = lib
linux-g++ | linux-g++-64 {
LIBS += -L../libplc -lplc
}
win32 {
LIBS += -L../libplc/release -lplc
}

DEPENDPATH += . ../libplc
INCLUDEPATH += ../../plc

