#ifndef RTAUDIO_GLOBAL_H
#define RTAUDIO_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(RTAUDIO_LIBRARY)
#  define RTAUDIO_EXPORT Q_DECL_EXPORT
#else
#  define RTAUDIO_EXPORT Q_DECL_IMPORT
#endif

#endif // RTAUDIO_GLOBAL_H
