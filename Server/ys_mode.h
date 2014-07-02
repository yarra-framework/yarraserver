#ifndef YS_MODE_H
#define YS_MODE_H

#include <QtCore>

#include "ys_job.h"

class ysProcess;

class ysMode
{   
public:
    ysMode();

    void setProcess(ysProcess* process);

    bool readModeSettings(QString modeName, ysJob* job);
    void parseCmdlines();

    QString getReconCmdLine();
    QString getTransferCmdLine();
    QString getPostprocCmdLine(int i);

    QString name;

    QString reconBinary;
    QString reconArguments;

    int postprocCount;
    QStringList postprocBinary;
    QStringList postprocArguments;

    QString transferBinary;
    QString transferArguments;

    QString parseString(QString input, QString postprocIn="", QString postprocOut="");

    ysProcess* currentProcess;
    ysJob*     currentJob;

};



#endif // YS_MODE_H
