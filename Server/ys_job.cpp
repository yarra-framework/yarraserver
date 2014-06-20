#include "ys_job.h"
#include "ys_global.h"


ysJob::ysJob()
{    
    taskFile="";
    scanFile="";
    adjustmentFiles.clear();

    emailNotification="";
    accNumber="";
    patientName="Unknown";
    protocolName="Unknown";

    reconMode="";
    systemName="Unknown";

    uniqueID="";

    reconCallCmd="";
}


void ysJob::generateUniqueID()
{
    // TODO
}


bool ysJob::readTaskFile(QString filename)
{
    taskFile=filename;

    YS_OUT("Now processing file " + taskFile);

    return true;
}


QStringList ysJob::getAllFiles()
{
    QStringList allFiles;

    allFiles.clear();
    allFiles.append(taskFile);
    if (scanFile!="")
    {
        allFiles.append(scanFile);
    }
    allFiles.append(adjustmentFiles);

    return allFiles;
}
