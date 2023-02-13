#ifndef COREAPP_GLOBAL_H
#define COREAPP_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(COREAPP_LIBRARY)
#  define COREAPP_EXPORT Q_DECL_EXPORT
#else
#  define COREAPP_EXPORT Q_DECL_IMPORT
#endif

#endif // COREAPP_GLOBAL_H
