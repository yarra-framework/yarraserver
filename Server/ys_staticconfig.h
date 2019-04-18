#ifndef YS_STATICCONFIG_H
#define YS_STATICCONFIG_H

#include <QtCore>

class ysStaticConfig
{
public:
    ysStaticConfig();

    QString serverName;

    QString modesPath;
    QString logPath;
    QString inqueuePath;
    QString workPath;
    QString failPath;
    QString storagePath;
    QString modulesPath;
    QString modulesUserPath;
    QString resumePath;

    QString matlabBinary;

    bool    notificationEnabled;
    QString notificationErrorMail;
    QString notificationFromAddress;
    QString notificationDomainRestriction;

    double  memkillThreshold;
    int     driveSpaceNeededGB;
    int     driveSpaceNotificationThresholdGB;
    int     processTimeout;

    bool    useNightTasks;
    QTime   nightStart;
    QTime   nightEnd;
    bool    nightAfterMidnight;

    bool    terminateAfterOneTask;

    bool    resumeTasks;
    int     resumeDelayMin;

    bool    readConfiguration();
    bool    checkDirectories();
    bool    allowNightReconNow();

    QString execPath;
    QString logServer;
    QString logServerKey;
    int heartbeatSecs;
};

#endif // YS_STATICCONFIG_H
