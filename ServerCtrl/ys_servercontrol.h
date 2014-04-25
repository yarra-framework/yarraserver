#ifndef YS_SERVERCONTROL_H
#define YS_SERVERCONTROL_H

#include <QtCore>
#include <QtNetwork>


class ysServerControl : public QObject
{
    Q_OBJECT
public:

    enum {
        MODE_NONE=0,
        MODE_TEST,
        MODE_SHUTDOWN,
        MODE_HALT
    };

    explicit ysServerControl(QObject *parent = 0);
    void quit();

signals:
    void finished();

public slots:
    void run();

protected:
    void printUsage();

    QLocalSocket socket;
    int mode;

};

#endif // YS_SERVERCONTROL_H
