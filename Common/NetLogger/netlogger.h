// Note: This is the YarraServer version of the NetLogger interface. It does not use
//       domain validation because the configuration file is secured.


#ifndef NETLOGGER_H
#define NETLOGGER_H

#include <QString>
#include <QUrlQuery>
#include <QtNetwork>
#include "netlog_events.h"


class NetLogger
{
public:

    NetLogger();
    ~NetLogger();

    bool configure(QString path, EventInfo::SourceType sourceType, QString sourceId, QString key);
    bool isConfigured();
    bool isConfigurationError();

    QNetworkReply* postDataAsync(QUrlQuery query, QString endpt);
    QUrlQuery buildEventQuery(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data);

    bool postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status, QString &errorString, int timeoutMsec=NETLOG_POST_TIMEOUT);
    void postEvent    (EventInfo::Type type, EventInfo::Detail detail=EventInfo::Detail::Information, EventInfo::Severity severity=EventInfo::Severity::Success, QString info=QString(""), QString data=QString(""));
    bool postEventSync(EventInfo::Type type, EventInfo::Detail detail=EventInfo::Detail::Information, EventInfo::Severity severity=EventInfo::Severity::Success, QString info=QString(""), QString data=QString(""), int timeoutMsec=NETLOG_EVENT_TIMEOUT);
    bool postEventSync(QNetworkReply::NetworkError& error, int& status_code, EventInfo::Type type, EventInfo::Detail detail=EventInfo::Detail::Information, EventInfo::Severity severity=EventInfo::Severity::Success, QString info=QString(""), QString data=QString(""), int timeoutMsec=NETLOG_POST_TIMEOUT);

protected:

    bool configured;
    bool configurationError;

    QString serverPath;
    QString apiKey;
    QString source_id;
    EventInfo::SourceType source_type;

    QNetworkAccessManager* networkManager;
};


inline bool NetLogger::isConfigured()
{
    return configured;
}


inline bool NetLogger::isConfigurationError()
{
    return configurationError;
}


#endif // NETLOGGER_H
