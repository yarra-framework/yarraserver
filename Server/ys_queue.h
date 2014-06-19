#ifndef YS_QUEUE_H
#define YS_QUEUE_H

#include "ys_job.h"

class ysQueue
{
public:
    ysQueue();

    bool prepare();

    bool isTaskAvailable();

    ysJob* fetchTask();

    bool cleanWorkPath();
    bool cleanStoragePath();

    bool moveTaskToWorkPath(ysJob* job);
    bool moveTaskToFailPath(ysJob* job);
    bool moveTaskToStoragePath(ysJob* job);

};

#endif // YS_QUEUE_H
