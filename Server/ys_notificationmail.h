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
    void sendErrorNotification(ysJob* job);

    void sendDiskSpaceNotification(QString dirs);
    void sendDiskErrorNotification(QString dirs);

    void sendMail(bool highPriority=false, QString attachFilename="");

    bool notificationEnabled;
    QString fromField;

    QString receivers;
    QString subject;
    QString body;
    QString footer;

};

#endif // YS_NOTIFICATIONMAIL_H
