#ifndef YS_DYNAMICCONFIG_H
#define YS_DYNAMICCONFIG_H

#include <QtCore>

class ysDynamicConfig
{
public:
    ysDynamicConfig();

    void updateDynamicConfigList();
    bool validateAllReconModes();
    bool isReconModeAvailable(QString reconMode);
    bool readDynamicConfig(QString reconMode);

protected:
    QStringList availableReconModes;

};

#endif // YS_DYNAMICCONFIG_H

