#ifndef IP_MAINCLASS_H
#define IP_MAINCLASS_H

#include <QtCore>


class ipMainClass : public QObject
{
    Q_OBJECT
public:
    explicit ipMainClass(QObject *parent = 0);

    void printUsage();
    void processPatch();

    QStringList args;

    QString iniFilename;
    QString accValue;

    int returnValue;

signals:
    void finished();

public slots:
    void run();

};

#endif // IP_MAINCLASS_H
