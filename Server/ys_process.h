#ifndef YS_PROCESS_H
#define YS_PROCESS_H

#include <QtCore>

#include "ys_job.h"
#include "ys_mode.h"


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

    int getUsedMemoryMB();
    int getPhysicalMemoryMB();
    int totalPhysicalMemory;
    double memkillThreshold;
    int memoryDuringKill;
    bool disableMemKill;

    // Variables for detecting module problems
    int   outputLines;
    QTime lastOutput;
    int   maxOutputIdleTime;

    int terminationReason;

    void haltAnyProcess();

public slots:
    void logOutput();
    void checkMemory();

};

#endif // YS_PROCESS_H
