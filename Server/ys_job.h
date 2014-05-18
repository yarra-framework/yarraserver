#ifndef YS_JOB_H
#define YS_JOB_H

#include <QtCore>


class ysJob
{
public:
    ysJob();

    QString     taskFile;
    QString     scanFile;
    QStringList adjustmentFiles;

    QString emailNotification;
    QString accNumber;

    QString reconMode;
    QString systemName;

    QDateTime submissionTime;
    QDateTime processingStart;
    QDateTime processingEnd;

    QString uniqueID;

    QString reconCallCmd;


protected:
    void generateUniqueID();


};


#endif // YS_JOB_H

