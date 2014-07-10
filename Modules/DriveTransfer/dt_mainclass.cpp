#include "dt_mainclass.h"

#include <iostream>
#include <QtCore>

using namespace std;

#define DT_VER        QString("0.1a")
#define OUT(x)        cout << QString(x).toStdString() << endl;
#define DT_MODE_ID    QString("DriveTransfer")


ptMainClass::ptMainClass(QObject *parent) :
    QObject(parent)
{
    returnValue=0;
    process.setProcessChannelMode(QProcess::MergedChannels);
}


void ptMainClass::run()
{
    args=QCoreApplication::arguments();

    OUT("");
    if (args.count()!=3)
    {
       printUsage();
       returnValue=1;
    }
    else
    {
        processTransfer();
    }
    OUT("");

    emit finished();
}


void dtMainClass::readOutput()
{
    // TODO: Search output for error messages from storescu
    while (process.canReadLine())
    {
        cout << QString(process.readLine()).toStdString();
    }
}


void dtMainClass::printUsage()
{
    OUT("PACSTransfer for YarraServer - " + PT_VER);
    OUT("-----------------------------------\n");
    OUT("Usage:   PACSTransfer [mode file] [directory with DICOMs]\n");
    OUT("Purpose: Sends reconstructed DICOM images to one or more PACS servers.");
    OUT("         Requires installation of the OFFIS dcmtk package.");
}


void ptMainClass::processTransfer()
{
    if (readConfig())
    {
        QDir inputDir(args.at(2));
        QStringList allFiles=inputDir.entryList(QDir::Files);
        OUT(QString::number(allFiles.count()) + " files found for DICOM transfer.");
        OUT("");

        for (int i=0; i<cfg_count; i++)
        {
            OUT("Starting transfer to PACS "+QString::number(i+1));
            QString callCmd="storescu --timeout 20 +sd -aet "+cfg_AET.at(i)+" -aec "+cfg_AEC.at(i)+" "+cfg_IP.at(i)+" "+cfg_port.at(i)+" ";
            callCmd+=args.at(2)+"/";

            OUT("Command: "+callCmd);
            runCommand(callCmd);
            OUT("");
        }
        OUT("Finished sending DICOMs.")
    }
}


bool ptMainClass::readConfig()
{
    cfg_count=0;
    QString modeFilePath=args.at(1);

    {
        QSettings settings(modeFilePath, QSettings::IniFormat);

        // First, check for settings without number index
        if (settings.value(PT_MODE_ID+"/AEC", "").toString()!="")
        {
            cfg_count=1;
            cfg_AEC.append ( settings.value(PT_MODE_ID+"/AEC" , "YARRA")   .toString() );
            cfg_AET.append ( settings.value(PT_MODE_ID+"/AET" , "STORESCU").toString() );
            cfg_IP.append  ( settings.value(PT_MODE_ID+"/IP"  , "")        .toString() );
            cfg_port.append( settings.value(PT_MODE_ID+"/Port", "")        .toString() );
        }

        // Now, read settings with a number index.
        while ((cfg_count<20) && (settings.value(PT_MODE_ID+"/AEC_" +QString::number(cfg_count+1), "").toString()!=""))
        {
            cfg_count++;
            cfg_AEC.append ( settings.value(PT_MODE_ID+"/AEC_" +QString::number(cfg_count), "YARRA")   .toString() );
            cfg_AET.append ( settings.value(PT_MODE_ID+"/AET_" +QString::number(cfg_count), "STORESCU").toString() );
            cfg_IP.append  ( settings.value(PT_MODE_ID+"/IP_"  +QString::number(cfg_count), "")        .toString() );
            cfg_port.append( settings.value(PT_MODE_ID+"/Port_"+QString::number(cfg_count), "")        .toString() );
        }
    }


    if (cfg_count==0)
    {
        OUT("ERROR: No PACS configuration has been found.");
        returnValue=1;
        return false;
    }
    else
    {
        OUT("Sending data to " + QString::number(cfg_count) + " PACS server(s).");
        return true;
    }
}





