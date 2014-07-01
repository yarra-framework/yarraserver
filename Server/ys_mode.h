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

    QString getFullCmdLine();

    QString name;

    QString reconBinary;
    QString reconArguments;

    QString parseString(QString input);

    ysProcess* currentProcess;
    ysJob*     currentJob;

};



#endif // YS_MODE_H
