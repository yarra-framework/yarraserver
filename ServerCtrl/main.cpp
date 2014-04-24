#include <QCoreApplication>
#include "ys_servercontrol.h"
#include "../Server/ys_global.h"


int main(int argc, char *argv[])
{
    YS_OUT("");
    QCoreApplication app(argc, argv);
    ysServerControl instance;

    // Connect the application signals
    QObject::connect(&instance, SIGNAL(finished()),
             &app, SLOT(quit()));

    QTimer::singleShot(10, &instance, SLOT(run()));
    int result=app.exec();

    YS_OUT("");
    return result;
}
