#ifndef PT_MAINCLASS_H
#define PT_MAINCLASS_H

#include <QtCore>


class ddpMainClass : public QObject
{
    Q_OBJECT
public:
    explicit ddpMainClass(QObject *parent = 0);

    bool runCommand(QString cmd);
    void printUsage();
    void processPostProc();

    QProcess process;
    QStringList args;

    int returnValue;

signals:
    void finished();

public slots:
    void readOutput();
    void run();

};

#endif // PT_MAINCLASS_H
