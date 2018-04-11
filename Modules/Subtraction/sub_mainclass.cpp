#include "sub_mainclass.h"

#include <iostream>
#include <QtCore>

#include "dcmtk/dcmdata/dcpath.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/ofstd/ofstd.h"



using namespace std;


#define SUB_VER      QString("0.1a")
#define OUT(x)       cout << QString(x).toStdString() << endl;
#define SUB_MODE_ID  QString("Subtraction")


subMainClass::subMainClass(QObject *parent) :
    QObject(parent)
{
    keepSourceImages=false;
    baselineSeries=true;
    returnValue=0;
}


void subMainClass::run()
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
        processPostProc();
    }
    OUT("");

    emit finished();
}


void subMainClass::printUsage()
{
    OUT("Subtraction for YarraServer - " + SUB_VER);
    OUT("--------------------------------------\n");
    OUT("Usage:   Subtraction [source directory] [target directory] [mode file]\n");
    OUT("Purpose: Calculates subtraction images from a series");
}


void subMainClass::processPostProc()
{
    QString inputPath =args.at(1);
    QString outputPath=args.at(2);
    QString modeFile  =args.at(3);

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

    if (!generateFileList(inputPath))
    {
        OUT("ERROR: Unable to generate file list.");
        returnValue=1;
        return;
    }

    if (!readConfig(modeFile))
    {
        OUT("ERROR: Unable to read configuration.");
        returnValue=1;
        return;
    }

    if (!readBaseline())
    {
        OUT("ERROR: Unable to read baseline images.");
        returnValue=1;
        return;
    }

    if (!createSubtractions(outputPath))
    {
        OUT("ERROR: Unable to create subtraction images.");
        returnValue=1;
        return;
    }

    if (keepSourceImages)
    {
        if (!moveSourceImages(outputPath))
        {
            OUT("ERROR: Unable to move source images.");
            returnValue=1;
            return;
        }
    }

    OUT("Done.");
}


bool subMainClass::readConfig(QString modeFilePath)
{
    QSettings settings(modeFilePath, QSettings::IniFormat);

    baselineSeries  =settings.value(SUB_MODE_ID+"/BaselineSeries", 0).toInt();
    keepSourceImages=settings.value(SUB_MODE_ID+"/KeepSourceImages", false).toBool();

    seriesState.clear();

    /*
        // Now read the keys that should be patched with the values specified in the .mode file
        settings.beginGroup(IP_MODE_ID2);
        keys = settings.allKeys();

        for (int i=0; i<keys.count(); i++)
        {
            values.append(settings.value(keys.at(i), "").toString());
        }
    */

    OUT("Using series " + QString::number(baselineSeries) + " as baseline.");

    return true;
}


bool subMainClass::generateUIDs()
{
    char uid[100];

    dcmGenerateUniqueIdentifier(uid, SITE_STUDY_UID_ROOT);

    studyUID=QString(uid);

    QString uidString="";
    for (int i=0; i<seriesCount; i++)
    {
        uidString=dcmGenerateUniqueIdentifier(uid, SITE_SERIES_UID_ROOT);
        seriesUIDList.append(uidString);
    }

    return true;
}


bool subMainClass::readBaseline()
{
    return true;
}


bool subMainClass::createSubtractions(QString outputPath)
{
    for (int i=0; i<fileList.count(); i++)
    {
        if (fileList.at(i).seriesNumber > baselineSeries)
        {
            // Calculate the new series number (by using the input series count as offset)
            int newSeriesNumber=fileList.at(i).seriesNumber+seriesCount;
            QString outputFilename=outputPath+QString("/sub_%1.%2.dcm").arg(newSeriesNumber,4,10,QLatin1Char('0')).arg(fileList.at(i).sliceNumber,4,10,QLatin1Char('0'));

            OUT("Processing " + fileList.at(i).fileName);
            processDICOM(fileList.at(i).fileName, outputFilename, fileList.at(i).sliceNumber, fileList.at(i).seriesNumber, newSeriesNumber);
        }
    }

    return true;
}


bool subMainClass::processDICOM(QString inFilename, QString outFilename, int slice, int inSeries, int outSeries)
{
    DcmFileFormat fileformat;
    OFCondition status = fileformat.loadFile(inFilename.toUtf8());

    if (!status.good())
    {
        OUT("ERROR: Cannot open DICOM file " + inFilename);
        return false;
    }

    Uint16 rows=0;
    Uint16 cols=0;
    fileformat.getDataset()->findAndGetUint16(DCM_Rows, rows);
    fileformat.getDataset()->findAndGetUint16(DCM_Columns, cols);

    /* get reference to dataset from the fileformat */
    DcmDataset *dataset = fileformat.getDataset();
    const Uint16* pixelData = NULL;

    if (!dataset->findAndGetUint16Array(DCM_PixelData, pixelData).good())
    {
        OUT("ERROR: Cannot read pixel data from " + inFilename);
        return false;
    }

    OUT("Image dim = " + QString::number(cols) + " x " + QString::number(rows));

    Uint16* modData=new Uint16[rows*cols];

    for (int y=0; y< rows; y++)
    {
        for (int x=0; x<cols; x++)
        {
            // TODO

            /*
            if (y % 2)
            {
                modData[y*cols+x]=pixelData[y*cols+x];
            }
            else
            {
                modData[y*cols+x]=0;
            }
            */
        }
    }

    dataset->putAndInsertUint16Array(DCM_PixelData, modData, rows*cols);

    // TODO: Write new DICOM tags

    status = fileformat.saveFile(outFilename.toUtf8(), EXS_LittleEndianExplicit);

    delete[] modData;

    if (!status.good())
    {
        OUT("ERROR: Cannot save DICOM file " + outFilename);
        return false;
    }

    return true;
}


bool subMainClass::moveSourceImages(QString outputPath)
{
    for (int i=0; i<fileList.count(); i++)
    {
        if (!QFile::rename(fileList.at(i).fileName, outputPath+"/"+fileList.at(i).fileNameWithoutPath))
        {
            OUT("ERROR: Cannot move file "+fileList.at(i).fileNameWithoutPath+" to path "+outputPath);
            return false;
        }
    }

    return true;
}


/*
bool subMainClass::readDICOM(QString filename, QString outputFilename)
{
    DcmFileFormat fileformat;
    OFCondition status = fileformat.loadFile(filename.toUtf8());

    if (status.good())
    {
        Uint16 rows=0;
        Uint16 cols=0;
        fileformat.getDataset()->findAndGetUint16(DCM_Rows, rows);
        fileformat.getDataset()->findAndGetUint16(DCM_Columns, cols);

        // get reference to dataset from the fileformat
        DcmDataset *dataset = fileformat.getDataset();
        const Uint16* pixelData = NULL;

        if (dataset->findAndGetUint16Array(DCM_PixelData, pixelData).good())
        {
        }

        OUT("Image dim = " + QString::number(cols) + " x " + QString::number(rows));

        //for (int i = 0; i< 10; i++)
        //{
        //    testDmp+=QString::number(pixelData[i]) + " ";
        //}

        Uint16* modData=new Uint16[rows*cols];

        for (int y=0; y< rows; y++)
        {
            for (int x=0; x<cols; x++)
            {
                if (y % 2)
                {
                    modData[y*cols+x]=pixelData[y*cols+x];
                }
                else
                {
                    modData[y*cols+x]=0;
                }
            }
            //testDmp+=QString::number(pixelData[i]) + " ";
        }

        dataset->putAndInsertUint16Array(DCM_PixelData, modData, rows*cols);
        //OUT ("Pixel: "+testDmp);

        delete[] modData;
    }

    status = fileformat.saveFile(outputFilename.toUtf8(), EXS_LittleEndianExplicit);
    return true;
}
*/



bool subMainClass::generateFileList(QString inputPath)
{
    seriesCount=0;

    QDir inputDir;
    if (!inputDir.cd(inputPath))
    {
        OUT("ERROR: Cannot enter input directory.");
        returnValue=1;
        return false;
    }

    QStringList allFiles=inputDir.entryList(QDir::Files, QDir::Name);
    OUT(QString::number(allFiles.count()) + " files found.");

    if (allFiles.count()==0)
    {
        OUT("Nothing to do.");
        return true;
    }

    bool processingError=false;

    for (int i=0; i<allFiles.count(); i++)
    {
        int series=0;
        int slice=0;

        QString   sourceFile=inputPath+"/"+allFiles.at(i);
        QFileInfo fileInfo=QFileInfo(sourceFile);
        QString   cleanedName=fileInfo.completeBaseName();

        if (!parseFilename(cleanedName.toStdString(), series, slice))
        {
            OUT("ERROR: Unable to parse file names (" + sourceFile + ")" );
            processingError=true;
            break;
        }

        fileList.append(subSeriesEntry(series, slice, sourceFile, allFiles.at(i)));

        if (series > seriesCount)
        {
            seriesCount=series+1;
        }
    }

    /*
    bool processingError=false;
    for (int i=0; i<allFiles.count(); i++)
    {
        QString sourceFile=inputPath+"/"+allFiles.at(i);
        QString targetFile=outputPath+"/sub"+allFiles.at(i);
        QDir inputDir;
        if (!inputDir.cd(inputPath))
        {
            OUT("ERROR: Cannot enter input directory.");
            returnValue=1;
            return;
        }
    }
    */

    if (!processingError)
    {
        OUT("Highest series: " + QString::number(seriesCount));
    }

    return (!processingError);
}


int subMainClass::getAppendedNumber(std::string input)
{
    int pos=0;

    for (int i=input.length()-1; i>=0; i--)
    {
        if (!isdigit(input.at(i)))
        {
            break;
        }
        pos=i;
    }

    input.erase(0,pos);

    return atoi(input.c_str());
}


bool subMainClass::parseFilename(std::string filename, int& series, int& slice)
{
    series=1;
    slice=1;

    // Check if filename starts with "reco.", as created by the GRASP C++ module.
    // If so, remove it.
    if (filename.find("reco.")==0)
    {
        filename.erase(0,5);
    }

    size_t dotPos=filename.find('.');

    if (dotPos==std::string::npos)
    {
        // TODO: Error reporting
    }

    std::string tmpSlice=filename;

    struct isnotdigit { bool operator()(char c) { return !isdigit(c); } };

    std::string tmpSeries=filename;

    // Split into series and slice at the dot
    tmpSeries.erase(dotPos, std::string::npos);
    tmpSlice.erase(0,dotPos+1);

    // Keep only the number at the end of the string and remove any preceeding characters
    series=getAppendedNumber(tmpSeries);

    // Keep only the number at the end of the string and remove any preceeding characters
    slice=getAppendedNumber(tmpSlice);

    // Error checking for reasonable value range of slice and series number
    if ((slice<0) || (series<0))
    {
        //LOG("ERROR: Invalid series or slice number (series " << series << ", slice " << slice << ")");
        return false;
    }

    return true;
}


