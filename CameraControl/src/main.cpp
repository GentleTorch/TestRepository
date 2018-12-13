#include "JQHttpServer.h"
#include "netdvr.h"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

#ifndef QT_NO_SSL
    JQHttpServer::SslServerManage sslServerManage(2); // 设置最大处理线程数，默认2个

    sslServerManage.setHttpAcceptedCallback([](const QPointer<JQHttpServer::Session>& session) {
        // 回调发生在新的线程内，不是主线程，请注意线程安全
        // 若阻塞了此回调，那么新的连接将不会得到处理（默认情况下有2个线程可以阻塞2次，第3个连接将不会被处理）
        if (session->requestUrl().compare(QString("/pc_vas/CruiseControl")) == 0) {
            qDebug() << "rightful url";
        } else {
            qDebug() << "unsupported url";
            return;
        }

        session->replyText(QString("url:%1\ndata:%2").arg(session->requestUrl(), QString(session->requestBody())));

        qDebug() << QDateTime::currentDateTime().toLocalTime().toString();
        qDebug() << session->requestUrl();

        QJsonParseError json_error;
        QJsonDocument parse_document = QJsonDocument::fromJson(session->requestBody(), &json_error);
        qDebug() << parse_document.object();
        QJsonObject obj = parse_document.object();
        bool cameraSpin = obj["CameraSpin"].toBool();
        bool cameraDetect = obj["CameraDetect"].toBool();

        qDebug() << cameraDetect << cameraSpin;

        Netdvr dvr("192.168.100.64", 8000, "admin", "Pdio#179530!!");
        if (cameraSpin) {
            dvr.runCruise();

        } else {
            dvr.stopCruise();
        }
    });

    qDebug() << "listen:" << sslServerManage.listen(QHostAddress("192.168.100.100"), 24684, ":/server.crt", ":/server.key");

    return a.exec();
#else
    return 0;
#endif
}
