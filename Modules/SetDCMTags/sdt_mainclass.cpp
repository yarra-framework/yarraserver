#include "sdt_mainclass.h"

#include <iostream>
#include <QtCore>

using namespace std;

#define SDT_VER        QString("0.1a")

#define OUT(x)        cout << QString(x).toStdString() << endl;
#define EXEC_TIMEOUT  21600000
#define SDT_MODE_ID   QString("SetDCMTags")

#define DCMS_PER_CALL 100


sdtMainClass::sdtMainClass(QObject *parent) :
    QObject(parent)
{
    returnValue=0;
    process.setProcessChannelMode(QProcess::MergedChannels);
}


void sdtMainClass::run()
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
        processPostProc();
    }
    OUT("");

    emit finished();
}


bool sdtMainClass::runCommand(QString cmd)
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
        returnValue=1;
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
            returnValue=1;
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
                returnValue=1;
            }
        }
    }     

    readOutput();

    if (process.exitStatus()==QProcess::CrashExit)
    {
        OUT("ERROR: The dcmodify process crashed.");
        execResult=false;
        returnValue=1;
    }
    if (process.exitStatus()==QProcess::NormalExit)
    {
        if (process.exitCode()!=0)
        {
            OUT("ERROR: dcmodify returned an error.");
            returnValue=1;
        }
    }

    // Notify the user if the mysterious eventloop problem occured
    if (eventloopProblem)
    {
        OUT("WARNING: QEventLoop returned too early. Starting secondary loop.");
    }

    return execResult;
}


void sdtMainClass::readOutput()
{
    while (process.canReadLine())
    {
        QString line(process.readLine());
        cout << line.toStdString();
    }
}


void sdtMainClass::printUsage()
{
    OUT("SetDCMTags for YarraServer - " + SDT_VER);
    OUT("--------------------------------------\n");
    OUT("Usage:   SetDCMTags [source directory] [target directory] [ACC]\n");
    OUT("Purpose: Sets the required DICOM tags for sending images into the PACS.");
    OUT("         Requires installation of the OFFIS dcmtk package.");
}


void sdtMainClass::processPostProc()
{
    QString inputPath =args.at(1);
    QString outputPath=args.at(2);

    QDir inputDir;
    if (!inputDir.cd(inputPath))
    {
        OUT("ERROR: Cannot enter input directory.");
        returnValue=1;
        return;
    }

    QDir outputDir;
    if (!outputDir.cd(outputPath))
    {
        OUT("ERROR: Cannot enter input directory.");
        returnValue=1;
        return;
    }

    QStringList allFiles=inputDir.entryList(QDir::Files, QDir::Name);
    OUT(QString::number(allFiles.count()) + " files found for DICOM patching.");

    if (allFiles.count()==0)
    {
        OUT("Nothing to do.");
        return;
    }

    // Move all files to the output directory
    bool moveError=false;
    for (int i=0; i<allFiles.count(); i++)
    {
        QString sourceFile=inputPath+"/"+allFiles.at(i);
        QString targetFile=outputPath+"/"+allFiles.at(i);

        if (!QFile::rename(sourceFile, targetFile))
        {
            moveError=true;
        }
    }

    if (moveError)
    {
        OUT("ERROR: DICOM patching not successful.");
        returnValue=1;
    }
    else
    {
        process.setWorkingDirectory(outputPath);                      
        QString callCmd="";

        for (int i=0; i<allFiles.count(); i++)
        {
            callCmd="dcmodify -nb ";

            // Get the required instructions for the DICOM tags
            callCmd.append(composeDICOMTags(allFiles.at(i)));

            // Append the file name
            callCmd.append(" " + allFiles.at(i));

            // Execute the DICOM modify command
            runCommand(callCmd);
        }

        if (returnValue==0)
        {
            OUT("Finished patching DICOMs.");
        }
    }
}


QString sdtMainClass::composeDICOMTags(QString fname)
{
    QString cmdLine="";

    // Set patient name
    cmdLine += "-i \"(0010,0010)=" + patientName + "\" ";
    cmdLine += "-i \"(0010,0020)=" + patientID + "\" ";

    // TODO: Define image comments via config file etc

    return cmdLine;
}


bool sdtMainClass::readConfiguration()
{
    // TODO

    return true;
}


bool sdtMainClass::readTWIXInformation()
{
    // TODO

    patientName="Mustermann^Hans";
    patientID="100100100";

    return true;
}


