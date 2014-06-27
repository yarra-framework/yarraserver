#ifndef YS_PROCESS_H
#define YS_PROCESS_H

#include "ys_job.h"


class ysProcess
{
public:
    ysProcess();

    bool runReconstruction(ysJob* job);
    bool runPostProcessing(ysJob* job);
    bool runTransfer(ysJob* job);

    bool prepareOutputDirs();
    bool cleanTmpDir();


    QString reconDir;
    QString transferDir;
    QString tmpDir;


};

#endif // YS_PROCESS_H
