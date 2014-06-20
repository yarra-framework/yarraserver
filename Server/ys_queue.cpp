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
        YS_OUT("ERROR: Unable to change to queue directory.");
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

    if(!newJob->readTaskFile(taskFilename))
    {
        // Reading task file was not successful
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
    // TODO

    return true;
}


bool ysQueue::cleanStoragePath()
{
    // TODO

    return true;
}


bool ysQueue::moveTaskToWorkPath(ysJob* job)
{
    // TODO

    return true;
}


bool ysQueue::moveTaskToFailPath(ysJob* job)
{
    // TODO

    return true;
}


bool ysQueue::moveTaskToStoragePath(ysJob* job)
{
    /* TODO: Check if storing the raw data is desired
    if (job->)
    {
        // TODO
    }
    */

    return true;
}


bool moveFiles(QStringList files, QString sourcePath, QString targetPath)
{
    // TODO

    return true;
}




