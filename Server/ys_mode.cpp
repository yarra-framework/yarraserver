#include "ys_mode.h"
#include "ys_global.h"
#include "ys_server.h"
#include "ys_process.h"


ysMode::ysMode()
{
    name="!!INVALID";

    preprocCount=0;
    preprocBinary.clear();
    preprocArguments.clear();
    preprocDisableMemKill=false;

    reconBinary="";
    reconArguments="";
    reconDisableMemKill=false;

    postprocCount=0;
    postprocBinary.clear();
    postprocArguments.clear();
    postprocDisableMemKill=false;

    transferBinary="";
    transferArguments="";
    transferDisableMemKill=false;

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
    currentJob=job;

    QString modeFilename=YSRA->staticConfig.modesPath+"/"+job->reconMode+YS_MODE_EXTENSION;

    {
        // Open the mode file
        QSettings modeFile(modeFilename, QSettings::IniFormat);

        // Read pre-processing settings
        preprocCount=0;
        while ((preprocCount<YS_PROCCOUNT_MAX) &&
               (modeFile.value("PreProcessing/Bin"+QString::number(preprocCount+1), YS_INI_INVALID).toString()!=YS_INI_INVALID))
        {
            preprocCount++;
            preprocBinary   .append(modeFile.value("PreProcessing/Bin" +QString::number(preprocCount), "").toString());
            preprocArguments.append(modeFile.value("PreProcessing/Args"+QString::number(preprocCount), "").toString());
        }
        preprocDisableMemKill=modeFile.value("PreProcessing/DisableMemKill", false).toBool();

        // Reconstruction settings
        reconBinary        =modeFile.value("Reconstruction/Bin",  YS_INI_INVALID).toString();
        reconArguments     =modeFile.value("Reconstruction/Args", YS_INI_INVALID).toString();
        reconDisableMemKill=modeFile.value("Reconstruction/DisableMemKill", false).toBool();

        // Read post-processing settings
        postprocCount=0;
        while ((postprocCount<YS_PROCCOUNT_MAX) &&
               (modeFile.value("PostProcessing/Bin"+QString::number(postprocCount+1), YS_INI_INVALID).toString()!=YS_INI_INVALID))
        {
            postprocCount++;
            postprocBinary   .append(modeFile.value("PostProcessing/Bin" +QString::number(postprocCount), "").toString());
            postprocArguments.append(modeFile.value("PostProcessing/Args"+QString::number(postprocCount), "").toString());
        }
        postprocDisableMemKill=modeFile.value("PostProcessing/DisableMemKill", false).toBool();

        // Read transfer settings
        transferBinary        =modeFile.value("Transfer/Bin",  "").toString();
        transferArguments     =modeFile.value("Transfer/Args", "").toString();
        transferDisableMemKill=modeFile.value("Transfer/DisableMemKill", false).toBool();

        // Read the additional options from the mode file
        currentJob->storeProcessedFile=modeFile.value("Options/KeepRawdata", false).toBool();
    }

    return true;
}


void ysMode::parseCmdlines()
{
    // Read preprocessing settings
    for (int i=0; i<preprocCount; i++)
    {
        preprocBinary.replace(i,parseString(preprocBinary.at(i)));
        preprocArguments.replace(i,parseString(preprocArguments.at(i)));
    }

    // Read reconstruction settings
    reconBinary   =parseString(reconBinary);
    reconArguments=parseString(reconArguments);

    // Read postprocessing settings
    QString inDir=currentProcess->reconDir;
    QString outDir="";

    for (int i=0; i<postprocCount; i++)
    {
        outDir=YSRA->staticConfig.workPath+"/"+YS_WORKDIR_POSTPROC+QString::number(i+1);
        postprocBinary.replace(i,parseString(postprocBinary.at(i), inDir, outDir));
        postprocArguments.replace(i,parseString(postprocArguments.at(i), inDir, outDir));
        inDir=outDir;
    }

    // Read transfer settings
    transferBinary   =parseString(transferBinary);
    transferArguments=parseString(transferArguments);
}


QString ysMode::getReconCmdLine()
{
    return reconBinary + " " + reconArguments;
}


QString ysMode::getTransferCmdLine()
{
    return transferBinary + " " + transferArguments;
}


QString ysMode::getPreprocCmdLine(int i)
{
    if (i>=preprocCount)
    {
        return "";
    }

    return preprocBinary.at(i) + " " + preprocArguments.at(i);
}


QString ysMode::getPostprocCmdLine(int i)
{
    if (i>=postprocCount)
    {
        return "";
    }

    return postprocBinary.at(i) + " " + postprocArguments.at(i);
}



QString ysMode::parseString(QString input, QString postprocIn, QString postprocOut)
{
    if (input.contains(YS_MODEMACRO_RECON_INPUTFILE))
    {
        QString replacement=currentJob->scanFile;
        input.replace(YS_MODEMACRO_RECON_INPUTFILE,replacement);
    }

    if (input.contains(YS_MODEMACRO_RECON_INPUTPATH))
    {
        QString replacement=YSRA->staticConfig.workPath;
        input.replace(YS_MODEMACRO_RECON_INPUTPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_RECON_TASKFILE))
    {
        QString replacement=currentJob->taskFile;
        input.replace(YS_MODEMACRO_RECON_TASKFILE,replacement);
    }

    if (input.contains(YS_MODEMACRO_RECON_INPUTNAME))
    {
        QString replacement=currentJob->taskFile;
        replacement.truncate(replacement.indexOf(YS_TASK_EXTENSION));
        input.replace(YS_MODEMACRO_RECON_INPUTNAME,replacement);
    }

    if (input.contains(YS_MODEMACRO_RECON_OUTPUTPATH))
    {
        QString replacement=currentProcess->reconDir;
        input.replace(YS_MODEMACRO_RECON_OUTPUTPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_TEMPPATH))
    {
        QString replacement=currentProcess->tmpDir;
        input.replace(YS_MODEMACRO_TEMPPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_TRANSFER_PATH))
    {
        QString replacement=currentProcess->transferDir;
        input.replace(YS_MODEMACRO_TRANSFER_PATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_POSTPROC_INPUTPATH))
    {
        QString replacement=postprocIn;
        input.replace(YS_MODEMACRO_POSTPROC_INPUTPATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_POSTPROC_OUTPUTPATH))
    {
        QString replacement=postprocOut;
        input.replace(YS_MODEMACRO_POSTPROC_OUTPUTPATH,replacement);
    }    

    if (input.contains(YS_MODEMACRO_MODULES_PATH))
    {
        QString replacement=YSRA->staticConfig.modulesPath;
        input.replace(YS_MODEMACRO_MODULES_PATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_MODE_PATH))
    {
        QString replacement=YSRA->staticConfig.modesPath;
        input.replace(YS_MODEMACRO_MODE_PATH,replacement);
    }

    if (input.contains(YS_MODEMACRO_MODE_FILE))
    {
        QString replacement=currentJob->reconMode+YS_MODE_EXTENSION;
        input.replace(YS_MODEMACRO_MODE_FILE,replacement);
    }

    if (input.contains(YS_MODEMACRO_MODE_CONFIG))
    {
        QString replacement=YSRA->staticConfig.modesPath+"/"+currentJob->reconMode+YS_MODE_EXTENSION;
        input.replace(YS_MODEMACRO_MODE_CONFIG,replacement);
    }

    if (input.contains(YS_MODEMACRO_VALUE_ACC))
    {
        QString replacement=currentJob->accNumber;
        if (replacement=="")
        {
            replacement="0";
        }

        input.replace(YS_MODEMACRO_VALUE_ACC,replacement);
    }

    if (input.contains(YS_MODEMACRO_VALUE_PARAM))
    {
        QString replacement=currentJob->paramValue;
        if (replacement=="")
        {
            replacement="N";
        }

        input.replace(YS_MODEMACRO_VALUE_PARAM,replacement);
    }

    if (input.contains(YS_MODEMACRO_VALUE_TASKID))
    {
        QString replacement=currentJob->getTaskID();
        input.replace(YS_MODEMACRO_VALUE_TASKID,replacement);
    }

    if (input.contains(YS_MODEMACRO_VALUE_UNIQUETASKID))
    {
        QString replacement=currentJob->getUniqueTaskID();
        input.replace(YS_MODEMACRO_VALUE_UNIQUETASKID,replacement);
    }

    return input;
}

