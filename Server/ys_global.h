#ifndef YS_GLOBAL_H
#define YS_GLOBAL_H

#include <iostream>
#include <QtCore>
#include "ys_runtimeaccess.h"

#define YS_VERSION          "0.91"
#define YS_SLEEP_INTERVAL   10
#define YS_CI_ID            "YarraServer"
#define YS_CI_SETUPTIMOUT   1000
#define YS_CI_FORCECMD      "--force"

#define YS_MODE_EXTENSION       ".mode"
#define YS_TASK_EXTENSION       ".task"
#define YS_TASK_EXTENSION_PRIO  "_prio"
#define YS_TASK_EXTENSION_NIGHT "_night"
#define YS_LOCK_EXTENSION       ".lock"
#define YS_LOG_SERVER           "yarra.log"
#define YS_HALT_FILE            "HALT"

#define YS_WORKDIR_TMP          "temp"
#define YS_WORKDIR_RECON        "recon"
#define YS_WORKDIR_POSTPROC     "postproc"

#define YS_EXEC_LOOPSLEEP       10        // unit: ms
#define YS_EXEC_TIMEOUT         86400000  // unit: ms
#define YS_EXEC_MEMCHECK        1000      // unit: ms
#define YS_EXEC_MAXOUTPUTLINES  100000    // unit: lines
#define YS_EXEC_MAXOUTPUTIDLE   1800      // unit: sec

#define YS_WAITMESSAGE         "Waiting for next task..."
#define YS_INI_INVALID         "!!FAIL"

#define YS_PROCCOUNT_MAX       50


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
#define YS_MODEMACRO_RECON_INPUTFILE      "%rif"
#define YS_MODEMACRO_RECON_TASKFILE       "%rit"
#define YS_MODEMACRO_RECON_INPUTPATH      "%rid"
#define YS_MODEMACRO_RECON_OUTPUTPATH     "%rod"
#define YS_MODEMACRO_RECON_INPUTNAME      "%rin"

#define YS_MODEMACRO_POSTPROC_INPUTPATH   "%pid"
#define YS_MODEMACRO_POSTPROC_OUTPUTPATH  "%pod"

#define YS_MODEMACRO_TRANSFER_PATH        "%td"
#define YS_MODEMACRO_TEMPPATH             "%tmp"
#define YS_MODEMACRO_MODULES_PATH         "%bd"

#define YS_MODEMACRO_MODE_PATH            "%md"
#define YS_MODEMACRO_MODE_FILE            "%mf"
#define YS_MODEMACRO_MODE_CONFIG          "%mc"

#define YS_MODEMACRO_VALUE_ACC            "%vacc"
#define YS_MODEMACRO_VALUE_PARAM          "%vparam"
#define YS_MODEMACRO_VALUE_TASKID         "%vtid"
#define YS_MODEMACRO_VALUE_UNIQUETASKID   "%vuid"

#define YS_MODEMACRO_HELPER_QUOTE         "%hq"
#define YS_MODEMACRO_HELPER_MATLAB        "%hmb"


#endif // YS_GLOBAL_H

