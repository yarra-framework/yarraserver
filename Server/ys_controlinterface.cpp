#include "ys_controlinterface.h"
#include "ys_global.h"
#include "ys_server.h"
#include "ys_controlapi.h"


ysControlInterface::ysControlInterface()
{
    parent=0;

    if (QCoreApplication::arguments().contains(YS_CI_FORCECMD))
    {
        forceRestart=true;
    }
    else
    {
        forceRestart=false;
    }
}


ysControlInterface::~ysControlInterface()
{
    // Close the ipc server and free it
    if (ipcServer.isListening())
    {
        ipcServer.close();
    }
}


bool ysControlInterface::prepare()
{
    bool ipcError=false;

    if (!parent)
    {
        // Parent pointer not set before launching thread.
        YS_OUT("Fatal ControlInterface error.");
        return false;
    }

    if (forceRestart)
    {
        if (!ipcServer.removeServer(YS_CI_ID))
        {
            YS_OUT("Unable to force server restart.");
            ipcError=true;
        }
    }

    //ipcServer.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&ipcServer, SIGNAL(newConnection()), this, SLOT(receiveRequest()));

    if (!ipcServer.listen(YS_CI_ID))
    {
        // Launching the control server did not work.
        YS_OUT("ERROR: Launching the IPC interface did not succeed.");
        YS_OUT("Cannot launch server.");
        ipcError=true;
    }

    if (!ipcError)
    {
        myThread.start();

        moveToThread(&myThread);
        ipcServer.moveToThread(&myThread);
    }

    return !ipcError;
}


void ysControlInterface::finish()
{
    myThread.quit();
    myThread.wait(10000);
}


void ysControlInterface::receiveRequest()
{
    QLocalSocket* clientConnection = ipcServer.nextPendingConnection();
    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    if (clientConnection->waitForReadyRead(1000))
    {
        char cmd=YS_CTRL_INVALID;
        clientConnection->read(&cmd,1);

        switch (cmd)
        {
        case YS_CTRL_TEST:
            processTestRequest(clientConnection);
            break;

        case YS_CTRL_SHUTDOWN:
            processShutdownRequest(clientConnection);
            break;

        case YS_CTRL_HALT:
            processHaltRequest(clientConnection);
            break;

        case YS_CTRL_STATUS:
            processLogRequest(clientConnection);
            break;

        case YS_CTRL_INVALID:
        default:
            YS_OUT("WARNING: Invalid IPC request.")
            break;
        }
    }
    else
    {
        // Received IPC connection without request
        YS_OUT("WARNING: Incomplete IPC request.")
    }

    clientConnection->disconnectFromServer();
}


void ysControlInterface::processTestRequest(QLocalSocket* socket)
{
    writeToSocket(YS_CTRL_ACK, socket);
}


void ysControlInterface::processShutdownRequest(QLocalSocket* socket)
{
    //YS_OUT("ControlInterface (threadID " + QString::number((long)QThread::currentThreadId()) + ")");
    writeToSocket(YS_CTRL_ACK, socket);
    parent->setShutdownRequest();
}


void ysControlInterface::processHaltRequest(QLocalSocket* socket)
{
    writeToSocket(YS_CTRL_ACK, socket);
    parent->forceHalt();
}


void ysControlInterface::processLogRequest(QLocalSocket* socket)
{
    QString result=parent->status;

    if (parent->shutdownRequested)
    {
        result += YS_CTRL_STOPREQUEST;
    }

    writeToSocket(result, socket);
}


void ysControlInterface::writeToSocket(QString string, QLocalSocket* socket)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << QString(string);
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    socket->write(block);
    socket->flush();
}



