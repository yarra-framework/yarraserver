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
    Uint16* pixelData;
    int     width;
    int     height;
};


class subMainClass : public QObject
{
    Q_OBJECT

public:
    explicit subMainClass(QObject *parent = 0);

    void printUsage();
    void processPostProc();

    bool readConfig(QString modeFilePath);
    bool generateFileList(QString inputPath);
    bool generateUIDs();
    bool readBaseline();
    bool createSubtractions(QString outputPath);
    bool processDICOM(QString inFilename, QString outFilename, int slice, int inSeries, int outSeries);
    bool moveSourceImages(QString outputPath);

    int  getAppendedNumber(std::string input);
    bool parseFilename(std::string filename, int& series, int& slice);

    /*
    bool readDICOM(QString filename, QString outputFilename);
    */

    QStringList           args;
    int                   returnValue;

    QList<subSeriesEntry> fileList;

    QString               studyUID;
    QStringList           seriesUIDList;
    int                   seriesCount;

    int                   baselineSeries;
    bool                  keepSourceImages;
    QBitArray             seriesState;

signals:
    void finished();

public slots:
    void run();

};

#endif // SF_MAINCLASS_H

