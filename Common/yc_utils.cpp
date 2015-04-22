#include "yc_utils.h"


// Copies a directory recursively to a target destination (including subfolders). Since the implementation
// uses recursive function calling, the depth of the function calls is limited to 30.

// Based on source code from:
// https://qt.gitorious.org/qt-creator/qt-creator/source/1a37da73abb60ad06b7e33983ca51b266be5910e:src/app/main.cpp#L13-189

bool ycUtils::copyRecursively(const QString &srcFilePath, const QString &tgtFilePath, int recursionDepth)
{
    // Stop after 30 recursions (to be on the safe side regarding stack space consumption)
    if (recursionDepth>30)
    {
        return true;
    }


    // If input path describes a directory, try to copy the directory
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir())
    {
        // Try to create the target directory. Return if it fails.
        QDir targetDir(tgtFilePath);        
        targetDir.cdUp();
        if (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName()))
        {
            return false;
        }

        // Loop over all items in the input directory (files and dirs)
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames)
        {
            const QString newSrcFilePath=srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath=tgtFilePath + QLatin1Char('/') + fileName;

            // Perform the recursive function call, increase the recursion counter
            if (!copyRecursively(newSrcFilePath, newTgtFilePath,recursionDepth+1))
            {
                return false;
            }
        }
    }
    else
    {
        // If the input path is just a file, copy it using QT's standard function
        if (!QFile::copy(srcFilePath, tgtFilePath))
        {
            return false;
        }
    }

    return true;
}

