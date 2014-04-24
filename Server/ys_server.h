#ifndef YS_SERVER_H
#define YS_SERVER_H

#include "ys_controlinterface.h"
#include "ys_global.h"
#include "ys_staticconfig.h"
#include "ys_dynamicconfig.h"


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

    void setShutdownRequest();
    void forceHalt();

    void safeWait(int ms);

    ysControlInterface controlInterface;

protected:
    bool shutdownRequested;
    bool haltRequested;
};


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
