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
    // TODO: Find improved way for generating uniqueID for task. For now, it's the filename.
    uniqueID=taskFile;
    uniqueID.truncate(uniqueID.indexOf("."));
}


bool ysJob::readTaskFile(QString filename)
{
    taskFile=filename;

    QString queueDir=YSRA->staticConfig.inqueuePath;

    // Scoping for lifetime of QSettings object, as the file might be moved later
    {
        QSettings taskSettings(queueDir+"/"+taskFile, QSettings::IniFormat);

        scanFile=taskSettings.value("Task/ScanFile", "!!FAIL").toString();
        reconMode=taskSettings.value("Task/ReconMode", "!!FAIL").toString();
        accNumber=taskSettings.value("Task/ACC", "").toString();
        emailNotification=taskSettings.value("Task/EMailNotification", "").toString();

        patientName=taskSettings.value("Task/PatientName", "Unknown").toString();
        protocolName=taskSettings.value("Task/ScanProtocol", "Unknown").toString();
        reconReadableName=taskSettings.value("Task/ReconName", "Unknown").toString();
        systemName=taskSettings.value("Information/SystemName", "Unknown").toString();

        submittedScanFileSize=taskSettings.value("Information/ScanFileSize", 0).toLongLong();

        QDate submDate=QDate::fromString(taskSettings.value("Information/TaskDate", QDate::currentDate().toString()).toString());
        QTime submTime=QTime::fromString(taskSettings.value("Information/TaskTime", QTime::currentTime().toString()).toString());
        submissionTime=QDateTime(submDate,submTime);

        adjustmentFiles.clear();
        int adjustCount=taskSettings.value("Task/AdjustmentFilesCount", 0).toInt();

        for (int i=0; i<adjustCount; i++)
        {
            QString adjustFile=taskSettings.value("AdjustmentFiles/"+QString(i), "").toString();
            adjustmentFiles.append(adjustFile);
        }
    }

    // Create ID that is used for the log file and mail notifications
    generateUniqueID();

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
        YS_SYSLOG_OUT("ERROR: The submitted task is missing input files.");
        YS_SYSLOG_OUT("ERROR: Task will not be processed and moved to fail directory.");
        return false;
    }

    // Check if the reconstruction mode is valid
    if (!YSRA->dynamicConfig.isReconModeAvailable(reconMode))
    {
        YS_SYSLOG_OUT("ERROR: The requested reconstruction mode is not available on this server.");
        YS_SYSLOG_OUT("ERROR: Cannot process the task. Moving task to fail directory.");
        return false;
    }

    // Now that it is likely that the file can be processed, create a task specific log
    YSRA->log.openTaskLog(uniqueID);

    // Dump some job information into the job file
    logJobInformation();

    // Remember when the job was started
    processingStart=QDateTime::currentDateTime();

    return true;
}


void ysJob::logJobInformation()
{
    YS_TASKLOG("Task information");
    YS_TASKLOG("----------------");

    YS_TASKLOG("ReconMode:    " + reconMode + " ("+ reconReadableName +")");
    YS_TASKLOG("Patient:      " + patientName);
    YS_TASKLOG("ACC:          " + accNumber);
    YS_TASKLOG("Protocol:     " + protocolName);
    YS_TASKLOG("System:       " + systemName);
    YS_TASKLOG("Submission:   " + submissionTime.toString());
    YS_TASKLOG("Notification: " + emailNotification);

    QString adjFileList="none";

    if (adjustmentFiles.count()>0)
    {
        adjFileList="";
        for (int i=0; i<adjustmentFiles.count(); i++)
        {
            adjFileList += adjustmentFiles.at(i)+" ";
        }
    }

    YS_TASKLOG("Adjustments:  " + adjFileList);
    YS_TASKLOG("");
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
