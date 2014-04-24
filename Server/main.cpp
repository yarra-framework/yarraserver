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
        signal(SIGTERM,  &ysHaltDetection::serverHalt);
        //signal(SIGBREAK, &CleanExit::exitQt);
    }

    static void serverHalt(int sig)
    {
        YS_OUT(" ## HALT requested");
        instancePtr->forceHalt();
    }
};



int main(int argc, char *argv[])
{
    YS_OUT("");

    QCoreApplication app(argc, argv);
    ysServer instance;
    instancePtr=&instance;
    ysHaltDetection haltDetection;

    // Connect the application signals
    QObject::connect(&instance, SIGNAL(finished()),
             &app, SLOT(quit()));
    QObject::connect(&app, SIGNAL(aboutToQuit()),
             &instance, SLOT(aboutToQuitApp()));

    // This code will start the messaging engine in QT and in
    // 10ms it will start the execution in the MainClass.run routine;
    QTimer::singleShot(10, &instance, SLOT(run()));

    int result=app.exec();

    YS_OUT("");
    return result;
}
