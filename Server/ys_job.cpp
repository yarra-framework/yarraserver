#include "ys_job.h"
#include "ys_global.h"
#include "ys_server.h"
#include "ys_staticconfig.h"


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
    reconReadableName="Unknown";

    submittedScanFileSize=0;
    uniqueID="";

    reconCallCmd="";
    storeProcessedFile=false;
}


ysJob::~ysJob()
{
    YSRA->log.closeTaskLog();
}


void ysJob::generateUniqueID()
{
    // TODO
}


bool ysJob::readTaskFile(QString filename)
{
    taskFile=filename;

    QString queueDir=YSRA->staticConfig.inqueuePath;

    QSettings taskSettings(queueDir+"/"+taskFile, QSettings::IniFormat);

    scanFile=taskSettings.value("Task/ScanFile", "").toString();
    reconMode=taskSettings.value("Task/ReconMode", "").toString();
    accNumber=taskSettings.value("Task/ACC", "").toString();
    emailNotification=taskSettings.value("Task/EMailNotification", "").toString();

    patientName=taskSettings.value("Task/PatientName", "Unknown").toString();
    protocolName=taskSettings.value("Task/ScanProtocol", "Unknown").toString();
    reconReadableName=taskSettings.value("Task/ReconName", "Unknown").toString();
    systemName=taskSettings.value("Information/SystemName", "Unknown").toString();

    submittedScanFileSize=taskSettings.value("Information/ScanFileSize", 0).toLongLong();

    submissionTime.setDate(taskSettings.value("Information/TaskDate", QDate::currentDate()).toDate());
    submissionTime.setTime(taskSettings.value("Information/TaskTime", QTime::currentTime()).toTime());

    adjustmentFiles.clear();
    int adjustCount=taskSettings.value("Task/AdjustmentFilesCount", 0).toInt();

    for (int i=0; i<adjustCount; i++)
    {
        QString adjustFile=reconReadableName=taskSettings.value("AdjustmentFiles/"+QString(i), "").toString();
        adjustmentFiles.append(adjustFile);
    }

    QDir queue(queueDir);

    bool fileMissing=false;

    // Check if all files are there
    QStringList allFiles=getAllFiles();
    for (int i=0; i<allFiles.count(); i++)
    {
        if (!queue.exists(allFiles.at(i)))
        {
            YS_SYSLOG_OUT("ERROR: File is missing " + allFiles.at(i));
            fileMissing=true;
        }
    }

    if (fileMissing)
    {
        // TODO: Error handling!
        YS_SYSLOG_OUT("ERROR: The submitted task is missing input files.");
        YS_SYSLOG_OUT("ERROR: Task will not be processed and moved to fail directory.");
        return false;
    }

    // TODO: Check if the reconstruction mode is valid.


    // TODO: Find improved way for generating uniqueID for task. For now, it's the filename.
    uniqueID=taskFile;
    uniqueID.truncate(uniqueID.indexOf("."));

    // Now that it is likely that the file can be processed, create a task specific log
    YSRA->log.openTaskLog(uniqueID);

    // TODO: Dump some job information into the job file

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
