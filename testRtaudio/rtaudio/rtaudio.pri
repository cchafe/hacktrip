LIBTARGET = rtaudio
# pipewire
#LIBS += -L../rtaudio -lrtaudio  -L/usr/lib64/pipewire-0.3/jack -ljack -lpulse-simple -lasound -lpulse
# real jack
LIBS += -L../rtaudio -lrtaudio -ljack -lpulse-simple -lasound -lpulse

DEPENDPATH += . ../rtaudio
INCLUDEPATH += ../../rtaudio

