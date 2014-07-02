#include "ys_mode.h"
#include "ys_global.h"
#include "ys_server.h"
#include "ys_process.h"


ysMode::ysMode()
{
    name="!!INVALID";

    reconBinary="";
    reconArguments="";

    postprocCount=0;
    postprocBinary.clear();
    postprocArguments.clear();

    transferBinary="";
    transferArguments="";

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

        // Reconstruction settings
        reconBinary   =modeFile.value("Reconstruction/Bin",  YS_INI_INVALID).toString();
        reconArguments=modeFile.value("Reconstruction/Args", YS_INI_INVALID).toString();

        // Read post proc settings
        postprocCount=0;
        while ((postprocCount<50) &&
               (modeFile.value("PostProcessing/Bin"+QString::number(postprocCount+1), YS_INI_INVALID).toString()!=YS_INI_INVALID))
        {
            postprocCount++;
            postprocBinary   .append(modeFile.value("PostProcessing/Bin" +QString::number(postprocCount), "").toString());
            postprocArguments.append(modeFile.value("PostProcessing/Args"+QString::number(postprocCount), "").toString());
        }

        // Read transfer settings
        transferBinary   =modeFile.value("Transfer/Bin",  "").toString();
        transferArguments=modeFile.value("Transfer/Args", "").toString();

        // Read the additional options from the mode file
        currentJob->storeProcessedFile=modeFile.value("Options/KeepRawdata", false).toBool();
    }

    return true;
}


void ysMode::parseCmdlines()
{
    reconBinary   =parseString(reconBinary);
    reconArguments=parseString(reconArguments);

    QString inDir=currentProcess->reconDir;
    QString outDir="";

    // Add command lines for postprocessing and storage
    for (int i=0; i<postprocCount; i++)
    {
        outDir=YSRA->staticConfig.workPath+"/"+YS_WORKDIR_POSTPROC+QString::number(i+1);
        postprocBinary.replace(i,parseString(postprocBinary.at(i), inDir, outDir));
        postprocArguments.replace(i,parseString(postprocArguments.at(i), inDir, outDir));
        inDir=outDir;
    }

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

    if (input.contains(YS_MODEMACRO_VALUE_ACC))
    {
        QString replacement=currentJob->accNumber;
        if (replacement=="")
        {
            replacement="0";
        }

        input.replace(YS_MODEMACRO_VALUE_ACC,replacement);
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

