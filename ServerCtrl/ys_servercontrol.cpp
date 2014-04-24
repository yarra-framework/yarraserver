#include "ys_servercontrol.h"
#include "../Server/ys_global.h"
#include "../Server/ys_controlapi.h"


ysServerControl::ysServerControl(QObject *parent) :
    QObject(parent)
{
}


void ysServerControl::run()
{
    // TODO

    YS_OUT("Yarra Server Control");
    YS_OUT("====================");
    YS_OUT("");



    quit();
}


void ysServerControl::quit()
{
    emit finished();
}


