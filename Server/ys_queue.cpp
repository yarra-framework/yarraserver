#include "ys_queue.h"

ysQueue::ysQueue()
{
}


bool ysQueue::prepare()
{
    // TODO

    return true;
}


bool ysQueue::isTaskAvailable()
{
    return false;
}


ysJob* ysQueue::fetchTask()
{
    bool invalidJob=false;

    // TODO: Get task file for the next job to be processed
    QString taskFilename="";

    ysJob* newJob=new ysJob;

    if(!newJob->readTaskFile(taskFilename))
    {
        // Reading task file was not successful
    }

    return newJob;
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


