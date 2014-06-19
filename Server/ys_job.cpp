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
    // TODO

    return true;
}
