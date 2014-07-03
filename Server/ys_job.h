#ifndef YS_JOB_H
#define YS_JOB_H

#include <QtCore>


class ysJob
{
public:
    ysJob();
    ~ysJob();

    QString     taskFile;
    QString     scanFile;
    QStringList adjustmentFiles;

    QString emailNotification;
    QString accNumber;
    QString patientName;
    QString protocolName;

    QString reconMode;
    QString reconReadableName;
    QString systemName;

    QDateTime submissionTime;
    QDateTime processingStart;
    QDateTime processingEnd;

    qint64 submittedScanFileSize;

    QString taskID;
    QString uniqueID;    
    QString getUniqueTaskID();
    QString getTaskID();

    QString reconCallCmd;
    bool    storeProcessedFile;

    bool readTaskFile(QString filename);

    QStringList getAllFiles();

    void setProcessingEnd();
    qint64 durationSec;
    QString duration;

    QString errorReason;
    void setErrorReason(QString reason);

protected:
    void generateTaskID();
    void logJobInformation();
};



inline QString ysJob::getUniqueTaskID()
{
    return taskID + "_" + uniqueID;
}


inline QString ysJob::getTaskID()
{
    return taskID;
}

inline void ysJob::setErrorReason(QString reason)
{
    errorReason=reason;
}


#endif // YS_JOB_H


