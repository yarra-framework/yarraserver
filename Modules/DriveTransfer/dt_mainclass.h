#ifndef PT_MAINCLASS_H
#define PT_MAINCLASS_H

#include <QtCore>


class dtMainClass : public QObject
{
    Q_OBJECT
public:
    explicit dtMainClass(QObject *parent = 0);

    void printUsage();
    void processTransfer();

    bool readConfig();

    QProcess process;
    QStringList args;

    int returnValue;

signals:
    void finished();

public slots:
    void run();

};

#endif // PT_MAINCLASS_H
