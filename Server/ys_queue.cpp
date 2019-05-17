#include "ys_global.h"
#include "ys_queue.h"

#include "ys_server.h"
#include "ys_staticconfig.h"

#include <sys/statvfs.h>


ysQueue::ysQueue()
{
    uniqueID="";
    isNightTime=true;
    fileList.clear();
    displayedPermissionWarning=false;
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
    prioqueueDir=queueDir;

    // Prepare the filter for the normal tasks
    QStringList taskFilter;
    taskFilter <<  QString("*")+QString(YS_TASK_EXTENSION);
    queueDir.setNameFilters(taskFilter);
    queueDir.setFilter(QDir::Files);
    queueDir.setSorting(QDir::Time | QDir::Reversed);

    // Prepare the filter for the priority tasks
    QStringList priotaskFilter;
    priotaskFilter <<  QString("*")+QString(YS_TASK_EXTENSION)+QString(YS_TASK_EXTENSION_PRIO);
    prioqueueDir.setNameFilters(priotaskFilter);
    prioqueueDir.setFilter(QDir::Files);
    prioqueueDir.setSorting(QDir::Time | QDir::Reversed);

    lastDiskSpaceNotification=QDateTime::currentDateTime();
    diskSpaceNotificationSent=false;

    lastDiskErrorNotification=QDateTime::currentDateTime();
    diskErrorNotificationSent=false;

    displayedPermissionWarning=false;

    // Remove the HALT file to inform other nodes that we are active
    if (QFile::exists(YSRA->staticConfig.inqueuePath+"/"+YS_HALT_FILE))
    {
        if (!QFile::remove(YSRA->staticConfig.inqueuePath+"/"+YS_HALT_FILE))
        {
            YS_SYSLOG_OUT("ERROR: Unable to remove HALT file from queue directory.");
        }
    }

    return true;
}


void ysQueue::createServerHaltFile()
{
    QFile haltFile(YSRA->staticConfig.inqueuePath+"/"+YS_HALT_FILE);
    if (!haltFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        YS_SYSLOG_OUT("ERROR: Unable to create HALT file in queue directory.");
    }
    else
    {
        QString fileContent="HALT";
        haltFile.write(fileContent.toLatin1());
        haltFile.flush();
    }
    haltFile.close();
}


bool ysQueue::isTaskAvailable()
{
    isNightTime=YSRA->staticConfig.allowNightReconNow();

    // TODO: Check for resume jobs (if exists, check if the delay time has exceeded)

    QStringList taskFilter;
    if (isNightTime)
    {
        // If it's night time (or if the night mode is not enabled), process tasks
        // with extension ".task" and ".task_night"
        taskFilter << QString("*")+QString(YS_TASK_EXTENSION) << QString("*")+QString(YS_TASK_EXTENSION)+QString(YS_TASK_EXTENSION_NIGHT);
    }
    else
    {
        // During daytime, only process tasks with extension ".task"
        taskFilter <<  QString("*")+QString(YS_TASK_EXTENSION);
    }
    queueDir.setNameFilters(taskFilter);

    prioqueueDir.refresh();
    queueDir.refresh();

    // First, process then prio recons, afterwards the normal ones
    fileList.clear();
    fileList=prioqueueDir.entryList();
    fileList.append(queueDir.entryList());

    if (fileList.count()>0)
    {
        return true;
    }

    return false;
}


ysJob* ysQueue::fetchTask()
{
    // TODO: Fetch resume task after checking that delay period has passed

    int fileCount=fileList.count();
    if (fileCount==0)
    {
        // File disappeared?
        return 0;
    }

    QString taskFilename="";
    int fetchIndex=0;

    while ((taskFilename.length()==0) && (fetchIndex<fileCount))
    {
        // Get task file for the next job to be processed
        taskFilename=fileList.at(fetchIndex);

        if (queueDir.exists(taskFilename))
        {
            bool permissionCorrect=true;

            {
                // Check if we the file permission are valid. We need read and write.
                QFileInfo fileInfo(queueDir.absoluteFilePath(taskFilename));
                permissionCorrect=fileInfo.isReadable() && fileInfo.isWritable();
            }

            if (!permissionCorrect)
            {
                if (!displayedPermissionWarning)
                {
                    YS_SYSLOG_OUT("");
                    YS_SYSLOG_OUT("WARNING: Found (at least) one file with incorrect permissions " + taskFilename);
                    YS_SYSLOG_OUT("WARNING: It will not be possible to process this case.");
                    YS_SYSLOG_OUT("WARNING: Check configuration of your Linux shares.");
                    YS_SYSLOG_OUT("");
                    displayedPermissionWarning=true;
                }
                taskFilename="";
            }
            else
            {
                if (isTaskFileLocked(taskFilename))
                {
                    // File is locked, so go on with the next older file
                    taskFilename="";
                }
                else
                {
                    lockTask(taskFilename);

                    // Check if the current task is a night task. If so and
                    // it is during the day, then go on to the next file
                    if (skipNightRecon(taskFilename, isNightTime))
                    {
                        unlockTask(taskFilename);
                        taskFilename="";
                    }
                }
            }
        }
        else
        {
            // Task might have been removed by load balancer, go to next file
            taskFilename="";
        }

        fetchIndex++;
    }

    if (taskFilename.length()==0)
    {
        // No file to process found. Possibly, the only available file is locked right now.
        return 0;
    }

    // Check available diskspace. Don't process any case if
    // it cannot guaranteed that enough space is available.
    if (!isRequiredDiskSpaceAvailble())
    {
        unlockTask(taskFilename);
        taskFilename="";
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
        unlockTask(taskFilename);

        delete newJob;
        newJob=0; // It's important to return a null pointer!

        YS_SYSLOG_OUT("Job creation not successful.\n");
        YS_SYSLOG_OUT(YS_WAITMESSAGE);
    }

    return newJob;
}


QString ysQueue::getLockFilename(QString taskFilename)
{
    QString lockFilename=taskFilename;
    lockFilename.truncate(lockFilename.indexOf("."));
    lockFilename+=YS_LOCK_EXTENSION;

    return YSRA->staticConfig.inqueuePath + "/" + lockFilename;
}


bool ysQueue::isTaskFileLocked(QString taskFile)
{
    return QFile::exists(getLockFilename(taskFile));
}


bool ysQueue::lockTask(QString taskFile)
{
    QString lockFilename=getLockFilename(taskFile);

    if (QFile::exists(lockFilename))
    {
        YS_SYSLOG("ERROR: Cannot lock file (lock file already exists) " + lockFilename);
        return false;
    }

    QFile lockFile(lockFilename);
    lockFile.open(QIODevice::ReadWrite | QIODevice::Text);
    QString placeHolder="YARRA";
    lockFile.write(placeHolder.toLatin1());
    lockFile.flush();
    lockFile.close();

    return true;
}


bool ysQueue::unlockTask(QString taskFile)
{
    QString lockFilename=getLockFilename(taskFile);

    if (QFile::exists(lockFilename))
    {
        if (!QFile::remove(lockFilename))
        {
            YS_SYSLOG("ERROR: Failed to remove lockfile: " + lockFilename);
            return false;
        }
    }

    return true;
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

    // TODO: Handle case if the job is a resume case

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

    // TODO: Copy subdir with reconstructed images, so that they can be re-sent to the PACS
    //       without running the whole reconstruction again

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
        affectedDirs += YSRA->staticConfig.failPath + "; ";
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

}


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
        affectedDirs += YSRA->staticConfig.failPath + "; ";
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


bool ysQueue::skipNightRecon(QString taskFilename, bool nightTimeReconAllowedNow)
{
    // If the night-reconstruction mode has been disabled, or if it is night right now,
    // do not skip the file and process it
    if (nightTimeReconAllowedNow)
    {
        return false;
    }

    // If the task is a prio task, then don't skip
    if (taskFilename.contains(QString(YS_TASK_EXTENSION)+QString(YS_TASK_EXTENSION_PRIO)))
    {
        return false;
    }

    QString reconMode="";
    // Scoping for lifetime of QSettings object, as the file might be moved later
    {
        QString fullFilename=YSRA->staticConfig.inqueuePath+"/"+taskFilename;

        if (!QFile::exists(fullFilename))
        {
            YS_SYSLOG_OUT("WARNING: Cannot read taskfile " + fullFilename);
        }

        QSettings taskSettings(fullFilename, QSettings::IniFormat);
        reconMode=taskSettings.value("Task/ReconMode", reconMode).toString();
    }

    // If it unable to determine the recon more, return and don't skip the processing
    if (reconMode.length()==0)
    {
        return false;
    }

    bool modeRestrictedToNight=false;
    QString modeFilename=YSRA->staticConfig.modesPath+"/"+reconMode+YS_MODE_EXTENSION;
    {
        QSettings modeFile(modeFilename, QSettings::IniFormat);
        modeRestrictedToNight=modeFile.value("Options/NightTask", modeRestrictedToNight).toBool();
    }

    if (!modeRestrictedToNight)
    {
        return false;
    }

    // OK, file should be reconstructed at night. Rename the task file
    // to .task_night so that it is prevented that it is reparsed all the time
    // until the night.
    changeTaskToNight(taskFilename);

    return true;
}


void ysQueue::changeTaskToNight(QString taskFilename)
{
    // Note: Lock file already exists here
    QString queuePath=YSRA->staticConfig.inqueuePath+"/";

    QString newFilename=taskFilename;
    newFilename.truncate(newFilename.indexOf("."));
    newFilename+=QString(YS_TASK_EXTENSION)+QString(YS_TASK_EXTENSION_NIGHT);
    if (!QFile::rename(queuePath+taskFilename, queuePath+newFilename))
    {
        YS_SYSLOG("ERROR: Unable to change task to night job!");
    }
    else
    {
        YS_SYSLOG_OUT("Changed task to night mode " + newFilename);
    }
}


void ysQueue::checkForCrashedTask()
{
    QDir lworkDir(YSRA->staticConfig.workPath);

    QStringList taskFilter;
    taskFilter << QString("*")+QString(YS_TASK_EXTENSION) << QString("*")+QString(YS_TASK_EXTENSION)+QString(YS_TASK_EXTENSION_PRIO)
               << QString("*")+QString(YS_TASK_EXTENSION)+QString(YS_TASK_EXTENSION_NIGHT);

    lworkDir.setNameFilters(taskFilter);
    lworkDir.refresh();

    QStringList lfileList;
    lfileList.clear();
    lfileList=lworkDir.entryList();

    if (lfileList.count()>0)
    {
        // Take the first task file from the list (assuming that there is only one task file)
        QString taskFilename=lfileList.at(0);

        // OK, a former crashed task exists in the work direcotory
        // Create a job and move all files to the fail directory
        YS_SYSLOG_OUT("WARNING: Found incomplete task in work directory from prior crash: " + taskFilename);

        // Generate a unique ID for this job based on the current time (incl ms to make it unique).
        // This time ID will be used throughout the job processing (the job object retrieves it
        // from the queue object during the setup of the job).
        generateUniqueID();

        YS_SYSLOG_OUT("Moving crashed task to fail directory.");
        ysJob* newJob=new ysJob;
        newJob->readTaskFile(taskFilename, true);
        moveTaskToFailPath(newJob, false);

        // Send error notification
        newJob->setErrorReason("Incomplete task found during server boot");
        YSRA->notification.sendErrorNotification(newJob);

        delete newJob;
        newJob=0;
    }

    // Finally, clean all files in the work directory
    cleanWorkPath();
}


bool ysQueue::moveTaskToResumePath(ysJob* job)
{
    QString sourcePath=YSRA->staticConfig.workPath;

    // Create own folder in resume path. Check for existance and recreate with timestamp if exists.
    QDir resumeDir(YSRA->staticConfig.resumePath);
    QString folderName=job->taskID;

    // Check if subfolder with same task already exists. If so, add unique time stamp.
    if (resumeDir.exists(folderName))
    {
        folderName+= "_" + uniqueID;
    }

    QString resumeFolder=YSRA->staticConfig.resumePath + "/" + folderName;

    // Try to create a subfolder for the failed task
    if (!resumeDir.mkdir(resumeFolder))
    {
        // Subfolder creation not successful, push files to base path
        YS_SYSLOG_OUT("ERROR: Unable to create directory in resume path "+folderName);
        return false;
    }
    else
    {
        YS_SYSLOG("Moving all task files into resume directory " + folderName);
    }

    // Now move files
    if (!moveFolderRecurvisely(sourcePath, resumeFolder))
    {
        YS_SYSLOG_OUT("ERROR: Unable to move files to resume directory.");
        return false;
    }

    return true;
}


// TODO: Test function!
bool ysQueue::moveFolderRecurvisely(QString sourcePath, QString targetPath, int recursionLevel)
{
    if (recursionLevel > 999)
    {
        // Prevent infinite recursion, just in case
        return false;
    }

    QDir dir;
    dir.setPath(sourcePath);
    dir.refresh();
    sourcePath += QDir::separator();
    targetPath += QDir::separator();

    //YS_OUT(QString("DBG: Moving from ") + sourcePath + " to " + targetPath);

    QStringList fileList=dir.entryList(QDir::Files);
    foreach (QString fileName, fileList)
    {
        QString sourceFile = sourcePath + fileName;
        QString targetFile = targetPath + fileName;

        //YS_OUT(QString("DBG: Moving file ") + sourceFile + " to " + targetFile);

        if (QFile::exists(targetFile))
        {
            // Error: File already exists. That should not be the case.
            return false;
        }

        if (!QFile::rename(sourceFile, targetFile))
        {
            // Error: Moving the file failed.
            return false;
        }
    }

    QStringList dirList=dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QString dirName, dirList)
    {
        QString sourceDir = sourcePath + dirName;
        QString targetDir = targetPath + dirName;

        if (!dir.mkpath(targetDir))
        {
            return false;
        }

        if (!moveFolderRecurvisely(sourceDir, targetDir, recursionLevel+1))
        {
            return false;
        }

        if (!dir.rmdir(sourceDir))
        {
            return false;
        }
    }

    return true;
}
