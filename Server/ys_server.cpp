#include "ys_server.h"
#include "ys_global.h"
#include "ys_log.h"



ysServer::ysServer(QObject *parent) :
    QObject(parent)
{
    controlInterface.setParent(this);
    shutdownRequested=false;
    haltRequested=false;
    currentJob=0;
}


void ysServer::run()
{
    if (prepare())
    {
        runLoop();
    }

    quit();
}


void ysServer::quit()
{
    emit finished();
}


void ysServer::aboutToQuitApp()
{
}



bool ysServer::prepare()
{
    YS_OUT("Yarra Server - Version " + QString(YS_VERSION));
    YS_OUT("===========================");
    YS_OUT("");

    if (!staticConfig.readConfiguration())
    {
        YS_OUT("ERROR: Reading server configuration failed.");
        YS_OUT("Cannot launch server. Shutting down.");

        return false;
    }

    // Start the system log
    log.openSysLog();
    YS_SYSLOG_OUT("Initializing server...");

    // Prepare the dynamic configuration
    if (!dynamicConfig.prepare())
    {
        YS_SYSLOG_OUT("Cannot launch server. Shutting down.");
        return false;
    }

    if (!dynamicConfig.validateAllReconModes())
    {
        YS_SYSLOG_OUT("ERROR: Some reconstruction modes have not been configured correctly.");
        YS_SYSLOG_OUT("Please check configuration. Shutting down.");

        return false;
    }

    if (!queue.prepare())
    {
        YS_SYSLOG_OUT("ERROR: Preparing queing directories failed.");
        YS_SYSLOG_OUT("Please check installation and permissions. Shutting down.");

        return false;
    }

    return true;
}


bool ysServer::runLoop()
{
    shutdownRequested=false;


    // Check if controlInterface setup was successful
    if (!controlInterface.prepare())
    {
        YS_SYSLOG_OUT("Initialization of the server not successful.");
        YS_SYSLOG_OUT("Is the server already running?");
        YS_OUT("");
        YS_OUT("If the server is not running, this behavior might result from a previous crash.");
        YS_OUT("Start the server with parameter --force to enforce a restart.");

        return false;
    }


    YS_OUT("Server running (threadID " + QString::number((long)this->thread()->currentThreadId()) + ")");


    while (!shutdownRequested)
    {
        // Read the current configuration of reconstruction modes
        dynamicConfig.updateDynamicConfigList();

        if (queue.isTaskAvailable())
        {
            // Ask the queue to fetch the next task to be processed and
            // create a job instance for the task
            currentJob=queue.fetchTask();

            if (currentJob)
            {
                // ## TODO: Integrate error handling!!!

                // Delete all temporary files
                queue.cleanWorkPath();

                // Move all related files to work directory
                queue.moveTaskToWorkPath(currentJob);

                // Run the process
                processor.prepareOutputDirs();
                processor.runReconstruction(currentJob);
                processor.runPostProcessing(currentJob);
                processor.runTransfer      (currentJob);

                // If requested by mode, move raw data to storage location
                queue.moveTaskToStoragePath(currentJob);

                // Delete all temporary files
                queue.cleanWorkPath();

                // Discard the current job
                YS_FREE(currentJob);

                YS_SYSLOG_OUT("Job finished.");
            }
        }

        // Sleep for 10ms to prevent excessive CPU usage during idle times
        safeWait(10);
    }

    controlInterface.finish();

    YS_SYSLOG_OUT("Server stopped.");

    return true;
}



void ysServer::forceHalt()
{
    haltRequested=true;
    shutdownRequested=true;
}

