#ifndef SDT_MAINCLASS_H
#define SDT_MAINCLASS_H

#include <QtCore>

#define DCMTAG_PatientName        "(0010,0010)"
#define DCMTAG_PatientID          "(0010,0020)"
#define DCMTAG_PatientBirthday    "(0010,0030)"
#define DCMTAG_PatientSex         "(0010,0040)"
#define DCMTAG_PatientSize        "(0010,1020)"
#define DCMTAG_PatientWeight      "(0010,1030)"

#define DCMTAG_SliceThickness     "(0018,0050)"
#define DCMTAG_PixelSpacing       "(0028,0030)"
#define DCMTAG_SliceLocation      "(0020,1041)"
#define DCMTAG_PatientPosition    "(0018,5100)"
#define DCMTAG_ImageOrientation   "(0020,0037)"

#define DCMTAG_AccessionNumber    "(0020,0012)"
#define DCMTAG_Modality           "(0008,0060)"

#define DCMTAG_StudyDescription   "(0008,1030)"
#define DCMTAG_SeriesDescription  "(0008,103E)"

#define DCMTAG_ImageType          "(0008,0008)"

#define DCMTAG_SeriesNumber       "(0020,0011)"
#define DCMTAG_AcquisitionNumber  "(0020,0012)"
#define DCMTAG_ImageNumber        "(0020,0013)"

#define DCMTAG_ImageTime          "(0008,0033)"
#define DCMTAG_ImageDate          "(0008,0023)"
#define DCMTAG_StudyTime          "(0008,0030)"
#define DCMTAG_StudyDate          "(0008,0020)"
#define DCMTAG_SeriesTime         "(0008,0031)"
#define DCMTAG_SeriesDate         "(0008,0021)"
#define DCMTAG_AcquisitionTime    "(0008,0032)"
#define DCMTAG_AcquisitionDate    "(0008,0022)"





class sdtMainClass : public QObject
{
    Q_OBJECT
public:
    explicit sdtMainClass(QObject *parent = 0);

    bool runCommand(QString cmd);
    void printUsage();
    void processPostProc();

    QProcess process;
    QStringList args;

    int returnValue;

    QString composeDICOMTags(QString fname);
    bool readConfiguration();
    bool readTWIXInformation();


signals:
    void finished();

public slots:
    void readOutput();
    void run();

private:
    QString patientName;
    QString patientID;
    QString patientSex;
    QString patientBirthday;
    QString patientWeight;
    QString patientSize;
    QString patientPosition;
    QString imageOrientation;

    QString sliceThickness;
    QString pixelSpacing;
    QString sliceLocation;

    QString accessionNumber;
    QString modality;

    QString studyDate;
    QString studyTime;
    QString seriesDate;
    QString seriesTime;
    QString imageDate;
    QString imageTime;
    QString acquisitionDate;
    QString acquisitionTime;

    QString studyDescription;
    QString seriesDescription;
    QString imageType;

    int seriesNumber;
    int imageNumber;
    int acquisitionNumber;
};

#endif // SDT_MAINCLASS_H
