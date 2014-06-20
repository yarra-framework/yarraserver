#ifndef YS_QUEUE_H
#define YS_QUEUE_H

#include "ys_job.h"
#include <QtCore>


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

protected:

    QDir queueDir;

    bool moveFiles(QStringList files, QString sourcePath, QString targetPath);
    bool isTaskFileLocked(QString taskFile);


};

#endif // YS_QUEUE_H
