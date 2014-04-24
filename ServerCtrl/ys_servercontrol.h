#ifndef YS_SERVERCONTROL_H
#define YS_SERVERCONTROL_H

#include <QObject>

class ysServerControl : public QObject
{
    Q_OBJECT
public:
    explicit ysServerControl(QObject *parent = 0);
    void quit();

signals:
    void finished();

public slots:
    void run();


};

#endif // YS_SERVERCONTROL_H
