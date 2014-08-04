#include "ip_mainclass.h"

#include <iostream>
#include <QtCore>

using namespace std;

#define DT_VER        QString("0.1a")
#define OUT(x)        cout << QString(x).toStdString() << endl;


ipMainClass::ipMainClass(QObject *parent) :
    QObject(parent)
{
    returnValue=0;
    iniFilename="";
    accValue="";
}


void ipMainClass::run()
{
    args=QCoreApplication::arguments();

    if (args.count()!=3)
    {
       printUsage();
       returnValue=1;
    }
    else
    {
        processPatch();
    }

    emit finished();
}


void ipMainClass::printUsage()
{
    OUT("");
    OUT("IniPatch_GRASPAcc for YarraServer - " + DT_VER);
    OUT("----------------------------------------\n");
    OUT("Usage:   IniPatch_GRASPAcc [ini file] [ACC number]\n");
    OUT("Purpose: Patches the ACC number in the GRASP ini file for older versions of the GRASP command line tool.");
    OUT("");
}


void ipMainClass::processPatch()
{
    iniFilename=args.at(1);
    accValue=args.at(2);

    if (!QFile::exists(iniFilename))
    {
        OUT("ERROR: Cannot find source ini file " + iniFilename);
        returnValue=1;
        return;
    }

    {
        QSettings iniFile(iniFilename, QSettings::IniFormat);
        iniFile.setValue("PostProcessing/AccessionNumber", accValue);
    }

    OUT("Done adding ACC number to ini file " + iniFilename);
}


