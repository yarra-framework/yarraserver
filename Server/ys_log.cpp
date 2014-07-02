#include "ys_log.h"
#include "ys_global.h"
#include "ys_server.h"


ysLog::ysLog()
{
    taskLogFilename="";
}


ysLog::~ysLog()
{
    closeTaskLog();
    closeSysLog();
}



void ysLog::openSysLog()
{
    QString logFilename=YSRA->staticConfig.logPath+"/"+YS_LOG_SERVER;

    sysLogFile.setFileName(logFilename);
    sysLogFile.open(QIODevice::Append | QIODevice::Text);

    if (sysLogFile.size() > 100000000)
    {
        YS_OUT("Size of syslog getting large.");
        YS_OUT("Renaming file and creating empty file.");
        sysLogFile.flush();

        if (!sysLogFile.rename(logFilename + "_" + QDate::currentDate().toString("ddMMyy")))
        {
            // If the filename already exists (very unlikely), add the time to make the filename unique
            sysLogFile.rename(logFilename + "_" + QDate::currentDate().toString("ddMMyy")+QTime::currentTime().toString("HHmmss"));
        }

        sysLogFile.close();
        sysLogFile.setFileName(logFilename);
        sysLogFile.open(QIODevice::Append | QIODevice::Text);
    }

    sysLog("##[START]######################################");
    sysLog("YarraServer Version " + QString(YS_VERSION));
    sysLog("Name: " + YSRA->staticConfig.serverName + ", Type: " + YSRA->staticConfig.serverType);
}


void ysLog::closeSysLog()
{
    if (sysLogFile.isOpen())
    {
        sysLog("Closing session.");
        sysLog("##[END]########################################");
        sysLog("");
    }
    sysLogFile.close();
}


void ysLog::openTaskLog(QString suggestedName, QString uniqueID)
{
    QString logFilename=YSRA->staticConfig.logPath+"/"+suggestedName;

    taskLogFile.setFileName(logFilename + ".log");

    // If the log file already exists from a previous reconstruction, add the unique ID that
    // was created from the time by the queue object
    if (taskLogFile.exists())
    {
        logFilename += "_" + uniqueID;
    }

    taskLogFilename=logFilename+ ".log";

    taskLogFile.setFileName(taskLogFilename);
    taskLogFile.open(QIODevice::Append | QIODevice::Text);
    taskLog("##[TASK START]####################################");
    taskLog("");
    taskLog("YarraServer Vesion " + QString(YS_VERSION));
    taskLog("Name: " + YSRA->staticConfig.serverName + ", Type: " + YSRA->staticConfig.serverType);
    taskLog("");
}


void ysLog::closeTaskLog()
{
    if (taskLogFile.isOpen())
    {
        taskLog("Closing session.");
        taskLog("##[TASK END]######################################\n");
    }
    taskLogFile.close();
}


void ysLog::sysLog(QString message, bool screenOutput)
{
    QString line="\n";
    if (message.length()>0)
    {
        line=QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss") + "  --  " + message + "\n";
    }

    sysLogFile.write(line.toLatin1());
    sysLogFile.flush();

    if (screenOutput)
    {
        YS_OUT(message);
    }
}


void ysLog::taskLog(QString message, bool screenOutput)
{
    QString line="\n";
    if (message.length()>0)
    {
        line=QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss") + "  --  " + message + "\n";
    }

    taskLogFile.write(line.toLatin1());
    taskLogFile.flush();

    if (screenOutput)
    {
        YS_OUT(message);
    }
}


void ysLog::taskLogProc(QString message)
{
    QString line=QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss") + "  --  " + message;
    taskLogFile.write(line.toLatin1());
    taskLogFile.flush();
}
