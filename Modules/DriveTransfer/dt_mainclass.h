#ifndef DT_MAINCLASS_H
#define DT_MAINCLASS_H

#include <QtCore>


class dtMainClass : public QObject
{
    Q_OBJECT
public:
    explicit dtMainClass(QObject *parent = 0);

    void printUsage();
    void processTransfer();

    bool readConfig();

    QStringList args;

    QString sourcePath;
    QString targetPath;
    QDir targetDir;

    QString taskName;

    int returnValue;


signals:
    void finished();

public slots:
    void run();

};

#endif // DT_MAINCLASS_H
