#ifndef YS_GLOBAL_H
#define YS_GLOBAL_H

#include <iostream>
#include <QtCore>
#include "ys_runtimeaccess.h"

#define YS_VERSION          "0.1a"
#define YS_SLEEP_INTERVAL   1
#define YS_CI_ID            "YarraServer"
#define YS_CI_SETUPTIMOUT   1000
#define YS_CI_FORCECMD      "--force"

#define YS_OUT(x) std::cout << QString(x).toStdString() << std::endl;
#define YS_FREE(x) if (x!=0) { delete x; x=0; }
#define YS_FREEARR(x) if (x!=0) { delete[] x; x=0; }

// Convenience macros for runtime access
#define YSRA ysRuntimeAccess::getInstance()



#endif // YS_GLOBAL_H

