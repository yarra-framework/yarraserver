// Note: This is the YarraServer version of the NetLogger interface. It does not use
//       domain validation because the configuration file is secured.


#include "netlogger.h"
#include "ys_global.h"
#include "ys_server.h"

#include <iostream>


NetLogger::NetLogger()
{
    configured=false;
    configurationError=false;

    serverPath="";
    apiKey="";
    source_id="";
    source_type=EventInfo::SourceType::Generic;

    // Try to load the SSL certificate for communication with the LogServer
    // if provided.
    QString certificateFilename=qApp->applicationDirPath() + "/logserver.crt";

    if (QFile::exists(certificateFilename))
    {
        // Read certificate from external file
        QFile certificateFile(certificateFilename);

        // Read the certificate if possible and then add it
        if (certificateFile.open(QIODevice::ReadOnly))
        {
            const QByteArray certificateContent=certificateFile.readAll();

            // Create a certificate object
            const QSslCertificate certificate(certificateContent);

            // Add this certificate to all SSL connections
            QSslSocket::addDefaultCaCertificate(certificate);
            YS_OUT("Loaded logserver certificate from file.");
            YS_OUT("");
        }
        else
        {
            YS_OUT("Unable to load logserver certificate.");
            YS_OUT("");
        }
    }

    networkManager=new QNetworkAccessManager();
}


NetLogger::~NetLogger()
{
    if (networkManager!=0)
    {
        delete networkManager;
        networkManager=0;
    }
}


bool NetLogger::configure(QString path, EventInfo::SourceType sourceType, QString sourceId, QString key)
{
    configured=false;
    configurationError=false;
    serverPath=path;
    source_id=sourceId;
    source_type=sourceType;
    apiKey=key;
    configured=true;
    return true;
//    if ((!path.isEmpty()) && (!key.isEmpty()))
//    {
//        configured=false;
//    }

    return configured;
}


QUrlQuery NetLogger::buildEventQuery(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data)
{
    QUrlQuery query;

    // Send the API key if it has been entered
    if (!apiKey.isEmpty())
    {
        query.addQueryItem("api_key",apiKey);
    }

    // ip and time are filled in on the server
    query.addQueryItem("ip",            "0");
    query.addQueryItem("time",          "0");
    query.addQueryItem("info",          info);
    query.addQueryItem("data",          data);
    query.addQueryItem("type",          QString::number((int)type));
    query.addQueryItem("detail",        QString::number((int)detail));
    query.addQueryItem("severity",      QString::number((int)severity));
    query.addQueryItem("source_id",     source_id);
    query.addQueryItem("source_type",   QString::number((int)source_type));

    return query;
}


void NetLogger::postEvent(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data)
{
    if (!configured)
    {
        return;
    }

    QUrlQuery query=buildEventQuery(type,detail,severity,info,data);
    postDataAsync(query,NETLOG_ENDPT_EVENT);
}


bool NetLogger::postEventSync(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data, int timeoutMsec)
{
    QNetworkReply::NetworkError networkError;
    int networkStatusCode=0;
    return postEventSync(networkError, networkStatusCode, type, detail, severity, info, data, timeoutMsec);
}

bool NetLogger::postEventSync(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QVariantMap data, int timeoutMsec) {
    QJsonObject object = QJsonObject::fromVariantMap(data);
    QJsonDocument doc(object);
    QString dataString(doc.toJson(QJsonDocument::Compact));
    return postEventSync(type, detail, severity, info, dataString, timeoutMsec);
}


bool NetLogger::postEventSync(QNetworkReply::NetworkError& error, int& status_code, EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data, int timeoutMsec)
{
    if (!configured)
    {
        return false;
    }

    QUrlQuery query=buildEventQuery(type,detail,severity,info,data);

    QString errorString="";
    return postData(query,NETLOG_ENDPT_EVENT,error,status_code,errorString,timeoutMsec);
}


QNetworkReply* NetLogger::postDataAsync(QUrlQuery query, QString endpt)
{
    if (serverPath.isEmpty())
    {
        return 0;
    }

    QUrl serviceUrl = QUrl("https://" + serverPath + "/" + endpt);
    serviceUrl.setScheme("https");
    //YS_SYSLOG_OUT(serviceUrl.toString());

    QNetworkRequest req(serviceUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QUrl params;
    params.setQuery(query);
    QByteArray postData = params.toEncoded(QUrl::RemoveFragment);
    postData = postData.remove(0,1); // pop off extraneous "?"

    return networkManager->post(req,postData);
}


// This posts some data by urlencoding is, so that's why the parameter is a URLQuery.
// It returns true if and only if it recieves an HTTP 200 OK response. Otherwise, there's either a network error
// or, if the network succeeded but the server failed, an HTTP status code.

bool NetLogger::postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status, QString &errorString, int timeoutMsec)
{    
    if (!configured)
    {
        errorString="NetLogger not configured";
        return false;
    }

    http_status=0;
    errorString="Unknown";
    bool timeout=false;

    QNetworkReply* reply=postDataAsync(query,endpt);

    if (!reply)
    {
        errorString="No QNetworkReply pointer received";
        return false;
    }

    // Use eventloop to wait until post event has finished. Event loop will timeout after 20sec
    QEventLoop eventLoop;
    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    QTimer::singleShot(timeoutMsec, &eventLoop, SLOT(quit()));

    if (reply->isRunning())
    {
        eventLoop.exec();
    }

    if (reply->isRunning())
    {
        timeout=true;
    }

    reply->disconnect(&eventLoop);

    if (reply->error() != QNetworkReply::NoError)
    {
        error = reply->error();
        errorString=reply->errorString();
        return false;
    }
    else
    {      
        // Make sure the HTTP status is 200       
        http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (http_status != 200)
        {                     
            errorString="Incorrect response " + QString::number(http_status);

            if (timeout)
            {
                errorString="Response timeout";
            }

            return false;
        }
    }

    return true;
}


