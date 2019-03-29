#ifndef YS_JOB_H
#define YS_JOB_H

#include <QtCore>


class ysJob
{
public:

    enum ysJobType
    {
        YS_JOBTYPE_NEW=0,
        YS_JOBTYPE_RESUMED
    };

    enum ysJobState
    {
        YS_STATE_INITIALIZED=0,
        YS_STATE_PREPARED,
        YS_STATE_PREPROCESSING,
        YS_STATE_RECONSTRUCTION,
        YS_STATE_POSTPROCESSING,
        YS_STATE_TRANSFER,
        YS_STATE_COMPLETE
    };

    ysJob();
    ~ysJob();

    ysJobState  state;
    void        setState(ysJobState newState);
    ysJobState  getState();

    ysJobType   type;
    void        setType(ysJobType newType);
    ysJobType   getType();

    QString     taskFile;
    QString     scanFile;
    QStringList adjustmentFiles;

    QString emailNotification;
    QString accNumber;
    QString patientName;
    QString protocolName;
    QString paramValue;

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

    bool readTaskFile(QString filename, bool readCrashedTask=false);

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


inline void ysJob::setState(ysJobState newState)
{
    state=newState;
}


inline ysJob::ysJobState ysJob::getState()
{
    return state;
}


inline void ysJob::setType(ysJobType newType)
{
    type=newType;
}


inline ysJob::ysJobType ysJob::getType()
{
    return type;
}


#endif // YS_JOB_H


