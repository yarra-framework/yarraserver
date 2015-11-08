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

    QString matlabBinary;

    bool    notificationEnabled;
    QString notificationErrorMail;
    QString notificationFromAddress;
    QString notificationDomainRestriction;


    double memkillThreshold;
    int    driveSpaceNeededGB;
    int    driveSpaceNotificationThresholdGB;

    int processTimeout;

    bool useNightTasks;
    QTime nightStart;
    QTime nightEnd;
    bool nightAfterMidnight;

    bool readConfiguration();
    bool checkDirectories();

    QString execPath;

    bool allowNightReconNow();

};

#endif // YS_STATICCONFIG_H
