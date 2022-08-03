LIBTARGET = lib
linux-g++ | linux-g++-64 {
LIBS += -L../hapitrip -lhapitrip
}
win32 {
LIBS += -L../hapitrip/release -lhapitrip
}


DEPENDPATH += . ../hapitrip
INCLUDEPATH += ../hapitrip

