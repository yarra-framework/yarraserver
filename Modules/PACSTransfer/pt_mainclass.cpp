#include "pt_mainclass.h"

#include <iostream>
#include <QtCore>

using namespace std;

#define PT_VER        QString("0.4a")
#define PT_MODE_ID    QString("PACSTransfer")

#define OUT(x)        cout << QString(x).toStdString() << endl;

#define EXEC_TIMEOUT  21600000
#define DCMS_PER_CALL 100
#define NUMBER_RETRIES 3


ptMainClass::ptMainClass(QObject *parent) :
    QObject(parent)
{
    returnValue=0;
    process.setProcessChannelMode(QProcess::MergedChannels);
    storescuError=false;
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


bool ptMainClass::runCommand(QString cmd)
{
    bool execResult=true;
    bool eventloopProblem=false;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(EXEC_TIMEOUT);

    QEventLoop q;
    connect(&process, SIGNAL(finished(int , QProcess::ExitStatus)), &q, SLOT(quit()));
    connect(&process, SIGNAL(error(QProcess::ProcessError)), &q, SLOT(quit()));
    connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
    connect(&timeoutTimer, SIGNAL(timeout()), &q, SLOT(quit()));

    // Time measurement to diagnose calling problems
    QTime ti;
    ti.start();
    timeoutTimer.start();

    // Start the process. Note: The commandline and arguments need to be defined before.
    process.start(cmd);

    if (process.state()==QProcess::NotRunning)
    {
        OUT("ERROR: Process returned immediately.");
        execValue=1;
    }
    else
    {
        q.exec();
    }

    // Check for problems with the event loop: Sometimes it seems to return to quickly!
    // In this case, start a second while loop to check when the process is really finished.
    if ((timeoutTimer.isActive()) && (process.state()==QProcess::Running))
    {
        timeoutTimer.stop();
        eventloopProblem=true;
        while ((process.state()==QProcess::Running) && (ti.elapsed()<EXEC_TIMEOUT))
        {
            QCoreApplication::processEvents();
        }

        // If the process did not finish within the timeout duration
        if (process.state()==QProcess::Running)
        {
            OUT("WARNING: Process is still active. Killing process.");
            process.kill();
            execResult=false;
            execValue=1;
        }
        else
        {
            execResult=true;
        }
    }
    else
    {
        // Normal timeout-handling if QEventLoop works normally
        if (timeoutTimer.isActive())
        {
            execResult=true;
            timeoutTimer.stop();
        }
        else
        {
            OUT("WARNING: Process event loop timed out.");
            execResult=false;
            if (process.state()==QProcess::Running)
            {
                OUT("WARNING: Process is still active. Killing process.");
                process.kill();
                execValue=1;
            }
        }
    }     

    readOutput();

    if (process.exitStatus()==QProcess::CrashExit)
    {
        OUT("ERROR: The storescu process crashed.");
        execResult=false;
        execValue=1;
    }
    if (process.exitStatus()==QProcess::NormalExit)
    {
        if (process.exitCode()!=0)
        {
            OUT("ERROR: storescu returned an error.");
            storescuError=true;
            execValue=1;
        }
    }

    // Notify the user if the mysterious eventloop problem occured
    if (eventloopProblem)
    {
        OUT("WARNING: QEventLoop returned too early. Starting secondary loop.");
    }

    return execResult;
}


void ptMainClass::readOutput()
{
    while (process.canReadLine())
    {
        QString line(process.readLine());
        cout << line.toStdString();
    }
}


void ptMainClass::printUsage()
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
        QString inputPath=args.at(2);
        QDir inputDir(inputPath);
        QStringList allFiles=inputDir.entryList(QDir::Files, QDir::Name);
        OUT(QString::number(allFiles.count()) + " files found for DICOM transfer.");
        OUT("");
        process.setWorkingDirectory(inputPath);

        for (int i=0; i<cfg_count; i++)
        {
            OUT("Performing transfer to PACS "+QString::number(i+1));

            /*
            // Directory version
            QString callCmd="storescu --timeout 20 +sd -aet "+cfg_AET.at(i)+" -aec "+cfg_AEC.at(i)+" "+cfg_IP.at(i)+" "+cfg_port.at(i)+" ";
            callCmd+=args.at(2)+"/";
            OUT("Command: "+callCmd);
            runCommand(callCmd);
            */

            int numberRetries=NUMBER_RETRIES;

            // File-by-file version to ensure right sending order
            QString callCmd="";
            // NOTE: File counter is j here, not i (i=PACS counter)
            for (int j=0; j<allFiles.count(); j++)
            {
                // Combine always 100 DCMs into on call to increase speed
                if (j % DCMS_PER_CALL==0)
                {
                    callCmd="storescu --timeout 20 -aet "+cfg_AET.at(i)+" -aec "+cfg_AEC.at(i)+" "+cfg_IP.at(i)+" "+cfg_port.at(i)+" ";
                }
                callCmd.append(" " + allFiles.at(j));

                if ((j % DCMS_PER_CALL==DCMS_PER_CALL-1) || (j==allFiles.count()-1))
                {
                    // Initial value to get into the loop
                    execValue=-1;
                    int retryCount=0;

                    // Try sending the images several times
                    while ((retryCount<numberRetries) && (execValue!=0))
                    {
                        // Execute the transfer
                        runCommand(callCmd);

                        if (execValue!=0)
                        {
                            OUT("WARNING: Retrying PACS transfer.");
                        }

                        // TODO: Possibly wait for a little time until the PACS recovered.

                        retryCount++;
                    }

                    // If there was a problem even after retrying, return a failure
                    // value to Yarra that the transfer was not successfull.
                    if (execValue!=0)
                    {
                        returnValue=execValue;
                    }

                    // If connecting to the PACS failed, skip the attempt for the other files. Otherwise,
                    // the many timeouts will add up to a long delay. Continue with next PACS in the list.
                    if (returnValue==1)
                    {
                        break;
                    }
                }
            }
        }

        if (storescuError)
        {
            OUT("ERROR: PACS transfer was not successful.");
            returnValue=1;
        }
        else
        {
            OUT("Finished sending DICOMs.");
        }
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





