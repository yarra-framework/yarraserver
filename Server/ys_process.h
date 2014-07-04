#ifndef YS_PROCESS_H
#define YS_PROCESS_H

#include <QtCore>

#include "ys_job.h"
#include "ys_mode.h"


class ysProcess : public QObject
{
    Q_OBJECT

public:
    ysProcess();
    ~ysProcess();

    bool prepareReconstruction(ysJob* job);
    bool runReconstruction();
    bool runPostProcessing();
    bool runTransfer();

    bool prepareOutputDirs();
    bool cleanTmpDir();

    void finish();

    QString reconDir;
    QString transferDir;
    QString tmpDir;

    ysMode* mode;

    QString callCmd;
    QProcess process;

    QTimer* memcheckTimer;

    bool executeCommand();

    int getAvailMemoryMB();
    int getTotalMemoryMB();
    int totalMemory;
    double memkillThreshold;
    int memoryDuringKill;
    bool memkillOccured;
    bool disableMemKill;

public slots:
    void logOutput();
    void checkMemory();

};

#endif // YS_PROCESS_H
