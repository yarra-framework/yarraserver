#include "ys_dynamicconfig.h"
#include "ys_global.h"
#include "ys_server.h"


ysDynamicConfig::ysDynamicConfig()
{
    availableReconModes.clear();
}


void ysDynamicConfig::updateDynamicConfigList()
{
    availableReconModes.clear();
    modesDir.refresh();

    QStringList modeFileList=modesDir.entryList();

    // Remove the extension from the entries
    for (int i=0; i<modeFileList.count(); i++)
    {
        QString entry=modeFileList.at(i);
        entry.truncate(entry.indexOf("."));
        availableReconModes.append(entry);
    }
}


bool ysDynamicConfig::prepare()
{
    QString modesPath=YSRA->staticConfig.modesPath;

    if (!modesDir.cd(modesPath))
    {
        YS_SYSLOG_OUT("ERROR: Can't access modes directory.");
        YS_SYSLOG_OUT("ERROR: Check installation.");
        return false;
    }

    QStringList modesFilter;
    modesFilter << QString("*")+QString(YS_MODE_EXTENSION);
    modesDir.setNameFilters(modesFilter);
    modesDir.setFilter(QDir::Files);

    return true;
}


bool ysDynamicConfig::validateAllReconModes()
{
    updateDynamicConfigList();

    // TODO: Search for availability of binaries and paths

    return true;
}


bool ysDynamicConfig::isReconModeAvailable(QString reconMode)
{
    if (availableReconModes.contains(reconMode))
    {
        return true;
    }
    else
    {
        return false;
    }

}


