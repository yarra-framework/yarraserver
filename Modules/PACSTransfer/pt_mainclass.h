#ifndef PT_MAINCLASS_H
#define PT_MAINCLASS_H

#include <QtCore>


class ptMainClass : public QObject
{
    Q_OBJECT
public:
    explicit ptMainClass(QObject *parent = 0);

    bool runCommand(QString cmd);
    void printUsage();
    void processTransfer();

    bool readConfig();

    QProcess process;
    QStringList args;

    int returnValue;

    // Settings
    QStringList cfg_AEC;
    QStringList cfg_AET;
    QStringList cfg_IP;
    QStringList cfg_port;
    QStringList cfg_name;
    QStringList cfg_dirmode;

    int cfg_count;
    bool storescuError;

signals:
    void finished();

public slots:
    void readOutput();
    void run();

};

#endif // PT_MAINCLASS_H
