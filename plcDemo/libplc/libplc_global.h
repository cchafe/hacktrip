#ifndef LIBPLC_GLOBAL_H
#define LIBPLC_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBPLC_LIBRARY)
#define LIBPLC_EXPORT Q_DECL_EXPORT
#else
#define LIBPLC_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBPLC_GLOBAL_H
