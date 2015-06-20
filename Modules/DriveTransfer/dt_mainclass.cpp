#include "dt_mainclass.h"

#include <iostream>
#include <QtCore>

#include "../../Common/yc_utils.h"

using namespace std;

#define DT_VER        QString("0.11")
#define OUT(x)        cout << QString(x).toStdString() << endl;
#define DT_MODE_ID    QString("DriveTransfer")


dtMainClass::dtMainClass(QObject *parent) :
    QObject(parent)
{
    returnValue=0;
    sourcePath="";
    targetPath="";
    taskName="Unkown";
}


void dtMainClass::run()
{
    args=QCoreApplication::arguments();

    OUT("");
    if (args.count()!=4)
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


void dtMainClass::printUsage()
{
    OUT("DriveTransfer for YarraServer - " + DT_VER);
    OUT("------------------------------------\n");
    OUT("Usage:   DriveTransfer [mode file] [directory with DICOMs] [unique task name]\n");
    OUT("Purpose: Copies reconstructed DICOM images into a local or mounted directory.");
}


void dtMainClass::processTransfer()
{
    if (readConfig())
    {
        QDir inputDir(sourcePath);
        QStringList allFiles=inputDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        OUT(QString::number(allFiles.count()) + " files found for transfer.");

        for (int i=0; i<allFiles.count(); i++)
        {
            QString filename=allFiles.at(i);
            if (!ycUtils::copyRecursively(sourcePath+"/"+filename,targetDir.filePath(filename)))
            {
                OUT("ERROR: Cannot copy file " +QString(sourcePath+"/"+filename)+ " to " + targetDir.filePath(filename));
                returnValue=1;
                break;
            }
        }
        OUT("Finished transferring files.");
    }
    else
    {
        returnValue=1;
    }
}


bool dtMainClass::readConfig()
{
    QString modeFilePath=args.at(1);
    taskName=args.at(3);
    sourcePath=args.at(2);

    {
        QSettings settings(modeFilePath, QSettings::IniFormat);
        targetPath=settings.value(DT_MODE_ID+"/TargetPath", "").toString();
    }

    if (targetPath=="")
    {
        OUT("ERROR: No DriverTransfer configuration has been found.");
        return false;
    }

    if (!targetDir.cd(targetPath))
    {
        // Try to create target path if it does not exist
        if (!targetDir.mkdir(targetPath))
        {
            OUT("ERROR: Cannot access nor create target path " + targetPath);
            return false;
        }
    }

    // Check if task directory already exsists in the storage path
    if (targetDir.exists(taskName))
    {
        OUT("ERROR: Folder already exsists in target path " + targetDir.filePath(taskName));
        return false;
    }

    if (!targetDir.mkdir(taskName))
    {
        OUT("ERROR: Folder already exsists in target path " + targetDir.filePath(taskName));
        return false;
    }

    if (!targetDir.cd(taskName))
    {
        OUT("ERROR: Cannot enter created target path " + targetDir.filePath(taskName));
        return false;
    }

    OUT("Moving data to path " + targetPath);
    return true;
}


