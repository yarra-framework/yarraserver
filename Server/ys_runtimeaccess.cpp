#include "ys_runtimeaccess.h"


ysServer* ysRuntimeAccess::singleton=0;


ysRuntimeAccess::ysRuntimeAccess()
{
}


ysServer* ysRuntimeAccess::getInstance()
{
    return singleton;
}


void ysRuntimeAccess::setInstance(ysServer* globalInstance)
{
    singleton=globalInstance;
}

