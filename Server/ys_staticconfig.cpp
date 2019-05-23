#include "ys_staticconfig.h"

#include <QtCore>
#include "ys_global.h"


ysStaticConfig::ysStaticConfig()
{
    serverName="Default Yarra server";

    execPath=QCoreApplication::applicationDirPath();

    logPath        =execPath+"/log";
    modesPath      =execPath+"/modes";
    inqueuePath    =execPath+"/queue";
    workPath       =execPath+"/work";
    failPath       =execPath+"/fail";
    storagePath    =execPath+"/finished";
    modulesPath    =execPath+"/modules";
    modulesUserPath=execPath+"/modules_user";
    resumePath     =execPath+"/resume";

    matlabBinary="/usr/local/bin/matlab";

    notificationEnabled=false;
    notificationErrorMail="";
    notificationFromAddress="YarraServer <noreply@localhost>";
    notificationDomainRestriction="";

    memkillThreshold=95;
    driveSpaceNeededGB=20;
    driveSpaceNotificationThresholdGB=40;

    processTimeout=YS_EXEC_TIMEOUT;

    useNightTasks=false;
    nightStart=QTime(23,0);
    nightEnd=QTime(5,0);
    nightAfterMidnight=false;

    terminateAfterOneTask=false;

    resumeTasks     =false;
    resumeDelayMin  =20;
    resumeMaxRetries=5;

    logServerAddress="";
    logServerAPIKey ="";
}


bool ysStaticConfig::readConfiguration()
{
    QString configurationFile=execPath+"/YarraServer.ini";

    if (!QFile::exists(configurationFile))
    {
        YS_OUT("WARNING: Configuration file YarraServer.ini not found.");
        YS_OUT("WARNING: Default settings will be used.\n");
    }

    {
        QSettings configFile(configurationFile, QSettings::IniFormat);

        serverName     =configFile.value("Server/Name",       serverName).toString();

        logPath        =configFile.value("Paths/Log",         logPath).toString();
        modesPath      =configFile.value("Paths/Modes",       modesPath).toString();
        inqueuePath    =configFile.value("Paths/Queue",       inqueuePath).toString();
        workPath       =configFile.value("Paths/Work",        workPath).toString();
        failPath       =configFile.value("Paths/Fail",        failPath).toString();
        storagePath    =configFile.value("Paths/Storage",     storagePath).toString();
        modulesPath    =configFile.value("Paths/Modules",     modulesPath).toString();
        modulesUserPath=configFile.value("Paths/ModulesUser", modulesUserPath).toString();
        resumePath     =configFile.value("Paths/Resume",      resumePath).toString();

        matlabBinary   =configFile.value("Paths/MatlabBinary",matlabBinary).toString();

        notificationEnabled    =configFile.value("Notification/Enabled",     notificationEnabled).toBool();
        notificationFromAddress=configFile.value("Notification/FromAddress", notificationFromAddress).toString();
        QStringList tempList   =configFile.value("Notification/ErrorMail",   notificationErrorMail).toStringList();
        notificationErrorMail=tempList.join(",");
        notificationDomainRestriction=configFile.value("Notification/DomainRestriction", notificationDomainRestriction).toString();

        processTimeout=configFile.value("Options/ProcessTimeout", processTimeout).toInt();
        memkillThreshold=configFile.value("Options/MemKillThreshold", memkillThreshold).toDouble();
        driveSpaceNeededGB=configFile.value("Options/DriveSpaceNeeded", driveSpaceNeededGB).toInt();
        driveSpaceNotificationThresholdGB=configFile.value("Options/DriveSpaceNotificationThreshold", driveSpaceNotificationThresholdGB).toInt();

        terminateAfterOneTask=configFile.value("Options/TerminateAfterOneTask", terminateAfterOneTask).toBool();

        resumeTasks     =configFile.value("Options/ResumeTasks",      resumeTasks).toBool();
        resumeDelayMin  =configFile.value("Options/ResumeDelayMin",   resumeDelayMin).toInt();
        resumeMaxRetries=configFile.value("Options/ResumeMaxRetries", resumeMaxRetries).toInt();

        useNightTasks=configFile.value("Options/UseNightTasks", useNightTasks).toBool();
        nightStart   =QTime::fromString(configFile.value("Options/NightStart", nightStart.toString(Qt::ISODate)).toString(),Qt::ISODate);
        nightEnd     =QTime::fromString(configFile.value("Options/NightEnd",   nightEnd.toString(Qt::ISODate)).toString(),  Qt::ISODate);

        if ((!nightStart.isValid()) || (!nightEnd.isValid()))
        {
            nightStart=QTime(23,0);
            nightEnd=QTime(5,0);
            YS_OUT("ERROR: Invalid night time defined. Using default values.");
        }

        logServerAddress=configFile.value("LogServer/Address", logServerAddress).toString();
        logServerAPIKey =configFile.value("LogServer/APIKey",  logServerAPIKey ).toString();
    }

    if (nightStart<nightEnd)
    {
        nightAfterMidnight=true;
    }
    else
    {
        nightAfterMidnight=false;
    }

    return true;
}


bool ysStaticConfig::checkDirectories()
{
    bool dirError=false;
    QString affectedDirectories="";

    if (!QFile::exists(modesPath))       { dirError=true; affectedDirectories+=modesPath      +" "; }
    if (!QFile::exists(logPath))         { dirError=true; affectedDirectories+=logPath        +" "; }
    if (!QFile::exists(inqueuePath))     { dirError=true; affectedDirectories+=inqueuePath    +" "; }
    if (!QFile::exists(workPath))        { dirError=true; affectedDirectories+=workPath       +" "; }
    if (!QFile::exists(failPath))        { dirError=true; affectedDirectories+=failPath       +" "; }
    if (!QFile::exists(storagePath))     { dirError=true; affectedDirectories+=storagePath    +" "; }
    if (!QFile::exists(modulesPath))     { dirError=true; affectedDirectories+=modulesPath    +" "; }
    if (!QFile::exists(modulesUserPath)) { dirError=true; affectedDirectories+=modulesUserPath+" "; }
    if (!QFile::exists(resumePath))      { dirError=true; affectedDirectories+=resumePath     +" "; }

    if ((resumeTasks) && (!QFile::exists(resumePath)))
    {
        dirError=true;
        affectedDirectories+=resumePath+" ";
    }

    if (dirError)
    {
        YS_OUT("ERROR: Could not access all required directories. Check configuration.");
        YS_OUT("ERROR: The following directories are affected \n" + affectedDirectories);
        return false;
    }
    else
    {
        return true;
    }
}


bool ysStaticConfig::allowNightReconNow()
{
    // If night time reconstruction is not enabled, always allow
    // processing of night-time recons
    if (!useNightTasks)
    {
        return true;
    }

    QTime current=QTime::currentTime();

    if (nightAfterMidnight)
    {
        if ((current>nightEnd) || (current<nightStart))
        {
            return false;
        }
    }
    else
    {
        if ((current>nightEnd) && (current<nightStart))
        {
            return false;
        }
    }

    return true;
}


