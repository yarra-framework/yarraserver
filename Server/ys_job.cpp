#include "ys_job.h"
#include "ys_global.h"
#include "ys_server.h"
#include "ys_staticconfig.h"


ysJob::ysJob()
{    
    state=YS_STATE_INITIALIZED;
    type=YS_JOBTYPE_NEW;

    taskFile="";
    scanFile="";
    adjustmentFiles.clear();

    emailNotification="";
    accNumber="";
    patientName="Unknown";
    protocolName="Unknown";
    paramValue="N";

    reconMode="";
    systemName="Unknown";
    reconReadableName="Unknown";

    submittedScanFileSize=0;
    uniqueID="";

    reconCallCmd="";
    storeProcessedFile=false;
    durationSec=0;
    duration="none";

    submissionTime=QDateTime::currentDateTime();
    processingStart=submissionTime;
    processingEnd=submissionTime;

    resumeLog.clear();

    errorReason="Unknown - Refer to log file";
}


ysJob::~ysJob()
{
    YSRA->log.closeTaskLog();
}


void ysJob::generateTaskID()
{
    // TODO: Find improved way for generating uniqueID for task. For now, it's the filename.
    taskID=taskFile;
    taskID.truncate(taskID.indexOf("."));

    uniqueID=YSRA->queue.uniqueID;
}


bool ysJob::readTaskFile(QString filename, bool readCrashedTask)
{
    taskFile=filename;
    QString queueDir=YSRA->staticConfig.inqueuePath;

    // If a crashed task file was found in the work directory, read the
    // file from the work directory instead of the queue
    if (readCrashedTask)
    {
        queueDir=YSRA->staticConfig.workPath;
    }

    // Scoping for lifetime of QSettings object, as the file might be moved later
    {
        QSettings taskSettings(queueDir+"/"+taskFile, QSettings::IniFormat);

        scanFile=taskSettings.value("Task/ScanFile", "!!FAIL").toString();
        reconMode=taskSettings.value("Task/ReconMode", "!!FAIL").toString();
        accNumber=taskSettings.value("Task/ACC", "").toString();

        QStringList tempList=taskSettings.value("Task/EMailNotification", "").toStringList();
        emailNotification=tempList.join(",");

        paramValue=taskSettings.value("Task/ParamValue", "N").toString();

        patientName=taskSettings.value("Task/PatientName", "Unknown").toString();
        protocolName=taskSettings.value("Task/ScanProtocol", "Unknown").toString();
        reconReadableName=taskSettings.value("Task/ReconName", "Unknown").toString();
        systemName=taskSettings.value("Information/SystemName", "Unknown").toString();

        submittedScanFileSize=taskSettings.value("Information/ScanFileSize", 0).toLongLong();

        QDate submDate=QDate::fromString(taskSettings.value("Information/TaskDate", QDate::currentDate().toString(Qt::ISODate)).toString(),Qt::ISODate);
        QTime submTime=QTime::fromString(taskSettings.value("Information/TaskTime", QTime::currentTime().toString(Qt::ISODate)).toString(),Qt::ISODate);
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
    generateTaskID();

    // Conduct some tests if the task can be processed. This should not be done for previously
    // crashed files that were found in the work directory after a reboot of the server
    if (!readCrashedTask)
    {
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

            setErrorReason("Missing input files");
            YSRA->notification.sendErrorNotification(this);

            return false;
        }

        // Check if the reconstruction mode is valid
        if (!YSRA->dynamicConfig.isReconModeAvailable(reconMode))
        {
            YS_SYSLOG_OUT("ERROR: The requested reconstruction mode is not available on this server.");
            YS_SYSLOG_OUT("ERROR: Cannot process the task. Moving task to fail directory.");

            setErrorReason("Reconstruction mode not available");
            YSRA->notification.sendErrorNotification(this);

            return false;
        }

        // Now that it is likely that the file can be processed, create a task specific log
        YSRA->log.openTaskLog(taskID, uniqueID);

        // Dump some job information into the job file
        logJobInformation();

        // Remember when the job was started
        processingStart=QDateTime::currentDateTime();
    }

    return true;
}


void ysJob::logJobInformation()
{
    YS_TASKLOG("Task information");
    YS_TASKLOG("----------------");

    YS_TASKLOG("ReconMode:    " + reconMode + " ("+ reconReadableName +")");
    YS_TASKLOG("Patient:      " + patientName);
    YS_TASKLOG("ACC:          " + accNumber);
    YS_TASKLOG("ParamValue:   " + paramValue);
    YS_TASKLOG("Protocol:     " + protocolName);
    YS_TASKLOG("System:       " + systemName);
    YS_TASKLOG("Submission:   " + submissionTime.toString("dd.MM.yyyy hh:mm:ss"));
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


void ysJob::logResumeInformation()
{
    YS_TASKLOG("Resuming task after prior failure");
    YS_TASKLOG("Resuming state: " + getStateName());
    YS_TASKLOG("");
    YS_TASKLOG("Resume history");
    YS_TASKLOG("--------------");

    for (int i=0; i<resumeLog.count(); i++)
    {
        YS_TASKLOG(resumeLog.at(i));
    }

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


void ysJob::setProcessingEnd()
{
    processingEnd=QDateTime::currentDateTime();
    durationSec=processingStart.secsTo(processingEnd);

    // Convert duration in secs into a string
    if (durationSec<120)
    {
        duration=QString::number(durationSec) + " sec";
    }
    else
    {
        int sec=durationSec % 60;
        int min=(durationSec - sec)/60;

        if (min<120)
        {
            duration=QString::number(min) + " min " + QString::number(sec) + " sec";
        }
        else
        {
            int rmin=min % 60;
            int hour=(min - rmin)/60;
            duration=QString::number(hour) + " h " + QString::number(rmin) + " min " + QString::number(sec) + " sec";
        }
    }

    YS_TASKLOG("Duration of task: " + duration);
}


QString ysJob::toJson()
{
    QJsonObject object =
        QJsonObject::fromVariantMap(
            QVariantMap {
                {"patientName",   patientName},
                {"accNumber",     accNumber},
                {"reconMode",     reconMode},
                {"systemName",    systemName},
                {"protocolName",  protocolName},
                {"submissionTime",submissionTime.toString("dd.MM.yyyy hh:mm:ss")},
                {"errorReason",   errorReason},
                {"jobID",         taskID + "_" + uniqueID}
            }
        );
    QJsonDocument doc(object);
    QString strJson(doc.toJson(QJsonDocument::Compact));

    return strJson;
}


bool ysJob::writeResumeInformation(QString path)
{
    QString resumeFilename=path+"/"+taskID+YS_RESUME_EXTENSION;

    {
        QSettings resumeFile(resumeFilename, QSettings::IniFormat);

        int retries=resumeFile.value("Information/Retries", 0).toInt();

        QDateTime retryTime=QDateTime::currentDateTime();
        QDateTime nextRetry=retryTime.addSecs(YSRA->staticConfig.resumeDelayMin * 60);

        resumeFile.setValue("Information/Retries",   retries+1);
        resumeFile.setValue("Information/NextRetry", nextRetry);
        resumeFile.setValue("Information/State",     int(getState()));
        resumeFile.setValue("ResumeLog/Retry"+QString::number(retries), retryTime);
    }

    if (!QFile::exists(resumeFilename))
    {
        YS_SYSLOG_OUT("ERORR: Unable to store resume information.");
        return false;
    }

    return true;
}


bool ysJob::isFolderReadyForRetry(QString path)
{
    QDir dir(path);
    dir.refresh();

    // If the case is locked (i.e a lock file is present) then don't
    // do anything right now
    QStringList lockFilter;
    lockFilter << QString("*")+QString(YS_LOCK_EXTENSION);
    if (dir.entryList(lockFilter).count() > 0)
    {
        return false;
    }

    QStringList resumeFilter;
    resumeFilter << QString("*")+QString(YS_RESUME_EXTENSION);
    QStringList fileList=dir.entryList(resumeFilter);

    if (fileList.isEmpty())
    {
        // If no resume information is found, then try to process the case
        return true;
    }

    // Read the time from the resume file and compare with current time
    QSettings resumeFile(fileList.at(0), QSettings::IniFormat);
    QDateTime nextRetryTime=resumeFile.value("Information/NextRetry", QDateTime::currentDateTime()).toDateTime();

    if (nextRetryTime<=QDateTime::currentDateTime())
    {
        return true;
    }
    else
    {
        return false;
    }
}


int ysJob::getRetryCountFromFolder(QString path)
{
    QDir dir(path);
    dir.refresh();

    QStringList resumeFilter;
    resumeFilter << QString("*")+QString(YS_RESUME_EXTENSION);
    QStringList fileList=dir.entryList(resumeFilter);

    if (fileList.isEmpty())
    {
        // If no resume information is found, then try to process the case
        return 0;
    }

    // Open file and read the retry count
    QSettings resumeFile(fileList.at(0), QSettings::IniFormat);
    int retries=resumeFile.value("Information/Retries", 0).toInt();

    return retries;
}


bool ysJob::readResumeInformationFromFolder(QString path)
{
    QDir dir(path);
    dir.refresh();

    QStringList resumeFilter;
    resumeFilter << QString("*")+QString(YS_RESUME_EXTENSION);
    QStringList fileList=dir.entryList(resumeFilter);

    if (fileList.isEmpty())
    {
        // If no resume information is found, then try to process the case
        return false;
    }

    // Open file and read the retry count
    QSettings  resumeFile(fileList.at(0), QSettings::IniFormat);
    ysJobState resumeState=ysJobState(resumeFile.value("Information/State", 0).toInt());

    int retries=resumeFile.value("Information/Retries", 0).toInt();

    // Read the log of the previous attempts and store in the instance (will be written
    // into the task log file)
    resumeLog.clear();
    for (int i=0; i<retries; i++)
    {
        QString entry="Retry "+QString::number(i)+": " + resumeFile.value("ResumeLog/Retry"+QString::number(i),"").toString();
        resumeLog.append(entry);
    }

    // Set the state in which the task should be resumed
    state=resumeState;

    return true;
}
