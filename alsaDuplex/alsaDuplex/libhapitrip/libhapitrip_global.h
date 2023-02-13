#ifndef HAPITRIP_GLOBAL_H
#define HAPITRIP_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(HAPITRIP_LIBRARY)
#  define HAPITRIP_EXPORT Q_DECL_EXPORT
#else
#  define HAPITRIP_EXPORT Q_DECL_IMPORT
#endif

#endif // HAPITRIP_GLOBAL_H
