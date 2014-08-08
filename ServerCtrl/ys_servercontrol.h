#ifndef YS_SERVERCONTROL_H
#define YS_SERVERCONTROL_H

#include <QtCore>
#include <QtNetwork>

#include "../Server/ys_global.h"


class ysServerControl : public QObject
{
    Q_OBJECT
public:

    enum {
        MODE_NONE=0,
        MODE_TEST,
        MODE_SHUTDOWN,
        MODE_HALT,
        MODE_STATUS,
        MODE_LAUNCH,
        MODE_SHOWLOG
    };

    explicit ysServerControl(QObject *parent = 0);
    void quit();

    void output(QString normalOutput, QString parserOutput);
    void launchServer();

signals:
    void finished();

public slots:
    void run();

    void onConnected();
    void onDisconnected();
    void onSocketError(QLocalSocket::LocalSocketError socketError);
    void readResponse();

    void displayLog(QString logFile);
    void readLogOutput();

protected:
    void printUsage();

    QLocalSocket socket;
    int mode;
    bool parserFormat;
    bool forceStart;

    quint16 responseSize;

    QProcess process;
};


inline void ysServerControl::output(QString normalOutput, QString parserOutput)
{
    if (parserFormat)
    {
        if (parserOutput.length()>0)
        {
            YS_OUT(parserOutput);
        }
    }
    else
    {
        if (normalOutput.length()>0)
        {
            YS_OUT(normalOutput);
        }
    }
}


#endif // YS_SERVERCONTROL_H
