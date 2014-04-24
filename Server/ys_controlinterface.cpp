#include "ys_controlinterface.h"
#include "ys_global.h"
#include "ys_server.h"

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

    ipcServer.setSocketOptions(QLocalServer::UserAccessOption);

    if (!ipcServer.listen(YS_CI_ID))
    {
        // Launching the control server did not work.
        // TODO: Create error message
        ipcError=true;
    }


    if (!ipcError)
    {
        myThread.start();

        moveToThread(&myThread);
        ipcServer.moveToThread(&myThread);
        testTimer.moveToThread(&myThread);
    }

    //testTimer.singleShot(1000, this, SLOT(requestServerShutdown()));

    return !ipcError;
}


void ysControlInterface::finish()
{
    myThread.quit();
    myThread.wait(10000);
}



void ysControlInterface::requestServerShutdown()
{
    YS_OUT("ControlInterface (threadID " + QString::number((long)QThread::currentThreadId()) + ")");

    parent->setShutdownRequest();
}


