#include "ys_servercontrol.h"
#include "../Server/ys_controlapi.h"
#include "ys_parserformat.h"


ysServerControl::ysServerControl(QObject *parent) :
    QObject(parent)
{
    mode=MODE_NONE;
    parserFormat=false;
    responseSize=0;

    connect(&socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(onSocketError(QLocalSocket::LocalSocketError)));

    connect(&socket, SIGNAL(connected()),
            this, SLOT(onConnected()));

    connect(&socket, SIGNAL(disconnected()),
            this, SLOT(onDisconnected()));

    connect(&socket, SIGNAL(readyRead()),
            this, SLOT(readResponse()));
}


void ysServerControl::run()
{
    // TODO
    if ( (QCoreApplication::arguments().count() < 2)
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
        if (QCoreApplication::arguments().contains("-l",Qt::CaseInsensitive))
        {
            mode=MODE_LAUNCH;
        }
        if (QCoreApplication::arguments().contains("--p",Qt::CaseInsensitive))
        {
            parserFormat=true;
        }

        if (!parserFormat)
        {
            YS_OUT("");
        }

        if (mode==MODE_NONE)
        {
            YS_OUT("Invalid parameter.");
            printUsage();
        }
        else
        {
            if (mode!=MODE_LAUNCH)
            {
                // Create socket connection
                socket.connectToServer(YS_CI_ID);
            }
            else
            {
                launchServer();
            }
        }
    }
}


void ysServerControl::quit()
{
    if (!parserFormat)
    {
        YS_OUT("");
    }

    emit finished();
}


void ysServerControl::launchServer()
{
    QString serverBinary=QCoreApplication::applicationDirPath() + "/YarraServer";

    if (QProcess::startDetached(serverBinary))
    {
        output("Server started.", YS_CTRL_PF_TRUE);
    }
    else
    {
        output("Error starting server. Check installation.", YS_CTRL_PF_ERROR);
    }

    quit();
}



void ysServerControl::printUsage()
{
    YS_OUT("");
    YS_OUT("Yarra Server Control");
    YS_OUT("====================");
    YS_OUT("");
    YS_OUT("Usage:");
    YS_OUT("");
    YS_OUT("  -l   Launch the server (detached process).");
    YS_OUT("  -s   Shutdown server after current reconstruction job.");
    YS_OUT("  -h   Stop server immediately. Current job will be killed.");
    YS_OUT("  -t   Test if server is active and responding.");
    YS_OUT("  -u   Print this usage information.");
    YS_OUT("");
    YS_OUT("Options:");
    YS_OUT("");
    YS_OUT("  --p  Output format for easy parsing.");

    quit();
}


void ysServerControl::onConnected()
{
    char cmd=YS_CTRL_INVALID;

    switch (mode)
    {
    case MODE_TEST:
        cmd=YS_CTRL_TEST;
        break;
    case MODE_SHUTDOWN:
        cmd=YS_CTRL_SHUTDOWN;
        break;
    case MODE_HALT:
        cmd=YS_CTRL_HALT;
        break;
    default:
        YS_OUT("ERROR: Invalid mode selcted.");
        break;
    }

    if (cmd!=YS_CTRL_INVALID)
    {
        responseSize=0;

        // Send request to server
        socket.write(&cmd);
    }
}


void ysServerControl::onDisconnected()
{
   // TODO
}


void ysServerControl::onSocketError(QLocalSocket::LocalSocketError socketError)
{
    switch (socketError)
    {
    case QLocalSocket::ServerNotFoundError:
        output("Server not found. Is the Yarra server running?", YS_CTRL_PF_FALSE);
        break;

    case QLocalSocket::ConnectionRefusedError:
        output("ERROR: The connection was refused. Make sure that the Yarra server is running.", YS_CTRL_PF_ERROR);
        break;

    case QLocalSocket::PeerClosedError:
        //YS_OUT("Peer was closed.");
        break;

    default:
        output("ERROR: The following error occured", YS_CTRL_PF_ERROR);
        YS_OUT(socket.errorString());
        break;
    }

    quit();
}


void ysServerControl::readResponse()
{
    QDataStream in(&socket);
    in.setVersion(QDataStream::Qt_4_0);

    if (responseSize==0)
    {
        if (socket.bytesAvailable() < (int)sizeof(quint16))
        {
            return;
        }
        in >> responseSize;
    }

    if (in.atEnd())
    {
        return;
    }

    QString response;
    in >> response;

    bool incorrectAnswer=false;

    switch (mode)
    {
    case MODE_TEST:
        if (response==YS_CTRL_ACK)
        {
            output("Yarra server is active.", YS_CTRL_PF_TRUE);
        }
        else
        {
            incorrectAnswer=true;
        }
        break;

    case MODE_SHUTDOWN:
        if (response==YS_CTRL_ACK)
        {
            output("Yarra server is shutting down after current job.", YS_CTRL_PF_TRUE);
        }
        else
        {
            incorrectAnswer=true;
        }
        break;

    case MODE_HALT:
        if (response==YS_CTRL_ACK)
        {
            output("Yarra server is shutting down immediately.", YS_CTRL_PF_TRUE);
        }
        else
        {
            incorrectAnswer=true;
        }
        break;

    default:
        break;
    }

    if (incorrectAnswer)
    {
        output("ERROR: Incorrect answer from Yarra server.", YS_CTRL_PF_ERROR);
        YS_OUT("Response: '" + response +"'");
    }

    quit();
}



