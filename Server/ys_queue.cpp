#include "ys_global.h"
#include "ys_queue.h"

#include "ys_server.h"
#include "ys_staticconfig.h"

#include <sys/statvfs.h>


ysQueue::ysQueue()
{
    uniqueID="";
}


void ysQueue::generateUniqueID()
{
    uniqueID=QDate::currentDate().toString("ddMMyy")+QTime::currentTime().toString("HHmmsszzz");
}


bool ysQueue::prepare()
{
    if (!queueDir.cd(YSRA->staticConfig.inqueuePath))
    {
        YS_SYSLOG_OUT("ERROR: Unable to change to queue directory.");
        return false;
    }

    QStringList taskFilter;
    taskFilter <<  QString("*")+QString(YS_TASK_EXTENSION);
    queueDir.setNameFilters(taskFilter);
    queueDir.setFilter(QDir::Files);
    queueDir.setSorting(QDir::Time | QDir::Reversed);

    lastDiskSpaceNotification=QDateTime::currentDateTime();
    diskSpaceNotificationSent=false;

    lastDiskErrorNotification=QDateTime::currentDateTime();
    diskErrorNotificationSent=false;

    return true;
}


bool ysQueue::isTaskAvailable()
{
    queueDir.refresh();
    if (queueDir.entryList().count())
    {
        return true;
    }

    return false;
}


ysJob* ysQueue::fetchTask()
{
    QString taskFilename="";

    queueDir.refresh();
    int fileCount=queueDir.entryList().count();

    if (fileCount==0)
    {
        // File disappeared?
        return 0;
    }

    int fetchIndex=0;

    while ((taskFilename=="") && (fetchIndex<fileCount))
    {
        // Get task file for the next job to be processed
        taskFilename=queueDir.entryList().at(fetchIndex);

        if (isTaskFileLocked(taskFilename))
        {
            // File is locked, so go on with the next older file
            taskFilename="";
        }
    }

    // Check available diskspace. Don't process any case if
    // it cannot guaranteed that enough space is available.
    if (!isRequiredDiskSpaceAvailble())
    {
        taskFilename="";
    }

    if (taskFilename=="")
    {
        // No file to process found. Possibly, the only available file is locked right now,
        // or there is not enough diskspace available.
        return 0;
    }

    // Generate a unique ID for this job based on the current time (incl ms to make it unique).
    // This time ID will be used throughout the job processing (the job object retrieves it
    // from the queue object during the setup of the job).
    generateUniqueID();

    ysJob* newJob=new ysJob;

    YS_SYSLOG_OUT("Starting processing task " + taskFilename);

    if(!newJob->readTaskFile(taskFilename))
    {
        // Reading task file was not successful
        moveTaskToFailPath(newJob, true);

        delete newJob;
        newJob=0; // It's important to return a null pointer!

        YS_SYSLOG_OUT("Job creation not successful.\n");
        YS_SYSLOG_OUT(YS_WAITMESSAGE);
    }

    return newJob;
}


bool ysQueue::isTaskFileLocked(QString taskFile)
{
    QString lockFilename=taskFile;
    lockFilename.truncate(lockFilename.indexOf("."));
    lockFilename+=YS_LOCK_EXTENSION;

    return QFile::exists(YSRA->staticConfig.inqueuePath + "/" + lockFilename);
}


bool ysQueue::cleanWorkPath()
{
    bool deleteSuccess=cleanPath(YSRA->staticConfig.workPath);

    if (!deleteSuccess)
    {
        // Error handling
        YS_SYSLOG_OUT("ERROR: Some temporary files or folders in the work directory could not be removed.");
        YS_SYSLOG_OUT("ERROR: This might interfere with the following reconstruction task.");
    }


    return deleteSuccess;
}


bool ysQueue::cleanPath(QString path)
{
    bool deleteError=false;
    QDir deleteDir(path);

    // First remove all files
    QStringList filesToDelete=deleteDir.entryList(QDir::Files);
    for (int i=0; i<filesToDelete.count(); i++)
    {
        if (!deleteDir.remove(filesToDelete.at(i)))
        {
            deleteError=true;
        }
    }

    QStringList directoriesToDelete=deleteDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i=0; i<directoriesToDelete.count(); i++)
    {
        QDir subDir(path+"/"+directoriesToDelete.at(i));
        if (!subDir.removeRecursively())
        {
            deleteError=true;
        }
    }

    return !deleteError;
}




bool ysQueue::moveTaskToWorkPath(ysJob* job)
{
    YS_SYSLOG("Moving all task file to work directory.");

    if (!moveFiles(job->getAllFiles(), YSRA->staticConfig.inqueuePath, YSRA->staticConfig.workPath))
    {
        YS_SYSLOG_OUT("ERROR: Unable to move files to work directory.");
        return false;
    }

    return true;
}


bool ysQueue::moveTaskToFailPath(ysJob* job, bool filesInQueue)
{
    // Determine where the source files are located (depending on during which stage of the
    // processing the function was called)
    QString sourcePath=YSRA->staticConfig.workPath;
    if (filesInQueue)
    {
        sourcePath=YSRA->staticConfig.inqueuePath;
    }

    // Create own subdir in fail path. Check for existance and recreate with timestamp if exists.
    QDir failDir(YSRA->staticConfig.failPath);
    QString taskID=job->taskID;

    // Check if subfolder with same task already exists. If so, add unique time stamp.
    if (failDir.exists(taskID))
    {
        taskID+= "_" + uniqueID;
    }

    QString failSubdir=YSRA->staticConfig.failPath + "/" + taskID;

    // Try to create a subfolder for the failed task
    if (!failDir.mkdir(taskID))
    {
        // Subfolder creation not successful, push files to base path
        failSubdir=YSRA->staticConfig.failPath;
        YS_SYSLOG_OUT("ERROR: Unable to create directory in fail path. Storing files directly in fail path.");
    }
    else
    {
        YS_SYSLOG("Moving all task files into fail directory " + failSubdir);
    }

    // Now copy files
    if (!moveFiles(job->getAllFiles(), sourcePath, failSubdir))
    {
        YS_SYSLOG_OUT("ERROR: Unable to move files to fail directory.");
        return false;
    }

    return true;
}


bool ysQueue::moveTaskToStoragePath(ysJob* job)
{
    // Check if storing the raw data is desired
    if (job->storeProcessedFile)
    {
        // Create own subdir in storage path. Check for existance and recreate with timestamp if exists.
        QDir storageDir(YSRA->staticConfig.storagePath);
        QString taskID=job->taskID;

        // Check if subfolder with same task already exists. If so, add unique time stamp.
        if (storageDir.exists(taskID))
        {
            taskID+= "_" + uniqueID;
        }

        QString storageSubdir=YSRA->staticConfig.storagePath + "/" + taskID;

        // Try to create a subfolder for the failed task
        if (!storageDir.mkdir(taskID))
        {
            // Subfolder creation not successful, push files to base path
            storageSubdir=YSRA->staticConfig.storagePath;
            YS_SYSTASKLOG_OUT("ERROR: Unable to create directory in storage path. Storing files directly in storage path.");
        }
        else
        {
            YS_TASKLOG("Moving all task files into storage directory " + storageSubdir);
        }


        if (!moveFiles(job->getAllFiles(), YSRA->staticConfig.workPath, storageSubdir))
        {
            YS_SYSTASKLOG_OUT("ERROR: Unable to move files to storage directory.");
            return false;
        }
    }

    return true;
}


bool ysQueue::moveFiles(QStringList files, QString sourcePath, QString targetPath)
{
    bool copyError=false;

    for (int i=0; i<files.count(); i++)
    {
        QString sourceFile=sourcePath+"/"+files.at(i);
        QString targetFile=targetPath+"/"+files.at(i);

        if (!QFile::rename(sourceFile, targetFile))
        {
            YS_SYSLOG("ERROR: Error performing copy " + sourceFile + " to " + targetFile);
            copyError=true;
        }

    }

    return !copyError;
}



int ysQueue::getAvailSpaceGB(QString path)
{
    struct statvfs64 fiData;

    QByteArray byteArray = path.toUtf8();

    if((statvfs64(byteArray.constData(),&fiData)) < 0)
    {
        YS_SYSLOG_OUT("Failed to stat " + path);
        return -1;
    }
    else
    {
        return qint64(fiData.f_bsize)*qint64(fiData.f_bavail)/(1024*1024*1024);
    }
}


void ysQueue::checkAndSendDiskSpaceNotification()
{
    int spaceThreshold=YSRA->staticConfig.driveSpaceNotificationThresholdGB;

    bool triggerNotification=false;
    QString affectedDirs="";

    if (getAvailSpaceGB(YSRA->staticConfig.inqueuePath)<spaceThreshold)
    {
        triggerNotification=true;
        affectedDirs += YSRA->staticConfig.inqueuePath + "; ";
    }

    if (getAvailSpaceGB(YSRA->staticConfig.workPath)<spaceThreshold)
    {
        triggerNotification=true;
        affectedDirs += YSRA->staticConfig.workPath + "; ";
    }

    if (getAvailSpaceGB(YSRA->staticConfig.failPath)<spaceThreshold)
    {
        triggerNotification=true;
        affectedDirs += YSRA->staticConfig.workPath + "; ";
    }

    if (getAvailSpaceGB(YSRA->staticConfig.storagePath)<spaceThreshold)
    {
        triggerNotification=true;
        affectedDirs += YSRA->staticConfig.storagePath + "; ";
    }

    if (triggerNotification)
    {
        YS_SYSLOG_OUT("WARNING: Low diskspace.");
        YS_SYSLOG("The following directories are affected:\n" + affectedDirs);
    }

    // Don't send notification mails more often than every 12 hours
    int notificationInterval=60*60*12;

    if ((triggerNotification) &&
        ((diskSpaceNotificationSent==false) || (lastDiskSpaceNotification.secsTo(QDateTime::currentDateTime())>notificationInterval)))
    {
        YSRA->notification.sendDiskSpaceNotification(affectedDirs);
        lastDiskSpaceNotification=QDateTime::currentDateTime();
        diskSpaceNotificationSent=true;
    }

}\


bool ysQueue::isRequiredDiskSpaceAvailble()
{
    int errorThreshold=YSRA->staticConfig.driveSpaceNeededGB;

    bool triggerError=false;
    QString affectedDirs="";

    // Check the following dirs: workdir, faildir, storagepath
    if (getAvailSpaceGB(YSRA->staticConfig.workPath)<errorThreshold)
    {
        triggerError=true;
        affectedDirs += YSRA->staticConfig.workPath + "; ";
    }

    if (getAvailSpaceGB(YSRA->staticConfig.failPath)<errorThreshold)
    {
        triggerError=true;
        affectedDirs += YSRA->staticConfig.workPath + "; ";
    }

    if (getAvailSpaceGB(YSRA->staticConfig.storagePath)<errorThreshold)
    {
        triggerError=true;
        affectedDirs += YSRA->staticConfig.storagePath + "; ";
    }

    // Send error notification to the admin, but not more often
    // than once every 4 hours
    int errorNotificationInterval=60*60*4;

    if ((triggerError) &&
        ((diskErrorNotificationSent==false) || (lastDiskErrorNotification.secsTo(QDateTime::currentDateTime())>errorNotificationInterval)))
    {
        YS_SYSLOG_OUT("Error: Insufficient diskspace to process case.");
        YS_SYSLOG("The following directories are affected:\n" + affectedDirs);

        YSRA->notification.sendDiskErrorNotification(affectedDirs);
        lastDiskErrorNotification=QDateTime::currentDateTime();
        diskErrorNotificationSent=true;
    }

    // Return false if in any of the folders is less space free than the
    // configured space limit
    return (!triggerError);
}
