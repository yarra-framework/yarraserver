#include "ys_servercontrol.h"
#include "../Server/ys_global.h"
#include "../Server/ys_controlapi.h"


ysServerControl::ysServerControl(QObject *parent) :
    QObject(parent)
{
    mode=MODE_NONE;

    connect(&socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(onSocketError(QLocalSocket::LocalSocketError)));

    connect(&socket, SIGNAL(connected()),
            this, SLOT(onConnected()));

    connect(&socket, SIGNAL(disconnected()),
            this, SLOT(onDisconnected()));


}


void ysServerControl::run()
{
    // TODO

    YS_OUT("Yarra Server Control");
    YS_OUT("====================");
    YS_OUT("");

    if ( (QCoreApplication::arguments().count()<2)
         || (QCoreApplication::arguments().contains("-u",Qt::CaseInsensitive)) )
    {
        printUsage();
    }
    else
    {
        if (QCoreApplication::arguments().contains("-t",Qt::CaseInsensitive))
        {
            mode=MODE_TEST;
        }
        if (QCoreApplication::arguments().contains("-s",Qt::CaseInsensitive))
        {
            mode=MODE_SHUTDOWN;
        }
        if (QCoreApplication::arguments().contains("-h",Qt::CaseInsensitive))
        {
            mode=MODE_HALT;
        }

        if (mode==MODE_NONE)
        {
            YS_OUT("Invalid parameter.");
            YS_OUT("");
            printUsage();
        }
        else
        {
            // Create socket connection
            socket.connectToServer(YS_CI_ID);
        }
    }

    quit();
}


void ysServerControl::quit()
{
    emit finished();
}


void ysServerControl::printUsage()
{
    YS_OUT("Usage:");
    YS_OUT("");
    YS_OUT("  -s   Shutdown server after current reconstruction job.");
    YS_OUT("  -h   Stop server immediately. Current job will be killed.");
    YS_OUT("  -t   Test if server is active and responding.");
    YS_OUT("  -u   Print this usage information.");

}


void ysServerControl::onConnected()
{
   // TODO: Send request to server
}


void ysServerControl::onDisconnected()
{
   // TODO
}


void ysServerControl::onSocketError(QLocalSocket::LocalSocketError socketError)
{
   // TODO
}


