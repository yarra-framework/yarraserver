#include <QCoreApplication>
#include <QtCore>

#include "sub_mainclass.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    subMainClass instance;

    // Connect the application signals
    QObject::connect(&instance, SIGNAL(finished()),
             &app, SLOT(quit()));

    QTimer::singleShot(0, &instance, SLOT(run()));
    app.exec();

    return instance.returnValue;
}

