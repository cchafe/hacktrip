LIBTARGET = lib
LIBS += -L../lib -llib

linux-g++ | linux-g++-64 {
LIBS += -L../lib -llib
}
win32 {
LIBS += -L../lib/release -llib
}

DEPENDPATH += . ../lib
INCLUDEPATH += ../lib

