#include "HCNetSDK.h"
#include <QCoreApplication>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    char cUserChoose = 'r';

    NET_DVR_Init();
    NET_DVR_SetLogToFile(3, "./sdkLog");

    //Login device
    NET_DVR_DEVICEINFO_V30 struDeviceInfo{ 0 };
    //    LONG lUserID = NET_DVR_Login_V30("26.47.132.104", 8000, "admin", "ftzn123!", &struDeviceInfo);

    LONG lUserID = NET_DVR_Login_V30("192.168.100.64", 8000, "admin", "Pdio#179530!!", &struDeviceInfo);
    printf("--------------%d\n", lUserID);
    if (lUserID < 0) {
        printf("pyd---Login error, %d\n", NET_DVR_GetLastError());

        NET_DVR_Cleanup();
        return -1;
    }

    while ('q' != cUserChoose) {
        printf("\n");
        printf("Input 1, left\n");
        printf("      2, right\n");
        printf("      3, up\n");
        printf("      4, down\n");
        printf("      5, cruise\n ");
        printf("      q, Quit.\n");
        printf("Input:");

        int ret = 0;

        cin >> cUserChoose;
        switch (cUserChoose) {
        case '1':
            printf("---------------------------begin-----------------------\n");
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, PAN_LEFT, 0, 1);
            printf("control ==== %d ---- %d\n", ret, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, PAN_RIGHT, 1, 1);
            printf("control ==== %d\n", ret);
            break;
        case '2':
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, PAN_RIGHT, 0, 1);
            printf("control ==== %d\n", ret);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, PAN_RIGHT, 1, 1);
            printf("control ==== %d\n", ret);
            break;
        case '3':
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, TILT_UP, 0, 1);
            printf("control ==== %d\n", ret);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, TILT_UP, 1, 1);
            printf("control ==== %d\n", ret);
            break;
        case '4':
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, TILT_DOWN, 0, 1);
            printf("control ==== %d\n", ret);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            ret = NET_DVR_PTZControlWithSpeed_Other(lUserID, 1, TILT_DOWN, 1, 1);
            printf("control ==== %d\n", ret);
            break;
        case '5':
            ret = NET_DVR_PTZCruise_Other(lUserID, 1, RUN_SEQ, 1, 40, 5);
            printf("control ==== %d\n", ret);
            break;

        case '6':
            ret = NET_DVR_PTZCruise_Other(lUserID, 1, STOP_SEQ, 1, 40, 5);

            printf("control ==== %d\n", ret);
            break;
        default:
            break;
        }
    }
    NET_DVR_Logout_V30(lUserID);
    NET_DVR_Cleanup();

    return a.exec();
}
