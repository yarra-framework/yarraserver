#ifndef YC_UTILS_H
#define YC_UTILS_H

#include <QtCore>

class ycUtils
{
public:

    static bool copyRecursively(const QString &srcFilePath,const QString &tgtFilePath, int recursionDepth=0);

};


#endif // YC_UTILS_H

