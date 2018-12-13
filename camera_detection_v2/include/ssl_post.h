#ifndef SSL_POST_H
#define SSL_POST_H

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QTimer>
#include <fstream>
#include <iostream>

using namespace std;

string sslPost(string url, string strData);

QJsonObject sslGetConfig(string url);


#endif // SSL_POST_H
