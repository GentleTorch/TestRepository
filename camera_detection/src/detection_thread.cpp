#include "detection_thread.h"
#include "ObjectDetect.h"
#include "rtsp_client.h"
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
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;

/*
** 计算两帧相似度，采用平均哈希算法计算指纹
** matSrc1: 图片矩阵
** matSrc2: 图片矩阵
** 返回值：
**       double 指纹不同的位数，<5意味着相似，>10意味着不同的帧
*/
double getSimilarity(const Mat& matSrc1, const Mat& matSrc2)
{
    Mat matDst1, matDst2;
    cv::resize(matSrc1, matDst1, cv::Size(8, 8), 0, 0, cv::INTER_CUBIC);
    cv::resize(matSrc2, matDst2, cv::Size(8, 8), 0, 0, cv::INTER_CUBIC);

    cv::cvtColor(matDst1, matDst1, CV_BGR2GRAY);
    cv::cvtColor(matDst2, matDst2, CV_BGR2GRAY);

    int iAvg1 = 0, iAvg2 = 0;
    int arr1[64], arr2[64];

    for (int i = 0; i < 8; i++) {
        uchar* data1 = matDst1.ptr<uchar>(i);
        uchar* data2 = matDst2.ptr<uchar>(i);

        int tmp = i * 8;

        for (int j = 0; j < 8; j++) {
            int tmp1 = tmp + j;

            arr1[tmp1] = data1[j] / 4 * 4;
            arr2[tmp1] = data2[j] / 4 * 4;

            iAvg1 += arr1[tmp1];
            iAvg2 += arr2[tmp1];
        }
    }

    iAvg1 /= 64;
    iAvg2 /= 64;

    for (int i = 0; i < 64; i++) {
        arr1[i] = (arr1[i] >= iAvg1) ? 1 : 0;
        arr2[i] = (arr2[i] >= iAvg2) ? 1 : 0;
    }

    int iDiffNum = 0;

    for (int i = 0; i < 64; i++)
        if (arr1[i] != arr2[i])
            ++iDiffNum;

    cout << "Similarity : " << iDiffNum << endl;

    return iDiffNum;
}

void sslPost(string url, string strData)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest request;
    QByteArray line;
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
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    eventLoop.exec();
    line = reply->readAll();

    qDebug() << "response:" << line;

    reply->deleteLater();
    delete manager;
}

void detectionThread(Container* content)
{
    // 初始化
    cv::Mat m_frameMat;
    if (m_frameMat.empty()) {
        m_frameMat.create(cv::Size(600, 480), CV_8UC3);
        memset(m_frameMat.data, 0, 600 * 480 * 3);
    }

    // 初始化前一帧
    cv::Mat prevFrame;
    if (prevFrame.empty()) {
        prevFrame.create(cv::Size(600, 480), CV_8UC3);
        memset(prevFrame.data, 0, 600 * 480 * 3);
    }

    // 存储检测结果
    QJsonObject* result = nullptr;

    // 运行检测的对象
    ObjectDetect* object = new ObjectDetect();

    // 获取初始化状态
    if (!(object->getInitStatus())) {
        cout << "Init ObjectDetect Error" << endl;
        content->bIsTerminate = true;
    }

    double threshold = 5; // 相似度阀值

    while (!content->bIsTerminate) {
        // 帧缓冲区空，等待
        unique_lock<mutex> lock(content->mtx);
        if (content->que.empty()) {
            lock.unlock();
            cout << "##################sleep for a while###############" << endl;
            chrono::milliseconds dura(100);
            this_thread::sleep_for(dura);
            continue;
        }

        // 从缓冲区取出一帧
        auto data = content->que.front();
        content->que.pop();
        lock.unlock();

        //将数据拷贝到Mat中
        memcpy(m_frameMat.data, data->buffer, BUF_LEN);
        double similarity = getSimilarity(m_frameMat, prevFrame);
        if (similarity < threshold) {
            memcpy(prevFrame.data, m_frameMat.data, 600 * 480 * 3);
            continue;
        }

        // 执行检测
        int ret = object->runDetect(m_frameMat);
        if (-1 == ret) {
            content->bIsTerminate = true;
        } else if (1 == ret) {

            //　检测到物体，获取检测结果
            cout << "Send to server" << endl;
            result = object->getJsonResult();

            // 发送结果到服务器
            std::string strData = QJsonDocument(*result).toJson(QJsonDocument::Compact).toStdString();

            sslPost("https://192.168.100.88:8089/pdio_collect/analysisData/upload", strData);
            cout << "Sending ends" << endl;
        }
        memcpy(prevFrame.data, m_frameMat.data, 600 * 480 * 3);
    }

    // 回收资源
    delete object;
    restart(content);
}
