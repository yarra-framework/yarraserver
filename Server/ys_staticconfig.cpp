#include "ys_staticconfig.h"

#include <QtCore>
#include "ys_global.h"


ysStaticConfig::ysStaticConfig()
{
    execPath=QCoreApplication::applicationDirPath();

    serverName="Default Yarra server";
    serverType="Unspecified";

    logPath=execPath+"/log";
    modesPath=execPath+"/modes";

    inqueuePath=execPath+"/queue";
    workPath=execPath+"/work";
    failPath=execPath+"/fail";
    storagePath=execPath+"/finished";

    notificationEnabled=false;
    notificationErrorMail="";
    notificationFromAddress="YarraServer <noreply@localhost>";
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

        serverName=  configFile.value("Server/Name", serverName).toString();
        serverType=  configFile.value("Server/Type", serverType).toString();

        logPath    =configFile.value("Paths/Log",     logPath).toString();
        modesPath  =configFile.value("Paths/Modes",   modesPath).toString();
        inqueuePath=configFile.value("Paths/Queue",   inqueuePath).toString();
        workPath   =configFile.value("Paths/Work",    workPath).toString();
        failPath   =configFile.value("Paths/Fail",    failPath).toString();
        storagePath=configFile.value("Paths/Storage", storagePath).toString();

        notificationEnabled    =configFile.value("Notification/Enabled",     notificationEnabled).toBool();
        notificationErrorMail  =configFile.value("Notification/ErrorMail",   notificationErrorMail).toString();
        notificationFromAddress=configFile.value("Notification/FromAddress", notificationFromAddress).toString();
    }

    return true;
}

