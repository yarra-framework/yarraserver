#ifndef SF_MAINCLASS_H
#define SF_MAINCLASS_H

#include <QtCore>

#include "dcmtk/dcmdata/dctk.h"


class subSeriesEntry
{
public:
    int     seriesNumber;
    int     sliceNumber;
    QString fileName;
    QString fileNameWithoutPath;

    subSeriesEntry()
    {
        seriesNumber=-1;
        sliceNumber=-1;
        fileName="";
        fileNameWithoutPath="";
    }

    subSeriesEntry(int _seriesNumber, int _sliceNumber, QString _fileName, QString _fileNameWithoutPath)
    {
        seriesNumber=_seriesNumber;
        sliceNumber=_sliceNumber;
        fileName=_fileName;
        fileNameWithoutPath=_fileNameWithoutPath;
    }
};


class subBaselineSlice
{
public:
    int     width;
    int     height;
    int     slice;
    Uint16* pixelData;

    subBaselineSlice()
    {
        width    =-1;
        height   =-1;
        slice    =-1;
        pixelData=0;
    }

    subBaselineSlice(int _width, int _height, int _slice, Uint16* _pixelData)
    {
        width=_width;
        height=_height;
        slice=_slice;
        pixelData=_pixelData;
    }
};


class subMainClass : public QObject
{
    Q_OBJECT
public:
    explicit subMainClass(QObject *parent = 0);
    ~subMainClass();

    enum SubtractionModes
    {
        SUB_CHOPNEGATIVE=0,
        SUB_ABSOLUTE    =1
    };

    void printUsage();
    void processPostProc();

    bool readConfig(QString modeFilePath);
    bool generateFileList(QString inputPath);
    bool generateUIDs();
    bool readBaseline();
    void disposeBaseline();
    bool createSubtractions(QString outputPath);
    bool processDICOM(QString inFilename, QString outFilename, int slice, int inSeries, int outSeries);
    bool moveSourceImages(QString outputPath);

    int  getAppendedNumber(std::string input);
    bool parseFilename(std::string filename, int& series, int& slice);

    void setSeriesStateRange(QString minSeries, QString maxSeries, bool value);

    QStringList             args;
    int                     returnValue;

    QList<subSeriesEntry>   fileList;
    QList<subBaselineSlice> baselineImages;

    QString                 studyUID;
    QStringList             seriesUIDList;
    int                     seriesCount;

    int                     baselineSeries;
    bool                    keepSourceImages;
    int                     subtractionMode;
    int                     seriesOffset;

    QBitArray               seriesState;

signals:
    void finished();

public slots:
    void run();

};

#endif // SF_MAINCLASS_H

