#include "ys_global.h"
#include "ys_queue.h"

#include "ys_server.h"
#include "ys_staticconfig.h"


ysQueue::ysQueue()
{
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
    bool invalidJob=false;
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

    if (taskFilename=="")
    {
        // No unlocked file found. Possibly, the only available file is locked right now.
        return 0;
    }

    ysJob* newJob=new ysJob;

    YS_SYSLOG_OUT("Starting processing task " + taskFilename);

    if(!newJob->readTaskFile(taskFilename))
    {
        // Reading task file was not successful
        moveTaskToFailPath(newJob, true);

        delete newJob;
        newJob=0; // It's important to return a null pointer!

        YS_SYSLOG_OUT("Job creation not successful.");
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
    QDir workDir(YSRA->staticConfig.workPath);

    QStringList filesToDelete=workDir.entryList(QDir::Files);
    for (int i=0; i<filesToDelete.count(); i++)
    {
        if (!workDir.remove(filesToDelete.at(i)))
        {
            // TODO: Error handling
        }
    }

    QStringList directoriesToDelete=workDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i=0; i<directoriesToDelete.count(); i++)
    {
        if (!workDir.rmdir(directoriesToDelete.at(i)))
        {
            // TODO: Error handling
        }
    }

    return true;
}



bool ysQueue::moveTaskToWorkPath(ysJob* job)
{
    if (!moveFiles(job->getAllFiles(), YSRA->staticConfig.inqueuePath, YSRA->staticConfig.workPath))
    {
        YS_SYSLOG_OUT("ERROR: Unable to move files to work directory.");
        return false;
    }

    return true;
}


bool ysQueue::moveTaskToFailPath(ysJob* job, bool filesInQueue)
{
    QString sourcePath=YSRA->staticConfig.workPath;
    if (filesInQueue)
    {
        sourcePath=YSRA->staticConfig.inqueuePath;
    }

    // TODO: Create own subdir in fail path. Check for existance and recreate with timestamp if exists.

    if (!moveFiles(job->getAllFiles(), sourcePath, YSRA->staticConfig.failPath))
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
        // TODO: Create own subdir in fail path. Check for existance and recreate with timestamp if exists.

        if (!moveFiles(job->getAllFiles(), YSRA->staticConfig.workPath, YSRA->staticConfig.storagePath))
        {
            YS_SYSLOG_OUT("ERROR: Unable to move files to storage directory.");
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




