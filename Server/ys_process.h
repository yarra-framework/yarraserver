#ifndef YS_PROCESS_H
#define YS_PROCESS_H

#include "ys_job.h"


class ysProcess
{
public:
    ysProcess();

    bool runReconstruction(ysJob* job);
    bool runPostProcessing(ysJob* job);
    bool runStorage(ysJob* job);

};

#endif // YS_PROCESS_H
