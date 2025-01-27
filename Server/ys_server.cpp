#include "ys_server.h"
#include "ys_global.h"
#include "ys_log.h"
#include "ys_controlapi.h"


ysServer::ysServer(QObject *parent) :
    QObject(parent)
{
    controlInterface.setParent(this);
    shutdownRequested=false;
    haltRequested=false;
    currentJob=0;
    returnCode=0;
}


void ysServer::run()
{
    if (prepare())
    {
        runLoop();
    }
    else
    {
        netLogger.postEventSync(EventInfo::Type::Boot, EventInfo::Detail::Information, EventInfo::Severity::FatalError, "Initialization of the server not successful",QVariantMap{{"error", errorReason}}, 5000);
        returnCode=1;
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
    YS_OUT("YarraServer - Version " + QString(YS_VERSION));
    YS_OUT("============================");
    YS_OUT("");
    YS_OUT("Build: " + QString(__DATE__) + " " + QString(__TIME__));
    YS_OUT("");

    if (!staticConfig.readConfiguration())
    {
        YS_OUT("ERROR: Reading server configuration failed.");
        YS_OUT("Cannot launch server. Shutting down.");
        return false;
    }

    // Check if all directories are available
    if (!staticConfig.checkDirectories())
    {
        setErrorReason("Directories unavailable");
        return false;
    }

    // Start the system log
    log.openSysLog();
    YS_SYSLOG_OUT("Initializing server...");

    if (staticConfig.terminateAfterOneTask)
    {
        YS_SYSLOG_OUT("Server will terminate after one task.");
    }

    // Connect to the logserver if configured
    if (!staticConfig.logServerAddress.isEmpty())
    {
        if (!netLogger.configure(staticConfig.logServerAddress, EventInfo::SourceType::Server, staticConfig.serverName, staticConfig.logServerAPIKey))
        {
            YS_SYSLOG_OUT("Could not connect to logserver at "+ staticConfig.logServerAddress);
            return false;
        }
    }

    // Prepare the dynamic configuration
    if (!dynamicConfig.prepare())
    {
        YS_SYSLOG_OUT("Cannot launch server. Shutting down.");
        setErrorReason("Dynamic configuration error");
        return false;
    }

    if (!dynamicConfig.validateAllReconModes())
    {
        YS_SYSLOG_OUT("ERROR: Some reconstruction modes have not been configured correctly.");
        YS_SYSLOG_OUT("Please check configuration. Shutting down.");
        setErrorReason("Invalid reconstruction modes");
        return false;
    }

    if (!queue.prepare())
    {
        YS_SYSLOG_OUT("ERROR: Preparing queing directories failed.");
        YS_SYSLOG_OUT("Please check installation and permissions. Shutting down.");
        setErrorReason("unable to prepare queue");
        return false;
    }

    // Prepare the notification mailer
    YS_OUT("Prepare notification mailer...");
    notification.prepare();
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
        netLogger.postEventSync(EventInfo::Type::Boot, EventInfo::Detail::Information, EventInfo::Severity::Error, "controlInterface not successful",QVariantMap{{"error", errorReason}}, 5000);

        returnCode=1;
        return false;
    }

    YS_OUT("Server running (threadID " + QString::number((long)this->thread()->currentThreadId()) + ")");
    YS_SYSLOG_OUT("");

    // Send server information to the logserver (if configured)
    if (netLogger.isConfigured())
    {
        QProcess hostnameProcess;
        hostnameProcess.start("hostname -A");
        hostnameProcess.waitForFinished();
        QString output(hostnameProcess.readAllStandardOutput());

        QVariantMap bootInfo  {
            {"version", YS_VERSION},
            {"build", QString(__DATE__) + " " + QString(__TIME__) },
            {"exec_path", staticConfig.execPath },
            {"hostname", output.trimmed()},
            {"recon_modes", QJsonArray::fromStringList(dynamicConfig.availableReconModes)}
        };

        QNetworkReply::NetworkError networkError;
        int networkStatusCode=0;
        QString errorString;
        bool result = netLogger.postEventSync(networkError, networkStatusCode,errorString, EventInfo::Type::Boot, EventInfo::Detail::Information, EventInfo::Severity::Success, "", bootInfo, 5000);
        if (!result) {
            YS_SYSLOG_OUT("Boot message failed");
            YS_SYSLOG_OUT(QString(networkStatusCode));
            YS_SYSLOG_OUT(errorString);
        }
    } else {
        YS_SYSLOG_OUT("NetLogger is not configured.");
    }

    queue.checkAndSendDiskSpaceNotification();

    // Check if files from a former reconstruction exist in the work directory. This could be
    // the case if the server was powered off while a reconstruction was running.
    queue.checkForCrashedTask();

    // In the terminate-after-task mode, default the return code to error to ensure that
    // all error scenarios are captured. If the task is completed successfully, it will
    // be set to zero.
    if (staticConfig.terminateAfterOneTask)
    {
        returnCode=1;
    }

    YS_SYSLOG_OUT(YS_WAITMESSAGE);

    // Setup timer to send queue information regularly to logserver
    QTimer *logServerQueueTimer = new QTimer(this);

    if (netLogger.isConfigured()) {
        logServerQueueTimer->setInterval(1000*staticConfig.heartbeatSecs);
        connect(logServerQueueTimer, &QTimer::timeout, [=]() {
            QVariantMap queue_info{
                {"queue", queue.getAllQueueEntries()},
                {"job_state", currentJob? currentJob->getState() : -1 }
            };
            netLogger.postEventSync(EventInfo::Type::Heartbeat, EventInfo::Detail::Information, EventInfo::Severity::Success, "",queue_info, 100);
        });
        logServerQueueTimer->start();
    }

    while (!shutdownRequested)
    {


        status=YS_CTRL_IDLE;

        if (queue.isTaskAvailable())
        {
            // Read the current configuration of reconstruction modes
            dynamicConfig.updateDynamicConfigList();

            // Ask the queue to fetch the next task to be processed and
            // create a job instance for the task
            currentJob=queue.fetchTask();

            if (currentJob)
            {
                netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::Start, EventInfo::Severity::Success, "job start",QVariantMap{{"job",currentJob->getUniqueTaskID()},{"job_info",currentJob->toJson()}}, 5000);

                status=log.getTaskLogFilename();
                bool procError=false;

                if (currentJob->getType()==ysJob::YS_JOBTYPE_NEW)
                {
                    // Delete all temporary files (should be clean, but it case the server
                    // previously crashed)
                    queue.cleanWorkPath();

                    // Move all related files to work directory
                    queue.moveTaskToWorkPath(currentJob);
                    queue.unlockTask(currentJob->taskFile);
                }

                // ## Run the individual processing modules
                procError=!processJob();

                if (procError)
                {
                    bool saveResumeState=false;

                    if (staticConfig.resumeTasks)
                    {
                        // Decide if the job should be stored for later resume
                        if ((currentJob->getState()==ysJob::YS_STATE_TRANSFER)
                           || (currentJob->getState()==ysJob::YS_STATE_POSTPROCESSING))
                        {
                            saveResumeState=true;
                        }
                    }

                    if (saveResumeState)
                    {
                        // Check if job has already been resumed previously and abort if
                        // max number of retries has been reached
                        int previousRetries=ysJob::getRetryCountFromFolder(YSRA->staticConfig.workPath);

                        if (previousRetries>YSRA->staticConfig.resumeMaxRetries)
                        {
                            YS_SYSTASKLOG("Maximum number of task retries reached.");
                            YS_SYSTASKLOG("Declaring task as failed.");

                            saveResumeState=false;
                        }
                        else
                        {
                            YS_SYSTASKLOG("Storing intermediate results for later retry.");

                            // Save the task state in the resume folder
                            if (!queue.moveTaskToResumePath(currentJob))
                            {
                                // Saving the resume state didn't work, so move the task to the fail folder instead
                                saveResumeState=false;
                            }
                        }
                    }

                    if (!saveResumeState)
                    {
                        // Move raw data to the fail location
                        queue.moveTaskToFailPath(currentJob);
                        YS_SYSTASKLOG("ERROR: Processing of task was not successful.");
                        notification.sendErrorNotification(currentJob);
                    }
                    else
                    {
                        YS_SYSTASKLOG("WARNING: Processing of task was not successful. Job will be suspended and resumed later.");
                    }
                }
                else
                {
                    // If requested by mode, move raw data to storage location
                    queue.moveTaskToStoragePath(currentJob);

                    YS_SYSTASKLOG("Processing of task was successful.");
                    notification.sendSuccessNotification(currentJob);

                    YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::End, EventInfo::Severity::Success, "job complete",QVariantMap{{"job",currentJob->getUniqueTaskID()},{"stage","complete"},{"job_info",currentJob->toJson()}}, 5000);

                    // In the terminate-after-task mode, the default return code is 1.
                    // Explicitly set it to 0 to allow detection that the task was successful.
                    if (staticConfig.terminateAfterOneTask)
                    {
                        returnCode=0;
                    }
                }

                // Delete all temporary files
                YS_TASKLOG("Removing all created files.");
                queue.cleanWorkPath();

                // Discard the current job
                YS_FREE(currentJob);

                // Check the current diskspace situation and notify the admin
                // if the server is out of storage capacity
                queue.checkAndSendDiskSpaceNotification();

                YS_SYSLOG_OUT("Job has finished.\n");

                // Check if system log is too large. If so, rename and create empty file
                log.limitSysLogSize();

                if ((!shutdownRequested) && (!staticConfig.terminateAfterOneTask))
                {
                    YS_SYSLOG_OUT(YS_WAITMESSAGE);
                }
            }
        }

        // If only one task should be processed per server run, shutdown the server now
        if (staticConfig.terminateAfterOneTask)
        {
            shutdownRequested=true;
        }

        // Sleep for 50ms to prevent excessive CPU usage during idle times
        safeWait(50);
    }
    YS_SYSLOG_OUT("Shutdown requested. Server going down.");

    logServerQueueTimer->stop();
    logServerQueueTimer->deleteLater();

    queue.createServerHaltFile();
    controlInterface.finish();

    YSRA->netLogger.postEventSync(EventInfo::Type::Shutdown, EventInfo::Detail::Information, EventInfo::Severity::Success, "","");
    YS_SYSLOG_OUT("Server stopped.");

    return true;
}


bool ysServer::processJob()
{
    bool success=true;

    // 1. Preparation (has to be run by new and resumed jobs)
    if (!processor.prepareReconstruction(currentJob))
    {
        YS_SYSTASKLOG_OUT("Processing preparation not successful.");
        currentJob->setErrorReason("Processing preparation not successful.");
        success=false;
    }

    if ((success) && (!haltRequested))
    {
        if (!processor.prepareOutputDirs(currentJob->getState()))
        {
            YS_SYSTASKLOG_OUT("Preparing working directories not successful.");
            currentJob->setErrorReason("Preparing working directories not successful.");
            success=false;
        }
        else
        {
            // Don't change the job state for resumed jobs
            if (currentJob->getType()==ysJob::YS_JOBTYPE_NEW)
            {
                currentJob->setState(ysJob::YS_STATE_PREPARED);
            }
        }
    }

    // 2. Preprocessing modules
    if ((success) && (!haltRequested) && (currentJob->getState()<=ysJob::YS_STATE_PREPROCESSING))
    {
        currentJob->setState(ysJob::YS_STATE_PREPROCESSING);

        if (!processor.runPreProcessing())
        {
            YS_SYSTASKLOG_OUT("Running pre-processing modules failed.");
            currentJob->setErrorReason("Running pre-processing modules failed.");
            success=false;
        }
    }

    // 3. Reconstruction module
    if ((success) && (!haltRequested) && (currentJob->getState()<=ysJob::YS_STATE_RECONSTRUCTION))
    {
        currentJob->setState(ysJob::YS_STATE_RECONSTRUCTION);

        if (!processor.runReconstruction())
        {
            YS_SYSTASKLOG_OUT("Running reconstruction module failed.");
            currentJob->setErrorReason("Running reconstruction module failed.");
            success=false;
        }
    }

    // 4. Postprocessing modules
    if ((success) && (!haltRequested) && (currentJob->getState()<=ysJob::YS_STATE_POSTPROCESSING))
    {
        currentJob->setState(ysJob::YS_STATE_POSTPROCESSING);

        if (!processor.runPostProcessing())
        {
            YS_SYSTASKLOG_OUT("Running post-processing modules failed.");
            currentJob->setErrorReason("Running post-processing module failed.");
            success=false;
        }
    }

    // 5. Transfer module
    if ((success) && (!haltRequested) && (currentJob->getState()<=ysJob::YS_STATE_TRANSFER))
    {
        currentJob->setState(ysJob::YS_STATE_TRANSFER);

        if (!processor.runTransfer())
        {
            YS_SYSTASKLOG_OUT("Running transfer module failed.");
            currentJob->setErrorReason("Running transfer module failed.");
            success=false;
        }
        else
        {
            currentJob->setState(ysJob::YS_STATE_COMPLETE);
        }
    }

    // 6. Clean up
    processor.finish();
    currentJob->setProcessingEnd();

    return success;
}


void ysServer::forceHalt()
{
    haltRequested=true;
    shutdownRequested=true;
    processor.haltAnyProcess();
}

