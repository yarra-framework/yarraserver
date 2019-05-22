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
    bool moveTaskToResumePath(ysJob* job);

    bool cleanWorkPath();

    static bool cleanPath(QString path);
    void checkForCrashedTask();

    QString uniqueID;
    void generateUniqueID();

    void checkAndSendDiskSpaceNotification();
    QDateTime lastDiskSpaceNotification;
    bool      diskSpaceNotificationSent;

    bool isRequiredDiskSpaceAvailble();
    QDateTime lastDiskErrorNotification;
    bool      diskErrorNotificationSent;

    void createServerHaltFile();

    bool skipNightRecon(QString taskFilename, bool nightTimeReconAllowed);
    void changeTaskToNight(QString taskFilename);

    bool isTaskFileLocked(QString taskFile);
    bool lockTask(QString taskFile);
    bool unlockTask(QString taskFile);

    bool moveFolderRecurvisely(QString sourcePath, QString targetPath, int recursionLevel=0);

    QStringList getAllQueueEntries();

    bool   checkAndLockResumeCase(QString folder);
    ysJob* fetchResumeTask(QString folder);

protected:

    QDir queueDir;
    QDir prioqueueDir;
    QDir resumeDir;

    QStringList fileList;
    QString     resumeCase;

    bool isNightTime;

    QDateTime lastResumeCheck;

    bool displayedPermissionWarning;

    bool moveFiles(QStringList files, QString sourcePath, QString targetPath);

    int getAvailSpaceGB(QString path);

    QString getLockFilename(QString taskFilename);

};


#endif // YS_QUEUE_H

