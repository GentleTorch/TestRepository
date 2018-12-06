#include "detection_thread.h"
#include "ffmpeg_h264.h"
#include "rtsp_client.h"
#include <QCoreApplication>
#include <opencv2/opencv.hpp>
#include <thread>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);



    while (true) {
        Container content;
        thread objectDetect(detectionThread, &content);

        RtspThread clientThread(
            "rtsp://admin:Pdio#179530!!@192.168.100.64:554/h264/ch1/main/av_stream",
            "", "", &content); // admin:pdio#123456@admin:Pdio#179530!!

        thread client(&RtspThread::run, &clientThread);

        client.join();
        objectDetect.join();

        chrono::milliseconds dura(1000);
        this_thread::sleep_for(dura);
    }

    exit(0);
    return a.exec();
}
