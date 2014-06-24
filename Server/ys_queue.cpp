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
    queueDir.setSorting(QDir::Time);

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
        YS_SYSLOG_OUT("Job creation not successful.");
    }

    return newJob;
}


bool ysQueue::isTaskFileLocked(QString taskFile)
{
    QString lockFilename=taskFile;
    lockFilename.truncate(lockFilename.indexOf("."));
    lockFilename+=YS_LOCK_EXTENSION;

    QLockFile lockFile(YSRA->staticConfig.inqueuePath + "/" + lockFilename);

    return lockFile.isLocked();
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
        // TODO: Error handling
        return false;
    }

    return true;
}


bool ysQueue::moveTaskToFailPath(ysJob* job)
{
    if (!moveFiles(job->getAllFiles(), YSRA->staticConfig.workPath, YSRA->staticConfig.failPath))
    {
        // TODO: Error handling
        return false;
    }

    return true;
}


bool ysQueue::moveTaskToStoragePath(ysJob* job)
{
    // Check if storing the raw data is desired
    if (job->storeProcessedFile)
    {
        if (!moveFiles(job->getAllFiles(), YSRA->staticConfig.workPath, YSRA->staticConfig.storagePath))
        {
            // TODO: Error handling
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
            // TODO: Error handling
            copyError=true;
        }

    }

    return copyError;
}




