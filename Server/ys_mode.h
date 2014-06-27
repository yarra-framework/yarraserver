#ifndef YS_MODE_H
#define YS_MODE_H

#include <QtCore>


class ysMode
{   
public:
    ysMode();

    void readModeSettings(QString modeName);
    QString getFullCmdLine();

    QString name;

    QString reconBinary;
    QString reconArguments;

    QString parseString(QString input);

};



#endif // YS_MODE_H
