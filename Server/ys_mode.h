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

    QString getPreprocCmdLine(int i);
    QString getReconCmdLine();
    QString getPostprocCmdLine(int i);
    QString getTransferCmdLine(int i);

    QString name;

    int preprocCount;
    QStringList preprocBinary;
    QStringList preprocArguments;
    bool        preprocDisableMemKill;
    int         preprocMaxOutputIdle;

    QString reconBinary;
    QString reconArguments;
    bool    reconDisableMemKill;
    int     reconMaxOutputIdle;

    int postprocCount;
    QStringList postprocBinary;
    QStringList postprocArguments;
    bool        postprocDisableMemKill;
    int         postprocMaxOutputIdle;

    int transferCount;
    QStringList transferBinary;
    QStringList transferArguments;
    bool    transferDisableMemKill;
    int     transferMaxOutputIdle;

    QString parseString(QString input, QString postprocIn="", QString postprocOut="");

    ysProcess* currentProcess;
    ysJob*     currentJob;

};



#endif // YS_MODE_H
