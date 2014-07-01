#include "ys_mode.h"
#include "ys_global.h"
#include "ys_server.h"
#include "ys_process.h"


ysMode::ysMode()
{
    name="!!INVALID";

    reconBinary="";
    reconArguments="";
    currentJob=0;
    currentProcess=0;
}


void ysMode::setProcess(ysProcess* process)
{
    currentProcess=process;
}


bool ysMode::readModeSettings(QString modeName, ysJob* job)
{
    name=modeName;
    QString modeFilename=YSRA->staticConfig.modesPath+"/"+job->reconMode+YS_MODE_EXTENSION;

    {
        QSettings modeFile(modeFilename, QSettings::IniFormat);
        reconBinary   =modeFile.value("Reconstruction/Bin",  "!!FAIL").toString();
        reconArguments=modeFile.value("Reconstruction/Args", "!!FAIL").toString();
    }

    currentJob=job;

    return true;
}


void ysMode::parseCmdlines()
{
    reconBinary   =parseString(reconBinary);
    reconArguments=parseString(reconArguments);

    // TODO: Add command lines for postprocessing and storage
}


QString ysMode::getFullCmdLine()
{
    return reconBinary + " " + reconArguments;
}


QString ysMode::parseString(QString input)
{
    if (input.contains(YS_MODEMACRO_INPUTFILE))
    {
        QString replacement=currentJob->scanFile;
        input.replace(YS_MODEMACRO_INPUTFILE,replacement);
    }

    if (input.contains(YS_MODEMACRO_INPUTPATH))
    {
        QString replacement=YSRA->staticConfig.workPath;
        input.replace(YS_MODEMACRO_INPUTPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_MODEPATH))
    {
        QString replacement=YSRA->staticConfig.modesPath;
        input.replace(YS_MODEMACRO_MODEPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_OUTPUTPATH))
    {
        QString replacement=currentProcess->reconDir;
        input.replace(YS_MODEMACRO_OUTPUTPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_TEMPPATH))
    {
        QString replacement=currentProcess->tmpDir;
        input.replace(YS_MODEMACRO_TEMPPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_ACC))
    {
        QString replacement=currentJob->accNumber;
        if (replacement=="")
        {
            replacement="0";
        }

        input.replace(YS_MODEMACRO_ACC,replacement);
    }

    return input;
}

