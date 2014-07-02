#include "ys_process.h"
#include "ys_server.h"
#include "ys_global.h"
#include "ys_queue.h"

ysProcess::ysProcess()
{
    mode=0;
    callCmd="";
}


ysProcess::~ysProcess()
{
}


bool ysProcess::prepareReconstruction(ysJob* job)
{
    mode=new ysMode;
    mode->currentProcess=this;

    QString modeName=job->reconMode;
    if (!mode->readModeSettings(modeName, job))
    {
        YS_TASKLOG_OUT("ERROR: Unable to read mode file correctly for mode " + modeName);
        YS_TASKLOG_OUT("Check syntax of mode file.");
    }

    return true;
}


void ysProcess::finish()
{
    YS_FREE(mode);
}


bool ysProcess::runReconstruction()
{
    bool reconResult=true;

    YS_SYSTASKLOG_OUT("Now running reconstruction module...");

    callCmd=mode->getReconCmdLine();
    process.setWorkingDirectory(YSRA->staticConfig.workPath);

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

    QDir outDir(reconDir);
    if (outDir.entryList(QDir::Files).count()==0)
    {
        YS_SYSTASKLOG_OUT("ERROR: Reconstruction directory contains no files.");
        reconResult=false;
    }

    return reconResult;
}


bool ysProcess::runPostProcessing()
{
    // Check if there are any postprocessing modules at all
    if (mode->postprocCount==0)
    {
        return true;
    }

    bool postprocResult=true;

    for (int i=0; i<mode->postprocCount; i++)
    {
        YS_SYSTASKLOG_OUT("Now running post processing module " + QString::number(i) + " ...");

        callCmd=mode->getPostprocCmdLine(i);
        process.setWorkingDirectory(YSRA->staticConfig.workPath);

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

        if (!postprocResult)
        {
            break;
        }
    }

    return postprocResult;
}


bool ysProcess::runTransfer()
{
    // Just return if no transfer module has been defined.
    if (mode->transferBinary=="")
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


    bool transferResult=true;

    YS_SYSTASKLOG_OUT("Now running transfer module...");

    callCmd=mode->getTransferCmdLine();
    process.setWorkingDirectory(transferDir);

    if (!executeCommand())
    {
        YS_TASKLOG("Transfer failed.");
        transferResult=false;
    }
    else
    {
        YS_TASKLOG("Transfer finished.");
        transferResult=true;
    }

    YS_TASKLOG("Cleaning temporary files.");
    cleanTmpDir();

    return transferResult;
}


bool ysProcess::prepareOutputDirs()
{
    YS_TASKLOG("Preparing temporary working directories.");

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
        transferDir=dirName;
    }

    // Set transfer directory
    YS_TASKLOG("Input directory for transfer is " + transferDir);

    // Now, that the directories are known, ask the mode object
    // to parse and finalize the command lines
    mode->parseCmdlines();

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

    bool execResult=true;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(YS_EXEC_TIMEOUT);
    QEventLoop q;
    connect(&process, SIGNAL(finished(int , QProcess::ExitStatus)), &q, SLOT(quit()));
    connect(&process, SIGNAL(error(QProcess::ProcessError)), &q, SLOT(quit()));
    connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(logOutput()));
    connect(&timeoutTimer, SIGNAL(timeout()), &q, SLOT(quit()));

    // TODO: Add timer event to monitor memory usage

    // Time measurement to diagnose RaidTool calling problems
    QTime ti;
    ti.start();
    timeoutTimer.start();

    // Start the process. Note: The commandline and arguments need to be defined before.
    process.setReadChannel(QProcess::StandardOutput);
    process.start(callCmd);

    if (process.state()==QProcess::NotRunning)
    {
        YS_TASKLOG("ERROR: Process returned immediately.");
        YS_TASKLOG("Is the mode configuration correct?");
    }
    else
    {
        q.exec();
    }

    // Check for problems with the event loop: Sometimes it seems to return to quickly!
    // In this case, start a second while loop to check when the process is really finished.
    if ((timeoutTimer.isActive()) && (process.state()==QProcess::Running))
    {
        timeoutTimer.stop();
        YS_SYSTASKLOG_OUT("WARNING: QEventLoop returned too early. Starting secondary loop.");
        while ((process.state()==QProcess::Running) && (ti.elapsed()<YS_EXEC_TIMEOUT))
        {
            QCoreApplication::processEvents();
            YSRA->safeWait(YS_EXEC_LOOPSLEEP);
        }

        // If the process did not finish within the timeout duration
        if (process.state()==QProcess::Running)
        {
            YS_SYSTASKLOG_OUT("WARNING: Process is still active. Killing process.");
            process.kill();
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
            if (process.state()==QProcess::Running)
            {
                YS_SYSTASKLOG_OUT("WARNING: Process is still active. Killing process.");
                process.kill();
            }
        }
    }

    while (process.canReadLine())
    {
        YS_TASKLOGPROC(process.readLine());
    }

    YS_TASKLOG("##[EXEC END]######################################");
    YS_TASKLOG("");

    if (process.exitStatus()==QProcess::CrashExit)
    {
        YS_TASKLOG("ERROR: The process crashed.");
        execResult=false;
    }
    if (process.exitStatus()==QProcess::NormalExit)
    {
        YS_TASKLOG("The process exited normally with code " + QString::number(process.exitCode()));
    }

    if (YSRA->haltRequested)
    {
        YS_TASKLOG("NOTE: Task has been terminated on user request.");
    }

    return execResult;
}


void ysProcess::logOutput()
{
    while (process.canReadLine())
    {
        YS_TASKLOGPROC(process.readLine());
    }
}
