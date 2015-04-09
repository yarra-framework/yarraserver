#include "pt_mainclass.h"

#include <iostream>
#include <QtCore>
#include <QTest>

using namespace std;

#define PT_VER        QString("0.4d")
#define PT_MODE_ID    QString("PACSTransfer")

#define OUT(x)        cout << QString(x).toStdString() << endl;

#define EXEC_TIMEOUT    21600000
#define DCMS_PER_CALL   100
#define NUMBER_RETRIES  6
#define RETRY_PAUSETIME 10000


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
        execResult=false;
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
        }
        else
        {
        }
    }
    else
    {
        // Normal timeout-handling if QEventLoop works normally
        if (timeoutTimer.isActive())
        {
            timeoutTimer.stop();
        }
        else
        {
            OUT("WARNING: Process event loop timed out.");
            if (process.state()==QProcess::Running)
            {
                OUT("WARNING: Process is still active. Killing process.");
                process.kill();
                execResult=false;
            }
        }
    }     

    readOutput();

    if (process.exitStatus()==QProcess::CrashExit)
    {
        OUT("ERROR: The storescu process crashed.");
        execResult=false;
    }
    if (process.exitStatus()==QProcess::NormalExit)
    {
        if (process.exitCode()!=0)
        {
            OUT("ERROR: storescu returned an error.");
            storescuError=true;
            execResult=false;
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
            OUT("Performing transfer to "+cfg_name.at(i));

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
                    callCmd="storescu -to 60 -ta 60 -aet "+cfg_AET.at(i)+" -aec "+cfg_AEC.at(i)+" "+cfg_IP.at(i)+" "+cfg_port.at(i)+" ";
                }
                callCmd.append(" " + allFiles.at(j));

                if ((j % DCMS_PER_CALL==DCMS_PER_CALL-1) || (j==allFiles.count()-1))
                {
                    // Initial value to get into the loop
                    bool transferSuccess=false;
                    int retryCount=0;

                    // Try sending the images several times
                    while ((retryCount<numberRetries) && (!transferSuccess))
                    {
                        // Execute the transfer
                        transferSuccess=runCommand(callCmd);

                        retryCount++;

                        // If the transfer failed, wait before retrying (because the PACS might be
                        // overloaded temporarily). Start with 10s and increase the wait time
                        // with each repetition (10s, 20s, 30s, 40s, 50s).
                        if ((!transferSuccess) && (retryCount<numberRetries))
                        {
                            OUT("WARNING: Retrying PACS transfer at image "+QString::number(j)+" / "+QString::number(allFiles.count())+".");

                            // Wait for some time (hoping that the PACSs recovers).
                            // Increase the wait time with each repetition.
                            int waitTime=retryCount*RETRY_PAUSETIME;
                            QTest::qWait(waitTime);
                        }
                    }

                    // If there was a problem even after retrying, return a failure
                    // value to Yarra that the transfer was not successfull.
                    // If connecting to the PACS failed, skip the attempt for the other files. Otherwise,
                    // the many timeouts will add up to a long delay. Continue with next PACS in the list.
                    if (!transferSuccess)
                    {
                        storescuError=true;
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
            cfg_name.append( settings.value(PT_MODE_ID+"/Name", "PACS "+QString::number(cfg_count)).toString() );
        }

        // Now, read settings with a number index.
        while ((cfg_count<20) && (settings.value(PT_MODE_ID+"/AEC_" +QString::number(cfg_count+1), "").toString()!=""))
        {
            cfg_count++;
            cfg_AEC.append ( settings.value(PT_MODE_ID+"/AEC_" +QString::number(cfg_count), "YARRA")   .toString() );
            cfg_AET.append ( settings.value(PT_MODE_ID+"/AET_" +QString::number(cfg_count), "STORESCU").toString() );
            cfg_IP.append  ( settings.value(PT_MODE_ID+"/IP_"  +QString::number(cfg_count), "")        .toString() );
            cfg_port.append( settings.value(PT_MODE_ID+"/Port_"+QString::number(cfg_count), "")        .toString() );
            cfg_name.append( settings.value(PT_MODE_ID+"/Name_"+QString::number(cfg_count), "PACS "+QString::number(cfg_count)).toString() );
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





