TEMPLATE = subdirs

# the chugin needs path to all libs it depends on
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/chuckTrip/hapitrip/
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cc/hacktrip/chuckTrip/coreApp/
SUBDIRS += \
    chugin

# to build app
#SUBDIRS += \
#    app   \
##    chugin
##    \
#    hapitrip  \
##    coreApp
##    lib \
#    rtaudio

#and
#//#define NO_AUDIO
#in hapitrip.h


