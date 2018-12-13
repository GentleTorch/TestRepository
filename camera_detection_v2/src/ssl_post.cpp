#include "ssl_post.h"

string sslPost(string url, string strData)
{
    QTimer timer;
    timer.setInterval(5000);
    timer.setSingleShot(true);

    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest request;
    QByteArray line;
    line.clear();

    QEventLoop eventLoop;
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone); //证书选项
    conf.setProtocol(QSsl::TlsV1SslV3); //协议，建议用这个TLS与SSL通用的
    request.setSslConfiguration(conf);
    request.setUrl(QUrl(url.c_str()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-AUTH-OID", "3DzedgMnCc1Vtys9t");
    QByteArray append(strData.c_str());
    QNetworkReply* reply = manager->post(request, append);

    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    timer.start();
    eventLoop.exec();

    if (timer.isActive()) {
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error String : " << reply->errorString();
        } else {
            QVariant variant = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            int statusCode = variant.toInt();
            qDebug() << "Status Code : " << statusCode;
            line = reply->readAll();

            qDebug() << "response:" << line;
        }
    } else {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        qDebug() << "Timeout";
    }

    reply->deleteLater();
    delete manager;

    return line.toStdString();
}

QJsonObject sslGetConfig(string url)
{
    string data = sslPost(url, "");
    QJsonParseError json_error;

    QJsonDocument parse_doucment = QJsonDocument::fromJson(QByteArray::fromStdString(data), &json_error);
    if (json_error.error != QJsonParseError::NoError
        || !parse_doucment.isObject()) {
        cout << "Json Parse error" << endl;
    }

    return parse_doucment.object()["data"].toObject();
}
