#ifndef LIBDUPLEX_GLOBAL_H
#define LIBDUPLEX_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBDUPLEX_LIBRARY)
#  define LIBDUPLEX_EXPORT Q_DECL_EXPORT
#else
#  define LIBDUPLEX_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBDUPLEX_GLOBAL_H
