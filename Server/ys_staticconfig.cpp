#include "ys_staticconfig.h"

#include <QtCore>
#include "ys_global.h"


ysStaticConfig::ysStaticConfig()
{
    QString execPath=QCoreApplication::applicationDirPath();

    logPath=execPath+"/log";
    modesPath=execPath+"/modes";

    inqueuePath=execPath+"/queue";
    workPath=execPath+"/work";
    failPath=execPath+"/fail";
    storagePath=execPath+"/finished";

    serverName="Yarra";
    serverType="Unspecified";

    errorNotificationMail="";

    YS_OUT("Path is " + logPath);
}


bool ysStaticConfig::readConfiguration()
{
    return true;
}

