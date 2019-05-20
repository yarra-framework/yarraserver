#ifndef YS_DYNAMICCONFIG_H
#define YS_DYNAMICCONFIG_H

#include <QtCore>

class ysDynamicConfig
{
public:
    ysDynamicConfig();

    bool prepare();

    void updateDynamicConfigList();
    bool validateAllReconModes();
    bool isReconModeAvailable(QString reconMode);

    QStringList availableReconModes;

protected:

    QDir modesDir;

};

#endif // YS_DYNAMICCONFIG_H

