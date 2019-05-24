#include "ys_process.h"
#include "ys_server.h"
#include "ys_global.h"
#include "ys_queue.h"

#include <unistd.h>


ysProcess::ysProcess()
{
    mode=0;
    callCmd="";
    memcheckTimer=0;
    memkillThreshold=-1;
    totalPhysicalMemory=1;
    memoryDuringKill=0;
    disableMemKill=false;
    terminationReason=YS_TERMINATION_NONE;
    maxOutputIdleTime=YS_EXEC_MAXOUTPUTIDLE;
    process=0;
    processWorkingDirectory="";
    currentModuleType="";
    errorReceived=false;
}


ysProcess::~ysProcess()
{
}


bool ysProcess::prepareReconstruction(ysJob* job)
{
    getPhysicalMemoryMB();

    mode=new ysMode;
    mode->currentProcess=this;

    QString modeName=job->reconMode;
    if (!mode->readModeSettings(modeName, job))
    {
        YS_TASKLOG_OUT("ERROR: Unable to read mode file correctly for mode " + modeName);
        YS_TASKLOG_OUT("Check syntax of mode file.");

        YSRA->currentJob->setErrorReason("Unable to read mode file");
        return false;
    }

    return true;
}


void ysProcess::finish()
{
    YS_FREE(mode);
}


bool ysProcess::runPreProcessing()
{
    // Check if there are any preprocessing modules at all
    if (mode->preprocCount==0)
    {
        return true;
    }

    disableMemKill=mode->preprocDisableMemKill;
    maxOutputIdleTime=mode->preprocMaxOutputIdle;
    currentModuleType="PreProcessing";

    bool preprocResult=true;

    for (int i=0; i<mode->preprocCount; i++)
    {
        YS_SYSTASKLOG_OUT("Now running pre processing module " + QString::number(i+1) + "...");

        callCmd=mode->getPreprocCmdLine(i);
        processWorkingDirectory=YSRA->staticConfig.workPath;

        if (!executeCommand())
        {
            YS_TASKLOG("Pre processing failed.");
            preprocResult=false;
        }
        else
        {
            YS_TASKLOG("Pre processing finished.");
            preprocResult=true;
        }

        YS_TASKLOG("Cleaning temporary files.");
        cleanTmpDir();

        YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::Information, preprocResult? EventInfo::Severity::Success : EventInfo::Severity::Error, "preprocessing " + QString::number(i), callCmd);

        if (!preprocResult)
        {
            break;
        }
    }

    YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::End, preprocResult? EventInfo::Severity::Success : EventInfo::Severity::Error, "preprocessing", "");

    return preprocResult;
}


bool ysProcess::runReconstruction()
{
    bool reconResult=true;

    YS_SYSTASKLOG_OUT("Now running reconstruction module...");

    callCmd=mode->getReconCmdLine();
    processWorkingDirectory=YSRA->staticConfig.workPath;

    disableMemKill=mode->reconDisableMemKill;
    maxOutputIdleTime=mode->reconMaxOutputIdle;
    currentModuleType="Reconstruction";

    if (!executeCommand())
    {
        YS_TASKLOG("Reconstruction failed.");
        reconResult=false;
    }
    else
    {
        YS_TASKLOG("Reconstruction finished.");
        reconResult=true;
    }

    YS_TASKLOG("Cleaning temporary files.");
    cleanTmpDir();

    if (reconResult==true)
    {
        QDir outDir(reconDir);
        if (outDir.entryList(QDir::Files).count()==0)
        {
            YS_SYSTASKLOG_OUT("ERROR: Reconstruction directory contains no files.");
            YSRA->currentJob->setErrorReason("Reconstruction created no files");
            reconResult=false;
        }
    }

    YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::Information, reconResult? EventInfo::Severity::Success : EventInfo::Severity::Error, "reconstruction", callCmd);

    return reconResult;
}


bool ysProcess::runPostProcessing()
{
    // Check if there are any postprocessing modules at all
    if (mode->postprocCount==0)
    {
        return true;
    }

    disableMemKill=mode->postprocDisableMemKill;
    maxOutputIdleTime=mode->postprocMaxOutputIdle;
    currentModuleType="PostProcessing";

    bool postprocResult=true;

    for (int i=0; i<mode->postprocCount; i++)
    {
        YS_SYSTASKLOG_OUT("Now running post processing module " + QString::number(i+1) + "...");

        callCmd=mode->getPostprocCmdLine(i);
        processWorkingDirectory=YSRA->staticConfig.workPath;

        if (!executeCommand())
        {
            YS_TASKLOG("Post processing failed.");
            postprocResult=false;
        }
        else
        {
            YS_TASKLOG("Post processing finished.");
            postprocResult=true;
        }

        YS_TASKLOG("Cleaning temporary files.");
        cleanTmpDir();

        YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::Information, postprocResult? EventInfo::Severity::Success : EventInfo::Severity::Error, "postprocessing " +  QString::number(i), callCmd);

        if (!postprocResult)
        {
            break;
        }
    }

    YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::End, postprocResult? EventInfo::Severity::Success : EventInfo::Severity::Error, "postprocessing", "");

    return postprocResult;
}


bool ysProcess::runTransfer()
{
    // Just return if no transfer module has been defined.
    if (mode->transferCount==0)
    {
        return true;
    }

    // Check if there are files to transfer at all
    QDir outDir(transferDir);
    if (outDir.entryList(QDir::Files).count()==0)
    {
        // Notify about absence of files, but still call transfer module for any case
        YS_SYSTASKLOG_OUT("WARNING: Transfer directory contains no files.");
    }

    disableMemKill=mode->transferDisableMemKill;
    maxOutputIdleTime=mode->transferMaxOutputIdle;
    currentModuleType="Transfer";

    bool transferResult=true;

    for (int i=0; i<mode->transferCount; i++)
    {
        YS_SYSTASKLOG_OUT("Now running transfer module " + QString::number(i+1) + " ...");

        processWorkingDirectory=transferDir;
        callCmd=mode->getTransferCmdLine(i);

        if (!executeCommand())
        {
            YS_TASKLOG("Transfer failed.");
            transferResult=false;
        }
        else
        {
            YS_TASKLOG("Transfer finished.");
        }

        YS_TASKLOG("Cleaning temporary files.");
        cleanTmpDir();

        YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::Information, transferResult? EventInfo::Severity::Success : EventInfo::Severity::Error, "transfer",callCmd);
    }

    YSRA->netLogger.postEventSync(EventInfo::Type::Processing, EventInfo::Detail::End, transferResult? EventInfo::Severity::Success : EventInfo::Severity::Error, "transfer","");

    return transferResult;
}


bool ysProcess::prepareOutputDirs(ysJob::ysJobState jobState)
{
    YS_TASKLOG("Preparing temporary working directories.");

    // NOTE: When resuming jobs, the subfolder in the work directory might already exist.
    //       Therefore, the function does not check for return of false.

    QDir workDir;
    workDir.cd(YSRA->staticConfig.workPath);
    workDir.mkdir(YS_WORKDIR_TMP);
    workDir.mkdir(YS_WORKDIR_RECON);

    reconDir=YSRA->staticConfig.workPath + "/" + YS_WORKDIR_RECON;
    tmpDir  =YSRA->staticConfig.workPath + "/" + YS_WORKDIR_TMP;

    transferDir=reconDir;

    // Prepare the directories for postprocessing
    for (int i=0; i<mode->postprocCount; i++)
    {
        QString dirName=YS_WORKDIR_POSTPROC+QString::number(i+1);
        workDir.mkdir(dirName);
        transferDir=YSRA->staticConfig.workPath + "/" + dirName;
    }

    // Set transfer directory
    YS_TASKLOG("Input directory for transfer is " + transferDir);

    // Now, that the directories are known, ask the mode object
    // to parse and finalize the command lines
    mode->parseCmdlines();

    if (jobState!=ysJob::YS_STATE_INITIALIZED)
    {
        // If job is resumed, then clean the temp folder
        cleanTmpDir();

        if (jobState==ysJob::YS_STATE_POSTPROCESSING)
        {
            // If the job is resumed in the postprocessing state (i.e. all postproc modules are executed
            // again), clean all output folder for the postproc modules. The last one will be the transfer
            // folder. If postproc modules create files in the work folder directly, these files remain.
            for (int i=0; i<mode->postprocCount; i++)
            {
                QString dirName=YSRA->staticConfig.workPath + "/" + YS_WORKDIR_POSTPROC+QString::number(i+1);
                if (!ysQueue::cleanPath(dirName))
                {
                    YS_TASKLOG("WARNING: Unable to clean folder for resuming task " + dirName);
                }
            }
        }
    }

    return true;
}


bool ysProcess::cleanTmpDir()
{
    if (!ysQueue::cleanPath(tmpDir))
    {
        YS_TASKLOG("ERROR: Unable to remove files from temporary folder.");
        YS_TASKLOG("ERROR: This might cause problems for the following processing step.");
        return false;
    }

    return true;
}


bool ysProcess::executeCommand()
{
    YS_TASKLOG("Executing command " + callCmd);
    YS_TASKLOG("");
    YS_TASKLOG("##[EXEC START]####################################");

    // Create clean new QProcess object
    process=new QProcess();
    process->setWorkingDirectory(processWorkingDirectory);

    bool execResult=true;
    terminationReason=YS_TERMINATION_NONE;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(YSRA->staticConfig.processTimeout);    

    memcheckTimer=new QTimer();
    memcheckTimer->setSingleShot(false);
    memcheckTimer->setInterval(YS_EXEC_MEMCHECK);
    totalPhysicalMemory=getPhysicalMemoryMB();
    memoryDuringKill=0;
    memkillThreshold=YSRA->staticConfig.memkillThreshold;

    if (disableMemKill)
    {
        memkillThreshold=-1;
    }

    // Initialize process monitoring variables
    outputLines=0;
    lastOutput=time(0);
    errorReceived=false;

    QEventLoop q;
    connect(process, SIGNAL(finished(int , QProcess::ExitStatus)), &q, SLOT(quit()));
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(receiveProcessError(QProcess::ProcessError)));
    connect(process, SIGNAL(error(QProcess::ProcessError)), &q, SLOT(quit()));
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(logOutput()));
    connect(&timeoutTimer, SIGNAL(timeout()), &q, SLOT(quit()));
    connect(memcheckTimer, SIGNAL(timeout()), this, SLOT(checkMemory()));

    // Time measurement to diagnose RaidTool calling problems
    QTime ti;
    ti.start();
    timeoutTimer.start();

    // Start the process. Note: The commandline and arguments need to be defined before.
    process->setReadChannel(QProcess::StandardOutput);
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(callCmd);

    if (process->state()==QProcess::NotRunning)
    {
        YS_TASKLOG("ERROR: Process returned immediately.");
        YS_TASKLOG("Is the mode configuration correct?");
        YSRA->currentJob->setErrorReason("Process did not start");
    }
    else
    {
        memcheckTimer->start();
        q.exec();
    }

    // Check for problems with the event loop: Sometimes it seems to return to quickly!
    // In this case, start a second while loop to check when the process is really finished.
    if ((timeoutTimer.isActive()) && (process->state()==QProcess::Running))
    {
        timeoutTimer.stop();
        YS_SYSTASKLOG_OUT("WARNING: QEventLoop returned too early. Starting secondary loop.");
        while ((process->state()==QProcess::Running) && (ti.elapsed()<YS_EXEC_TIMEOUT))
        {
            QCoreApplication::processEvents();
            YSRA->safeWait(YS_EXEC_LOOPSLEEP);
        }

        // If the process did not finish within the timeout duration
        if (process->state()==QProcess::Running)
        {
            YS_SYSTASKLOG_OUT("WARNING: Process is still active. Killing process.");
            YSRA->currentJob->setErrorReason("Process timed out");
            process->kill();
            execResult=false;
        }
        else
        {
            execResult=true;
        }
    }
    else
    {
        // Normal timeout-handling if QEventLoop works normally
        if (timeoutTimer.isActive())
        {
            execResult=true;
            timeoutTimer.stop();
        }
        else
        {
            YS_SYSTASKLOG_OUT("WARNING: Process event loop timed out.");
            YS_SYSTASKLOG_OUT("WARNING: Duration since start "+QString::number(ti.elapsed())+" ms");
            execResult=false;
            if (process->state()==QProcess::Running)
            {
                YS_SYSTASKLOG_OUT("WARNING: Process is still active. Killing process.");
                process->kill();
            }
        }
    }

    // Read the remaining output from the process
    logOutput();

    YS_TASKLOG("##[EXEC END]######################################");
    YS_TASKLOG("");

    if (errorReceived)
    {
        // Error was received via signal during runtime. Possibly, command line not valid.
        // Error message written by slot, does not have to be repeated here.
        execResult=false;
    }

    if (process->exitStatus()==QProcess::CrashExit)
    {
        YS_TASKLOG("ERROR: The process crashed.");
        YSRA->currentJob->setErrorReason("Process crashed (" + currentModuleType +")");
        execResult=false;

        if (YSRA->shutdownRequested)
        {
            YS_TASKLOG("NOTE: Shutdown was requested previously.");
            YS_TASKLOG("NOTE: Possibly the process exceeded the upstart killing timout.");
        }
    }
    if ((execResult) && (process->exitStatus()==QProcess::NormalExit))
    {
        if (process->exitCode()!=0)
        {
            execResult=false;
            YS_TASKLOG("ERROR: The process exited normally, but reported an error (code " + QString::number(process->exitCode()) + ").");
            YSRA->currentJob->setErrorReason("Error reported by module (" + currentModuleType +")");
        }
        else
        {
            YS_TASKLOG("The process exited normally.");
        }
    }

    if (YSRA->haltRequested)
    {
        YS_TASKLOG("NOTE: Task has been terminated on user request.");
        YSRA->currentJob->setErrorReason("Process terminated on request");
    }

    if (terminationReason==YS_TERMINATION_MEMKILL)
    {
        YS_TASKLOG("ERROR: Task has been killed due to lack of memory.");
        YS_TASKLOG("ERROR: Used memory when killed was " + QString::number(memoryDuringKill) + "Mb ("+QString::number(totalPhysicalMemory)+" MB physical memory).");
        YSRA->currentJob->setErrorReason("Out of memory");
    }

    if (terminationReason==YS_TERMINATION_OUTPUTLIMIT)
    {
        YS_TASKLOG("ERROR: The task has been killed due to too many output lines.");
        YSRA->currentJob->setErrorReason("Terminated due to infinite loop");
    }

    if (terminationReason==YS_TERMINATION_OUTPUTTIMEOUT)
    {
        YS_TASKLOG("ERROR: The task has been killed due to inactivity.");
        YSRA->currentJob->setErrorReason("Terminated due to inactivity");
    }

    YS_FREE(memcheckTimer);
    YS_FREE(process);

    return execResult;
}


void ysProcess::logOutput()
{
    if (process==0)
    {
        return;
    }

    while (process->canReadLine())
    {
        // Read the current line, but restrict the maximum length to 512 chars to
        // avoid infinite output (if a module starts outputting binary data)
        YS_TASKLOGPROC(process->readLine(512));

        // Remember how many lines were outputed. Enables detecting if the process
        // creates an unlimited number of lines (infinite loop).
        outputLines++;

        // Kill the process if too many lines were created (assuming infinite loop)
        if (outputLines>YS_EXEC_MAXOUTPUTLINES)
        {
            YS_SYSTASKLOG_OUT("WARNING: Process created too much output. Assuming infinite loop.");
            YS_SYSTASKLOG_OUT("WARNING: Killing process.");
            process->kill();
            terminationReason=YS_TERMINATION_OUTPUTLIMIT;
        }
    }

    // Remember at what time the last output was created. Allows shutting down processes
    // after too long idle time
    lastOutput=time(0);
}


int ysProcess::getUsedMemoryMB()
{
    if (process==0)
    {
        return 0;
    }

    // TODO: Potentially also analyze for zombie state of the process

    QFile memfile(QString("/proc/"+QString::number(process->pid())+"/status"));

    if ((!memfile.open(QIODevice::ReadOnly)) || (!memfile.isReadable()))
    {
        YS_SYSTASKLOG_OUT("WARNING: Can't read memfile. Memkill will not work.");
        return -1;
    }

    QByteArray contents = memfile.readAll();
    memfile.close();

    QTextStream in(&contents);

    int usedMem=-1;

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (line.contains("VmSize:"))
        {
            line.remove(0,7);
            line.truncate(line.indexOf(" kB"));
            usedMem=line.toInt()/1024;
            break;
        }
    }

    return usedMem;
}


int ysProcess::getPhysicalMemoryMB()
{
    int totalMem=qint64(sysconf(_SC_PAGESIZE)) * qint64(sysconf(_SC_PHYS_PAGES)) / (1024*1024);
    return totalMem;
}


void ysProcess::checkMemory()
{
    if (process==0)
    {
        return;
    }

    memcheckTimer->stop();

    int usedMem=getUsedMemoryMB();

    if (usedMem>0)
    {
        double percent=100;
        if  (totalPhysicalMemory!=0)
        {
            percent=double(usedMem)/double(totalPhysicalMemory)*100.;
        }

        //YS_OUT("##MEMCHECK## " + QString::number(usedMem) + " = " + QString::number(int(percent)) + "%");

        if ((memkillThreshold>0) && (percent>memkillThreshold))
        {
            YS_SYSTASKLOG_OUT("WARNING: Memory usage is high ("+ QString::number(usedMem) + " Mb).");
            YS_SYSTASKLOG_OUT("WARNING: Killing process due to lack of memory.");
            process->kill();
            terminationReason=YS_TERMINATION_MEMKILL;
            memoryDuringKill=usedMem;
        }
    }

    // Check duration since last output from module (unless check is disabled).
    // Terminate process if the module didn't respond for certain time.

    if ((maxOutputIdleTime>0) && (difftime(time(0),lastOutput)>maxOutputIdleTime))
    {        
        YS_SYSTASKLOG_OUT("WARNING: Process did not create output for too long time (" + QString::number(difftime(time(0),lastOutput)) + " sec)");
        YS_SYSTASKLOG_OUT("WARNING: Theshold time is " + QString::number(maxOutputIdleTime) + "sec.");
        YS_SYSTASKLOG_OUT("WARNING: Assuming hanging process. Killing task.");
        process->kill();
        terminationReason=YS_TERMINATION_OUTPUTTIMEOUT;
    }

    memcheckTimer->start();
}


void ysProcess::haltAnyProcess()
{
    if (process==0)
    {
        return;
    }

    if (process->state()==QProcess::Running)
    {
        process->kill();
    }
}


void ysProcess::receiveProcessError(QProcess::ProcessError processError)
{
    YS_TASKLOG("ERROR: Process error detected.");
    errorReceived=true;

    if (processError==QProcess::FailedToStart)
    {
        YS_TASKLOG("ERROR: Process failed to start");
        YS_TASKLOG("ERROR: Is the mode configuration (binary path) correct?");
        YSRA->currentJob->setErrorReason("Process did not start");
    }

    if (processError==QProcess::Crashed)
    {
        YS_TASKLOG("ERROR: Process crashed");
        YSRA->currentJob->setErrorReason("Process crashed (" + currentModuleType +")");
    }
}
