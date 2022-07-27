LIBTARGET = rtaudio
LIBS += -L../rtaudio -lrtaudio  -ljack -lpulse-simple -lasound -lpulse

DEPENDPATH += . ../rtaudio
INCLUDEPATH += ../../rtaudio

