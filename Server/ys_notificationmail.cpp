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

    footer = "<div style=\"color: #000; font-family: Arial, Helvetica, sans-serif;\">\n\
<p style=\"font-size:85%; border-top: 1px solid #999; padding-top: 6px; margin-top: 30px;\">\n\
<b style=\"color: #580F8B;\">YarraServer</b>&nbsp;&nbsp;-&nbsp;&nbsp;Version " + QString(YS_VERSION) +"</p></div>\n";

}


void ysNotificationMail::sendDiskSpaceNotification(QString dirs)
{
    if (YSRA->staticConfig.notificationErrorMail != "")
    {
        receivers = YSRA->staticConfig.notificationErrorMail;
    }
    else
    {
        return;
    }

    subject="Yarra: Low diskspace";

    body="<div style=\"color: #000; font-family: Arial, Helvetica, sans-serif;\">\n\
<p><b>Warning: </b>The following server reports <b>low diskspace</b>, which may soon halt the processing of cases:</p>\n\
<p><table border=\"0\" style=\"background-color: #EEE; border: 1px solid #000; padding-left: 6px; padding-right: 6px; border-left: 6px solid #F90; margin-top: 10px;\">\n\
<tr><td style=\"padding-right: 20px;\">Server name:</td><td><b>%prm1%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Server type:</td><td>%prm2%</td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Affected directories:</td><td>%prm3%</td></tr>\n\
</table>&nbsp;<br />\n\
It is advised to immediately resolve this situation to avoid possible processing delays.</p>\n\
</div>\n";

    body.replace("%prm1%", YSRA->staticConfig.serverName);
    body.replace("%prm2%", YSRA->staticConfig.serverType);

    QString frmtDir=dirs;
    frmtDir.replace("; ","<br />");
    body.replace("%prm3%", frmtDir);

    sendMail(true);
}


void ysNotificationMail::sendDiskErrorNotification(QString dirs)
{
    if (YSRA->staticConfig.notificationErrorMail != "")
    {
        receivers = YSRA->staticConfig.notificationErrorMail;
    }
    else
    {
        return;
    }

    subject="Yarra: Out of diskspace";

    body="<div style=\"color: #000; font-family: Arial, Helvetica, sans-serif;\">\n\
<p><b>Important: </b>The following server is <b>out of diskspace</b>:</p>\n\
<p><table border=\"0\" style=\"background-color: #EEE; border: 1px solid #000; padding-left: 6px; padding-right: 6px; border-left: 6px solid #CC0000; margin-top: 10px;\">\n\
<tr><td style=\"padding-right: 20px;\">Server name:</td><td><b>%prm1%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Server type:</td><td>%prm2%</td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Affected directories:</td><td>%prm3%</td></tr>\n\
</table>&nbsp;<br />\n\
Cases will not be processed until sufficient diskspace becomes available.</p>\n\
</div>\n";

    body.replace("%prm1%", YSRA->staticConfig.serverName);
    body.replace("%prm2%", YSRA->staticConfig.serverType);

    QString frmtDir=dirs;
    frmtDir.replace("; ","<br />");
    body.replace("%prm3%", frmtDir);

    sendMail(true);
}


void ysNotificationMail::sendSuccessNotification(ysJob* job)
{
    receivers=job->emailNotification;
    subject="Yarra: Task finished";

    body="<div style=\"color: #000; font-family: Arial, Helvetica, sans-serif;\">\n\
<p>The following reconstruction task has finished:</p>\n\
<p><table border=\"0\" style=\"background-color: #EEE; border: 1px solid #000; padding-left: 6px; padding-right: 6px; border-left: 6px solid #580F8B; margin-top: 10px;\">\n\
<tr><td style=\"padding-right: 20px;\">Patient name:</td><td><b>%prm1%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">ACC #:</td><td><b>%prm2%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Mode:</td><td><b>%prm3%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">MR system:</td><td>%prm4%</td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Acquisition time:</td><td>%prm5%</td></tr>\n\
<tr valign=\"top\"><td style=\"padding-right: 20px; padding-top: 20px;\">Processed by:</td><td style=\"padding-top: 20px;\">%prm6%</td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Duration:</td><td>%prm7%</td></tr>\n\
</table>&nbsp;<br />\n\
<span style=\"color: #888;\">TaskID: %prm8%</span></p></div>\n";

    body.replace("%prm1%", job->patientName);
    body.replace("%prm2%", job->accNumber);
    body.replace("%prm3%", job->reconReadableName);
    body.replace("%prm4%", job->systemName);
    QString timeString=job->submissionTime.toString("dd.MM.yyyy hh:mm:ss");
    body.replace("%prm5%", timeString);
    body.replace("%prm6%", YSRA->staticConfig.serverName);
    body.replace("%prm7%", job->duration);

    QString jobID=job->taskID + "_" + job->uniqueID;
    body.replace("%prm8%", jobID);

    sendMail();
}



void ysNotificationMail::sendErrorNotification(ysJob* job)
{
    receivers=job->emailNotification;

    // Append the general error receiver to the list of receivers
    if (YSRA->staticConfig.notificationErrorMail.length()>0)
    {
        // Add comma only if there are any other receivers
        if (receivers.length()>0)
        {
            receivers += ", ";
        }
        receivers += YSRA->staticConfig.notificationErrorMail;
    }

    subject="Yarra: Failure of task";

    body="<div style=\"color: #000; font-family: Arial, Helvetica, sans-serif;\">\n\
<p><b>Important:</b> The following reconstruction task was <b>not</b> successful:</p>\n\
<p><table border=\"0\" style=\"background-color: #EEE; border: 1px solid #000; padding-left: 6px; padding-right: 6px; border-left: 6px solid #CC0000; margin-top: 10px;\">\n\
<tr><td style=\"padding-right: 20px;\">Patient name:</td><td><b>%prm1%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">ACC #:</td><td><b>%prm2%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Mode:</td><td><b>%prm3%</b></td></tr>\n\
<tr><td style=\"padding-right: 20px;\">MR system:</td><td>%prm4%</td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Acquisition time:</td><td>%prm5%</td></tr>\n\
<tr><td style=\"padding-right: 20px;\">Failure Reason:</td><td><b>%prm6%</b></td></tr>\n\
<tr valign=\"top\"><td style=\"padding-right: 20px; padding-top: 20px;\">Processed by:</td><td style=\"padding-top: 20px;\">%prm7%</td></tr>\n\
</table>&nbsp;<br />\n\
Detailed failure information can be found in the attached log file.</p>\n\
<p>&nbsp;<br /><span style=\"color: #888;\">TaskID: %prm8%</span></p></div>\n";

    body.replace("%prm1%", job->patientName);
    body.replace("%prm2%", job->accNumber);
    body.replace("%prm3%", job->reconReadableName);
    body.replace("%prm4%", job->systemName);

    QString timeString=job->submissionTime.toString("dd.MM.yyyy hh:mm:ss");
    body.replace("%prm5%", timeString);
    body.replace("%prm6%", job->errorReason);
    body.replace("%prm7%", YSRA->staticConfig.serverName);

    QString jobID=job->taskID + "_" + job->uniqueID;
    body.replace("%prm8%", jobID);

    sendMail(true, YSRA->log.getTaskLogFilename());
}


void ysNotificationMail::sendMail(bool highPriority, QString attachFilename)
{
    if ((receivers.length()==0) || (!notificationEnabled))
    {
        return;
    }

    bool attachFile=false;

    if (attachFilename.length()>0)
    {
        attachFile=true;
    }

    QString boundaryID="sdfhsdhty4wsefg#YARRASERVER##sfgi0t3tgvb43vj";

    QString header=fromField;
    header.append("To: " + receivers + "\n");
    header.append("Subject: "+subject + "\n");
    header.append("MIME-Version: 1.0\n");

    if (highPriority)
    {
        header.append("X-Priority: 1 (Highest)\n");
        header.append("X-MSMail-Priority: High\n");
        header.append("Importance: High\n");
    }

    if (attachFile)
    {
        header.append("Content-Type: MULTIPART/MIXED; BOUNDARY=\"" + boundaryID + "\"\n");
        header.append("\n");
        header.append("--"+ boundaryID +"\n");
    }
    header.append("Content-Type: text/html; charset=ISO-8859-1\n");
    header.append("\n");

    QProcess *process_mail=new QProcess();
    process_mail->start("sendmail -t");
    process_mail->waitForStarted();

    process_mail->write(header.toLatin1());
    process_mail->write(QString("<html><body>\n").toLatin1());
    process_mail->write(body.toLatin1());
    process_mail->write(footer.toLatin1());
    process_mail->write(QString("</body></html>\n").toLatin1());

    if (attachFile)
    {
        QString attachHeader="--"+ boundaryID + "\n";
        attachHeader+="Content-Type: text/plain; charset=US-ASCII; name=\"log.txt\"\n";
        attachHeader+="Content-Disposition: attachment; filename=\"log.txt\"\n";
        attachHeader+="\n";
        process_mail->write(attachHeader.toLatin1());

        YSRA->log.flushLogs();

        QFile attachFile(attachFilename);
        if (attachFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            while (!attachFile.atEnd())
            {
                QByteArray line = attachFile.readLine();
                process_mail->write(line);
            }
        }
        process_mail->write(QString("--"+ boundaryID + "--\n").toLatin1());
    }

    process_mail->write(QString(".\n").toLatin1());
    //process_mail->closeWriteChannel();
    process_mail->waitForFinished();
    YS_FREE(process_mail);
}
