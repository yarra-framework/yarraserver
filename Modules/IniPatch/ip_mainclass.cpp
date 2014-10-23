#include "ip_mainclass.h"

#include <iostream>
#include <QtCore>

using namespace std;

#define DT_VER        QString("0.11")
#define OUT(x)        cout << QString(x).toStdString() << endl;
#define IP_MODE_ID    QString("IniPatch")
#define IP_MODE_ID2   QString("IniPatch_static")

//TODO: Implement mechanism for multiple cmdline arguments via section IniPatch_dynamic


ipMainClass::ipMainClass(QObject *parent) :
    QObject(parent)
{
    returnValue=0;
    sourceIniFilename="";
    targetIniFilename="";
    replacementValue="";
    keysToPatch.clear();
}


void ipMainClass::run()
{
    args=QCoreApplication::arguments();

    if (args.count()!=5)
    {
        OUT("");
        printUsage();
        returnValue=1;
        OUT("");
    }
    else
    {
        processTransfer();
    }

    emit finished();
}


void ipMainClass::printUsage()
{
    OUT("IniPatch for YarraServer - " + DT_VER);
    OUT("-------------------------------\n");
    OUT("Usage:   IniPatch [mode file] [source ini file] [target (in work dir)] [param value]\n");
    OUT("Purpose: Creates a copy of the source ini file in the work directory with the given name. All keys");
    OUT("         specified in the mode file will be replaced by the param value passsed as 4th argument,");
    OUT("         or by static values read from the mode file.");
}


void ipMainClass::processTransfer()
{
    if (readConfig())
    {
        if (!QFile::exists(sourceIniFilename))
        {
            OUT("ERROR: Cannot find source ini file " + sourceIniFilename);
            returnValue=1;
            return;
        }

        if (!QFile::copy(sourceIniFilename, targetIniFilename))
        {
            OUT("ERROR: Cannot copy ini file to " + targetIniFilename);
            returnValue=1;
            return;
        }

        {
            QSettings iniFile(targetIniFilename, QSettings::IniFormat);

            // First, change the keys that should be patched with the values specified in the .mode file
            for (int i=0; i<keys.count(); i++)
            {
                iniFile.setValue(keys.at(i), values.at(i));
            }

            // Second, change the keys that should be patched dynamically with the call argument
            for (int i=0; i<keysToPatch.count(); i++)
            {
                iniFile.setValue(keysToPatch.at(i), replacementValue);
            }
        }

        OUT("Done patching ini file " + targetIniFilename);
    }
    else
    {
        returnValue=1;
    }
}


bool ipMainClass::readConfig()
{
    QString modeFilePath=args.at(1);

    sourceIniFilename=args.at(2);
    targetIniFilename=args.at(3);
    replacementValue=args.at(4);

    {
        QSettings settings(modeFilePath, QSettings::IniFormat);

        // First read the keys that should be patched dynamically with the call argument
        int i=0;
        while ((i<50) && (settings.value(IP_MODE_ID+"/PatchKey"+QString::number(i+1), "").toString()!=""))
        {
            i++;
            keysToPatch.append(settings.value(IP_MODE_ID+"/PatchKey"+QString::number(i), "").toString());
        }

        // Now read the keys that should be patched with the values specified in the .mode file
        settings.beginGroup(IP_MODE_ID2);
        keys = settings.allKeys();

        for (int i=0; i<keys.count(); i++)
        {
            values.append(settings.value(keys.at(i), "").toString());
        }
    }

    if ((keys.count()) && (keysToPatch.count()==0))
    {
        OUT("ERROR: No keys to patch have been found in mode file.");
        return false;
    }

    return true;
}
