#ifndef YS_CONTROLINTERFACE_H
#define YS_CONTROLINTERFACE_H

#include <QtCore>
#include <QtNetwork>

class ysServer;

class ysControlInterface : public QObject
{
    Q_OBJECT

public:
    ysControlInterface();
    ~ysControlInterface();

    void setParent(ysServer* myParent);
    bool prepare();
    void finish();

    QLocalServer ipcServer;
    QTimer testTimer;

    void processTestRequest(QLocalSocket* socket);
    void processShutdownRequest(QLocalSocket* socket);
    void processHaltRequest(QLocalSocket* socket);

    void writeToSocket(QString string, QLocalSocket* socket);

public slots:
    void receiveRequest();

private:
   ysServer* parent;

   bool forceRestart;

   QThread myThread;

};



inline void ysControlInterface::setParent(ysServer* myParent)
{
    parent=myParent;
}




#endif // YS_CONTROLINTERFACE_H
