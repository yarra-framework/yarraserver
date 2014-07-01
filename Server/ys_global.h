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

#define YS_MODE_EXTENSION      ".mode"
#define YS_TASK_EXTENSION      ".task"
#define YS_TASK_EXTENSION_PRIO "_prio"
#define YS_LOCK_EXTENSION      ".lock"
#define YS_LOG_SERVER          "yarra.log"

#define YS_WORKDIR_TMP         "temp"
#define YS_WORKDIR_RECON       "recon"
#define YS_WORKDIR_POSTPROC    "postproc"

#define YS_EXEC_LOOPSLEEP      10
#define YS_EXEC_TIMEOUT        86400000

#define YS_WAITMESSAGE         "Waiting for next task..."


#define YS_FREE(x) if (x!=0) { delete x; x=0; }
#define YS_FREEARR(x) if (x!=0) { delete[] x; x=0; }

// Convenience macros for runtime access
#define YSRA ysRuntimeAccess::getInstance()

#define YS_OUT(x) std::cout << QString(x).toStdString() << std::endl;


#define YS_SYSLOG(x) ysRuntimeAccess::getInstance()->log.sysLog(x)
#define YS_TASKLOG(x) ysRuntimeAccess::getInstance()->log.taskLog(x)
#define YS_SYSTASKLOG(x) ysRuntimeAccess::getInstance()->log.sysTaskLog(x)
#define YS_TASKLOGPROC(x) ysRuntimeAccess::getInstance()->log.taskLogProc(x)

#define YS_SYSLOG_OUT(x) ysRuntimeAccess::getInstance()->log.sysLogOut(x)
#define YS_TASKLOG_OUT(x) ysRuntimeAccess::getInstance()->log.taskLogOut(x)
#define YS_SYSTASKLOG_OUT(x) ysRuntimeAccess::getInstance()->log.sysTaskLogOut(x)

#define YS_MAXLOGSIZE 100000000


// Macros for defining the reconstruction call in the mode file
#define YS_MODEMACRO_INPUTFILE  "%i"
#define YS_MODEMACRO_INPUTPATH  "%w"
#define YS_MODEMACRO_MODEPATH   "%m"
#define YS_MODEMACRO_OUTPUTPATH "%o"
#define YS_MODEMACRO_TEMPPATH   "%t"
#define YS_MODEMACRO_ACC        "%a"



#endif // YS_GLOBAL_H

