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

    QString errorNotificationMail;

    bool readConfiguration();

};

#endif // YS_STATICCONFIG_H
