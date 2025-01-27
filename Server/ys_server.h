#ifndef YS_SERVER_H
#define YS_SERVER_H

#include "ys_global.h"
#include "ys_controlinterface.h"
#include "ys_staticconfig.h"
#include "ys_dynamicconfig.h"
#include "ys_log.h"
#include "ys_job.h"
#include "ys_process.h"
#include "ys_queue.h"
#include "ys_statistics.h"
#include "ys_notificationmail.h"
#include <../Common/NetLogger/netlogger.h>
#include <QtCore>

class ysServer: public QObject
{
    Q_OBJECT

public:
    explicit ysServer(QObject *parent = 0);
    void quit();

signals:
    void finished();

public slots:
    void run();
    void aboutToQuitApp();

public:
    bool prepare();
    bool runLoop();
    bool processJob();

    void setShutdownRequest();
    void forceHalt();

    void safeWait(int ms);
    void setErrorReason(QString str);

    // Instances of the individual server modules
    ysLog              log;
    ysStaticConfig     staticConfig;
    ysDynamicConfig    dynamicConfig;
    ysStatistics       statistics;
    ysControlInterface controlInterface;
    ysQueue            queue;
    ysProcess          processor;
    ysJob*             currentJob;
    ysNotificationMail notification;

    QString            status;

    QString errorReason;
    NetLogger netLogger;

    bool shutdownRequested;
    bool haltRequested;
    int  returnCode;
};

inline void ysServer::setErrorReason(QString str) {
    errorReason = str;
}

inline void ysServer::setShutdownRequest()
{
    shutdownRequested=true;
}


inline void ysServer::safeWait(int ms)
{
    QTime ti;
    ti.start();

    while (ti.elapsed()<ms)
    {
        struct timespec ts = { YS_SLEEP_INTERVAL / 1000, (YS_SLEEP_INTERVAL % 1000) * 1000 * 1000 };
        nanosleep(&ts, NULL);
        QCoreApplication::processEvents();
    }
}


#endif // YS_SERVER_H
