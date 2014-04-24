#include "ys_server.h"
#include "ys_global.h"



ysServer::ysServer(QObject *parent) :
    QObject(parent)
{
    controlInterface.setParent(this);
    shutdownRequested=false;
    haltRequested=false;
}


void ysServer::run()
{
    if (prepare())
    {
        runLoop();
    }

    quit();
}


void ysServer::quit()
{
    emit finished();
}


void ysServer::aboutToQuitApp()
{
}



bool ysServer::prepare()
{
    YS_OUT("Yarra Server - Version " + QString(YS_VERSION));
    YS_OUT("================================");
    YS_OUT("");
    YS_OUT("Initializing server...\n");

    return true;
}


bool ysServer::runLoop()
{
    shutdownRequested=false;


    // Check if controlInterface setup was successful
    if (!controlInterface.prepare())
    {
        YS_OUT("Initialization of the server not successful.");
        YS_OUT("Is the server already running?");
        YS_OUT("");
        YS_OUT("If the server is not running, this behavior might result from a previous crash.");
        YS_OUT("Start the server with parameter --force to enforce a restart.");

        return false;
    }


    YS_OUT("Server running (threadID " + QString::number((long)this->thread()->currentThreadId()) + ")");


    while (!shutdownRequested)
    {


        // Sleep for 10ms to prevent excessive CPU usage during idle times
        safeWait(10);
    }

    controlInterface.finish();

    YS_OUT("Server stopped.");

    return true;
}



void ysServer::forceHalt()
{
    haltRequested=true;
    shutdownRequested=true;
}

