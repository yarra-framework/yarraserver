#include "ys_notificationmail.h"
#include "ys_global.h"
#include "ys_job.h"
#include "ys_server.h"


ysNotificationMail::ysNotificationMail()
{
    notificationEnabled=false;
}


void ysNotificationMail::prepare()
{
    // TODO: Check for installation of sendmail

    notificationEnabled=YSRA->staticConfig.notificationEnabled;
    fromField="From: " + YSRA->staticConfig.notificationFromAddress + "\n";
}


void ysNotificationMail::sendSuccessNotification(ysJob* job)
{
    receivers=job->emailNotification;
    subject="Yarra: Reconstruction finished";

    body="The following reconstruction job has been finished successfylly:\n";
    body.append("Patient name: <b>" + job->patientName + "</b>\n");

    sendMail();
}


void ysNotificationMail::sendMail()
{
    if ((receivers.length()==0) || (!notificationEnabled))
    {
        return;
    }

    QString header=fromField;
    header.append("To: " + receivers + "\n");
    header.append("Subject: "+subject + "\n");
    header.append("Content-Type: text/html\n");
    header.append("MIME-Version: 1.0\n");

    QProcess *process_mail;
    process_mail = new QProcess();
    process_mail->start("sendmail -t");
    process_mail->waitForStarted();
    process_mail->write(header.toLatin1());
    process_mail->write(body.toLatin1());
    process_mail->closeWriteChannel();
}
