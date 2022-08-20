LIBTARGET = coreApp
LIBS += -L../coreApp -lcoreApp

linux-g++ | linux-g++-64 {
LIBS += -L../coreApp -lcoreApp
}
win32 {
LIBS += -L../coreApp/release -lcoreApp
}

DEPENDPATH += . ../coreApp
INCLUDEPATH += ../coreApp

