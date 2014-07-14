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

    bool moveTaskToWorkPath(ysJob* job);
    bool moveTaskToFailPath(ysJob* job, bool filesInQueue=false);
    bool moveTaskToStoragePath(ysJob* job);

    bool cleanWorkPath();

    static bool cleanPath(QString path);

    QString uniqueID;
    void generateUniqueID();

    void checkAndSendDiskSpaceNotification();
    QDateTime lastDiskSpaceNotification;
    bool      diskSpaceNotificationSent;

    bool isRequiredDiskSpaceAvailble();
    QDateTime lastDiskErrorNotification;
    bool      diskErrorNotificationSent;

    void createServerHaltFile();

protected:

    QDir queueDir;

    bool moveFiles(QStringList files, QString sourcePath, QString targetPath);
    bool isTaskFileLocked(QString taskFile);

    int getAvailSpaceGB(QString path);


};

#endif // YS_QUEUE_H
