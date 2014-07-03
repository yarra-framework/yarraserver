#ifndef YS_LOG_H
#define YS_LOG_H

#include <QtCore>


class ysLog
{
public:
    ysLog();
    ~ysLog();

    void openSysLog();
    void closeSysLog();

    void openTaskLog(QString suggestedName, QString uniqueID);
    void closeTaskLog();

    void sysLog(QString message, bool screenOutput=false);
    void taskLog(QString message, bool screenOutput=false);
    void taskLogProc(QString message);
    void sysTaskLog(QString message);

    void sysLogOut(QString message);
    void taskLogOut(QString message);
    void sysTaskLogOut(QString message);

    QString getTaskLogFilename();
    void flushLogs();

protected:

    QString taskLogFilename;

    QFile sysLogFile;
    QFile taskLogFile;

};

inline void ysLog::sysTaskLog(QString message)
{
    taskLog(message);
    sysLog(message);
}

inline void ysLog::sysLogOut(QString message)
{
    sysLog(message, true);
}


inline void ysLog::taskLogOut(QString message)
{
    taskLog(message, true);
}

inline void ysLog::sysTaskLogOut(QString message)
{
    sysLog(message);
    taskLog(message,true);
}

inline QString ysLog::getTaskLogFilename()
{
    return taskLogFilename;
}


#endif // YS_LOG_H

