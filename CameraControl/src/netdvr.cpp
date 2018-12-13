#include "netdvr.h"

Netdvr::Netdvr(std::string cameraIP, int port, std::string userName, std::string passWord)
{
    NET_DVR_Init();
    NET_DVR_SetLogToFile(3, const_cast<char*>(logPath.c_str()));
    memset(&deviceInfo, 0, sizeof(NET_DVR_DEVICEINFO_V30));

    userID = NET_DVR_Login_V30(
        const_cast<char*>(cameraIP.c_str()),
        port,
        const_cast<char*>(userName.c_str()),
        const_cast<char*>(passWord.c_str()),
        &deviceInfo);

    qDebug() << userID;
    if (userID < 0) {
        qDebug() << "---login error: " << NET_DVR_GetLastError();
    }
}

Netdvr::~Netdvr()
{
    NET_DVR_Logout_V30(userID);
    NET_DVR_Cleanup();
}

void Netdvr::runCruise()
{
    if (userID < 0) {
        qDebug() << "userID < 0";
        return;
    }
    int ret = NET_DVR_PTZCruise_Other(userID, 1, RUN_SEQ, 1, 40, 20);
    qDebug() << "control========" << ret;
}

void Netdvr::stopCruise()
{
    if (userID < 0) {
        qDebug() << "userID < 0";
        return;
    }
    int ret = NET_DVR_PTZCruise_Other(userID, 1, STOP_SEQ, 1, 40, 20);
    qDebug() << "control========" << ret;
}
