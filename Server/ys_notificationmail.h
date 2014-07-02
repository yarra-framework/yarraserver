#ifndef YS_NOTIFICATIONMAIL_H
#define YS_NOTIFICATIONMAIL_H

#include <QtCore>


class ysJob;

class ysNotificationMail
{
public:
    ysNotificationMail();

    void prepare();
    void sendSuccessNotification(ysJob* job);


    void sendMail();

    bool notificationEnabled;
    QString fromField;

    QString receivers;
    QString subject;
    QString body;

};

#endif // YS_NOTIFICATIONMAIL_H
