#ifndef IP_MAINCLASS_H
#define IP_MAINCLASS_H

#include <QtCore>


class ipMainClass : public QObject
{
    Q_OBJECT
public:
    explicit ipMainClass(QObject *parent = 0);

    void printUsage();
    void processTransfer();
    bool readConfig();

    QStringList args;

    QString sourceIniFilename;
    QString targetIniFilename;
    QString replacementValue;
    QStringList keysToPatch;

    int returnValue;

signals:
    void finished();

public slots:
    void run();

};

#endif // IP_MAINCLASS_H
