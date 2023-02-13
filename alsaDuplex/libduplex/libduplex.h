#ifndef LIBDUPLEX_H
#define LIBDUPLEX_H
#include <QThread>

#include "libduplex_global.h"
#include <../../rtaudio/RtAudio.h>
// #define USEBETA // 6beta1 rtaudio from github, otherwise 5.2.0 from rthaudio site

class LIBDUPLEX_EXPORT Libduplex : public QThread
{
public:
    Libduplex();
    virtual void run();
private:
};

#endif // LIBDUPLEX_H
