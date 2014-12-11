#ifndef SDT_MAINCLASS_H
#define SDT_MAINCLASS_H

#include <QtCore>


class sdtMainClass : public QObject
{
    Q_OBJECT
public:
    explicit sdtMainClass(QObject *parent = 0);

    bool runCommand(QString cmd);
    void printUsage();
    void processPostProc();

    QProcess process;
    QStringList args;

    int returnValue;

    QString composeDICOMTags(QString fname);
    bool readConfiguration();
    bool readTWIXInformation();


signals:
    void finished();

public slots:
    void readOutput();
    void run();

private:

    QString patientName;
    QString patientID;
};

#endif // SDT_MAINCLASS_H
