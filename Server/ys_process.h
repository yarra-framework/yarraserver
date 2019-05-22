#ifndef YS_PROCESS_H
#define YS_PROCESS_H

#include <QtCore>

#include "ys_job.h"
#include "ys_mode.h"

// For timeout measurements
#include <stdio.h>
#include <time.h>


class ysProcess : public QObject
{
    Q_OBJECT

    enum ysTerminationReason
    {
        YS_TERMINATION_NONE=0,
        YS_TERMINATION_MEMKILL,
        YS_TERMINATION_OUTPUTLIMIT,
        YS_TERMINATION_OUTPUTTIMEOUT
    };

public:
    ysProcess();
    ~ysProcess();

    bool prepareReconstruction(ysJob* job);
    bool runPreProcessing ();
    bool runReconstruction();
    bool runPostProcessing();
    bool runTransfer();

    bool prepareOutputDirs(ysJob::ysJobState jobState);
    bool cleanTmpDir();

    void finish();

    QString    reconDir;
    QString    transferDir;
    QString    tmpDir;

    ysMode*    mode;

    QString    callCmd;
    QProcess*  process;

    QTimer*    memcheckTimer;

    bool       executeCommand();

    int        getUsedMemoryMB();
    int        getPhysicalMemoryMB();
    int        totalPhysicalMemory;
    double     memkillThreshold;
    int        memoryDuringKill;
    bool       disableMemKill;

    QString    processWorkingDirectory;
    QString    currentModuleType;

    // Variables for detecting module problems
    int        outputLines;
    time_t     lastOutput;
    int        maxOutputIdleTime;
    bool       errorReceived;
    int        terminationReason;

    void       haltAnyProcess();

public slots:
    void logOutput();
    void checkMemory();
    void receiveProcessError(QProcess::ProcessError processError);

};


#endif // YS_PROCESS_H
