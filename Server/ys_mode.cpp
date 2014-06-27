#include "ys_mode.h"

ysMode::ysMode()
{
    name="!!INVALID";

    reconBinary="";
    reconArguments="";

}


void ysMode::readModeSettings(QString modeName)
{
    // TODO
}


QString ysMode::getFullCmdLine()
{
    return reconBinary + " " + reconArguments;
}
