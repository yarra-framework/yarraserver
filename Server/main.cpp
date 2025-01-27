#include <csignal>
#include <QCoreApplication>

#include "ys_server.h"
#include "ys_global.h"


ysServer* instancePtr;

struct ysHaltDetection
{
    ysHaltDetection()
    {
        signal(SIGINT,   &ysHaltDetection::serverHalt);
        signal(SIGTERM,  &ysHaltDetection::serverShutdown);
        signal(SIGURG,   &ysHaltDetection::serverShutdown);
        signal(SIGHUP,   &ysHaltDetection::preventHUP);
        //signal(SIGBREAK, &CleanExit::exitQt);
    }

    static void serverHalt(int)
    {
        YS_OUT(" ## HALT requested");
        instancePtr->forceHalt();
    }

    static void serverShutdown(int)
    {
        YS_OUT(" ## STOP requested");
        instancePtr->setShutdownRequest();
    }

    static void preventHUP(int)
    {
        // Prevents that the server will be shutdown when the terminal is closed.
    }
};


int main(int argc, char *argv[])
{
    YS_OUT("");

    QCoreApplication app(argc, argv);
    ysServer instance;
    instancePtr=&instance;
    ysHaltDetection haltDetection;
    ysRuntimeAccess::setInstance(&instance);

    // Connect the application signals
    QObject::connect(&instance, SIGNAL(finished()),
             &app, SLOT(quit()));
    QObject::connect(&app, SIGNAL(aboutToQuit()),
             &instance, SLOT(aboutToQuitApp()));

    // This code will start the messaging engine in QT and in
    // 10ms it will start the execution in the MainClass.run routine;
    QTimer::singleShot(0, &instance, SLOT(run()));

    app.exec();
    int result=instance.returnCode;

    YS_OUT("");
    return result;
}
