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
    baselineSeries  =true;
    returnValue     =0;
    subtractionMode =SUB_CHOPNEGATIVE;
    seriesOffset=0;
}


subMainClass::~subMainClass()
{
    disposeBaseline();
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

    generateUIDs();

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

    disposeBaseline();

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


void subMainClass::setSeriesStateRange(QString minSeries, QString maxSeries, bool value)
{
    bool convOK=true;

    int minIndex=minSeries.toInt(&convOK);

    if ((!convOK) || (minIndex >= seriesCount))
    {
        return;
    }
    if (minIndex < 0)
    {
        minIndex=0;
    }

    int maxIndex=maxSeries.toInt(&convOK);

    if ((!convOK) || (maxIndex < 0))
    {
        return;
    }

    if (maxIndex >= seriesCount)
    {
        maxIndex=seriesCount-1;
    }

    for (int i=minIndex; i<=maxIndex; i++)
    {
        seriesState.setBit(i, value);
    }
}


bool subMainClass::readConfig(QString modeFilePath)
{
    QSettings settings(modeFilePath, QSettings::IniFormat);

    baselineSeries  =settings.value(SUB_MODE_ID+"/BaselineSeries",   0).toInt();
    keepSourceImages=settings.value(SUB_MODE_ID+"/KeepSourceImages", false).toBool();
    subtractionMode =settings.value(SUB_MODE_ID+"/SubtractionMode",  SUB_CHOPNEGATIVE).toInt();
    seriesOffset    =settings.value(SUB_MODE_ID+"/SeriesOffset",     0).toInt();

    QStringList includeString=settings.value(SUB_MODE_ID+"/IncludeSeries","").toStringList();
    QStringList excludeString=settings.value(SUB_MODE_ID+"/ExcludeSeries","").toStringList();

    seriesState.clear();
    seriesState.resize(seriesCount);
    seriesState.fill(false);

    // Evaluate the include-series setting
    if (includeString.isEmpty())
    {
        // Normally, the stringlist should never be empty. But to be sure...
        seriesState.fill(true);
    }
    else
    {
        if ((includeString.at(0).isEmpty()) || (includeString.at(0).contains("all",Qt::CaseInsensitive)))
        {
            seriesState.fill(true);
        }
        else
        {
            for (int i=0; i<includeString.count(); i++)
            {
                if (includeString.at(i).contains("-"))
                {
                    QStringList rangeStringList=includeString.at(i).split("-");
                    QString minString=rangeStringList.at(0);
                    QString maxString=rangeStringList.at(1);

                    setSeriesStateRange(minString, maxString, true);
                }
                else
                {
                    bool convOK=true;
                    int seriesIndex=includeString.at(i).toInt(&convOK);

                    if ((convOK) && (seriesIndex>=0) && (seriesIndex<seriesCount))
                    {
                        seriesState.setBit(seriesIndex,true);
                    }
                }
            }
        }
    }

    // Evaluate the exclude-series setting
    for (int i=0; i<excludeString.count(); i++)
    {
        // If no entry is given in the config, the stringlist will still contain an empy string
        if ((i==0) && (excludeString.at(0).isEmpty()))
        {
            break;
        }

        if (excludeString.at(i).contains("-"))
        {
            QStringList rangeStringList=excludeString.at(i).split("-");
            QString minString=rangeStringList.at(0);
            QString maxString=rangeStringList.at(1);

            setSeriesStateRange(minString, maxString, false);
        }
        else
        {
            bool convOK=true;
            int seriesIndex=excludeString.at(i).toInt(&convOK);

            if ((convOK) && (seriesIndex>=0) && (seriesIndex<seriesCount))
            {
                seriesState.setBit(seriesIndex,false);
            }
        }
    }

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
    baselineImages.clear();

    for (int i=0; i<fileList.count(); i++)
    {
        if (fileList.at(i).seriesNumber==baselineSeries)
        {
            DcmFileFormat fileformat;
            OFCondition status = fileformat.loadFile(fileList.at(i).fileName.toUtf8());

            if (!status.good())
            {
                OUT("ERROR: Cannot open baseline file " + fileList.at(i).fileName);
                return false;
            }

            Uint16 rows=0;
            Uint16 cols=0;
            fileformat.getDataset()->findAndGetUint16(DCM_Rows, rows);
            fileformat.getDataset()->findAndGetUint16(DCM_Columns, cols);

            DcmDataset *dataset = fileformat.getDataset();
            const Uint16* pixelData = NULL;

            if (!dataset->findAndGetUint16Array(DCM_PixelData, pixelData).good())
            {
                OUT("ERROR: Cannot read baseline data from " + fileList.at(i).fileName);
                return false;
            }

            Uint16* buffer=new Uint16[rows*cols];
            memcpy(buffer,pixelData,rows*cols*sizeof(Uint16));

            baselineImages.append(subBaselineSlice(cols,rows,fileList.at(i).sliceNumber,buffer));
        }
    }

    return true;
}


void subMainClass::disposeBaseline()
{
    for (int i=0; i<baselineImages.count(); i++)
    {
        if (baselineImages.at(i).pixelData != 0)
        {
            delete[] baselineImages.at(i).pixelData;
        }
    }

    baselineImages.clear();
}


bool subMainClass::createSubtractions(QString outputPath)
{
    int processedFiles=0;

    for (int i=0; i<fileList.count(); i++)
    {
        if ((fileList.at(i).seriesNumber > baselineSeries)
            && (seriesState.at(fileList.at(i).seriesNumber))
           )
        {
            int newSeriesNumber=fileList.at(i).seriesNumber;

            // When keeping the input images, add the input series count as offset for the series number
            if (keepSourceImages)
            {
                newSeriesNumber+=seriesCount;
            }

            QString outputFilename=outputPath+QString("/sub_%1.%2.dcm").arg(newSeriesNumber,4,10,QLatin1Char('0')).arg(fileList.at(i).sliceNumber,4,10,QLatin1Char('0'));

            //OUT("Processing " + fileList.at(i).fileName);
            processDICOM(fileList.at(i).fileName, outputFilename, fileList.at(i).sliceNumber, fileList.at(i).seriesNumber, newSeriesNumber);
            processedFiles++;
        }
    }

    OUT(QString::number(processedFiles) + " files processed.");

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

    DcmDataset *dataset = fileformat.getDataset();
    const Uint16* pixelData = NULL;

    if (!dataset->findAndGetUint16Array(DCM_PixelData, pixelData).good())
    {
        OUT("ERROR: Cannot read pixel data from " + inFilename);
        return false;
    }

    //OUT("Image dim = " + QString::number(cols) + " x " + QString::number(rows));

    int baselineIndex=-1;

    for (int i=0; i<baselineImages.count(); i++)
    {
        if (baselineImages.at(i).slice==slice)
        {
            baselineIndex=i;
            break;
        }
    }

    if (baselineIndex<0)
    {
        OUT("ERROR: Unable to find reference slice for file " + inFilename);
        return false;
    }

    if ( (rows!=baselineImages.at(baselineIndex).height)
         || (cols!=baselineImages.at(baselineIndex).width) )
    {
        OUT("ERROR: Image dimensions do not match with baseline for " + inFilename);
        return false;
    }

    Uint16* modData=new Uint16[rows*cols];
    Uint16* baselineData=baselineImages.at(baselineIndex).pixelData;

    int    idx  =0;
    int    value=0;

    switch (subtractionMode)
    {
    case SUB_CHOPNEGATIVE:
    default:
        for (int y=0; y<rows; y++)
        {
            for (int x=0; x<cols; x++)
            {
                idx=y*cols+x;

                value=(pixelData[idx] - baselineData[idx]);
                if (value<0)
                {
                    value=0;
                }

                modData[idx]=value;
            }
        }
        break;

    case SUB_ABSOLUTE:
        for (int y=0; y<rows; y++)
        {
            for (int x=0; x<cols; x++)
            {
                idx=y*cols+x;

                value=abs(pixelData[idx] - baselineData[idx]);
                modData[idx]=value;
            }
        }
        break;
    }

    dataset->putAndInsertUint16Array(DCM_PixelData, modData, rows*cols);

    // Write new DICOM tags for modified images
    dataset->putAndInsertUint16(DCM_SeriesNumber,      outSeries + seriesOffset);
    dataset->putAndInsertString(DCM_StudyID,           studyUID.toUtf8());
    dataset->putAndInsertString(DCM_StudyInstanceUID,  studyUID.toUtf8());

    char uid[100];
    dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
    dataset->putAndInsertString(DCM_SOPInstanceUID,  uid);

    OFString seriesDescription;
    if (!dataset->findAndGetOFString(DCM_SeriesDescription, seriesDescription).good())
    {
        OUT("WARNING: Unable to get SeriesDescription tag.")
    }
    else
    {
        seriesDescription="SUB "+seriesDescription;
        dataset->putAndInsertOFStringArray(DCM_SeriesDescription, seriesDescription);
    }

    OFString scanningSequence;
    if (!dataset->findAndGetOFString(DCM_ScanningSequence, scanningSequence).good())
    {
        OUT("WARNING: Unable to get ScanningSequence tag.")
    }
    else
    {
        scanningSequence="SUB "+scanningSequence;
        dataset->putAndInsertOFStringArray(DCM_ScanningSequence, scanningSequence);
    }

    if ((inSeries<0) || (inSeries>=studyUID.count()))
    {
        OUT("ERROR: Series number exceeds UID array.");
        return false;
    }
    dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesUIDList.at(inSeries).toUtf8());

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

    if (!processingError)
    {
        OUT("Highest series is " + QString::number(seriesCount)+".");
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


