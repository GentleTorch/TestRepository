#include "detection_thread.h"
#include "ffmpeg_h264.h"
#include "rtsp_client.h"
#include "ssl_post.h"
#include <QCoreApplication>
#include <opencv2/opencv.hpp>
#include <thread>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    string fileName{ "configurl.txt" };
    string configurl;
    ifstream in;
    in.open(fileName);

    if (!in.is_open()) {
        cout << "file open error" << endl;
    }

    getline(in, configurl);
    cout << configurl << endl;
    in.close();

    QJsonObject obj;
    while (true) {
        obj = sslGetConfig(configurl);
        if (!obj.empty())
            break;
        chrono::milliseconds dura(1000);
        this_thread::sleep_for(dura);
    }

    while (true) {
        Container content;
        thread objectDetect(detectionThread, &content, obj);

        RtspThread clientThread(
            obj["camera_rtsp_url"].toString().toStdString(),
            "", "", &content);

        thread client(&RtspThread::run, &clientThread);

        client.join();
        objectDetect.join();

        chrono::milliseconds dura(1000);
        this_thread::sleep_for(dura);
    }

    exit(0);
    return a.exec();
}
