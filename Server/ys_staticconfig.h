#ifndef YS_STATICCONFIG_H
#define YS_STATICCONFIG_H

#include <QtCore>

class ysStaticConfig
{
public:
    ysStaticConfig();

    QString modesPath;
    QString logPath;

    QString inqueuePath;
    QString workPath;
    QString failPath;
    QString storagePath;

    QString serverName;
    QString serverType;

    bool    notificationEnabled;
    QString notificationErrorMail;
    QString notificationFromAddress;

    double memkillThreshold;
    int    driveSpaceNeededGB;
    int    driveSpaceNotificationThresholdGB;

    bool readConfiguration();

    QString execPath;

};

#endif // YS_STATICCONFIG_H
