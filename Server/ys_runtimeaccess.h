#ifndef YS_RUNTIMEACCESS_H
#define YS_RUNTIMEACCESS_H

class ysServer;

// Singleton class for convenient access to the server object
class ysRuntimeAccess
{
public:
    ysRuntimeAccess();

    static ysServer* getInstance();
    static void setInstance(ysServer* globalInstance);

protected:
    static ysServer* singleton;

};



#endif // YS_RUNTIMEACCESS_H
