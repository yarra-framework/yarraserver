#include "ys_process.h"
#include "ys_server.h"
#include "ys_global.h"
#include "ys_queue.h"

ysProcess::ysProcess()
{
}


bool ysProcess::runReconstruction(ysJob* job)
{
    // TODO

    cleanTmpDir();

    return true;
}


bool ysProcess::runPostProcessing(ysJob* job)
{
    // TODO

    cleanTmpDir();

    return true;
}


bool ysProcess::runTransfer(ysJob* job)
{
    // TODO

    cleanTmpDir();

    return true;
}


bool ysProcess::prepareOutputDirs()
{
    QDir workDir;
    workDir.cd(YSRA->staticConfig.workPath);
    workDir.mkdir(YS_WORKDIR_TMP);
    workDir.mkdir(YS_WORKDIR_RECON);

    reconDir=YSRA->staticConfig.workPath + "/" + YS_WORKDIR_RECON;
    tmpDir  =YSRA->staticConfig.workPath + "/" + YS_WORKDIR_TMP;

    // YS_WORKDIR_POSTPROC

}


bool ysProcess::cleanTmpDir()
{
    if (!ysQueue::cleanPath(tmpDir))
    {
        YS_TASKLOG("ERROR: Unable to remove files from temporary folder.");
        YS_TASKLOG("ERROR: This might cause problems for the following processing step.");
    }
}

