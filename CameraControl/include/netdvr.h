#ifndef NETDVR_H
#define NETDVR_H

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <QCoreApplication>
#include <QDebug>
#include <QtCore>

#include "HCNetSDK.h"

class Netdvr {
public:
    Netdvr(std::string cameraIP, int port, std::string userName, std::string passWord);
    ~Netdvr();

    void runCruise();
    void stopCruise();

private:
    std::string logPath{ "./HCLog" };
    NET_DVR_DEVICEINFO_V30 deviceInfo;
    LONG userID;
};

#endif // NETDVR_H
