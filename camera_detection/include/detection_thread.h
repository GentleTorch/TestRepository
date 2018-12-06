/*
** 定义线程控制结构体
** 定义线程入口函数
**
** Authour: leaf
**/

#ifndef _DETECTION_THREAD_H
#define _DETECTION_THREAD_H


#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class RtspClient;

using namespace std;

const int BUF_LEN = 600 * 480 * 3;

/*
** 缓存区
**/
struct FrameData {
    uint8_t buffer[BUF_LEN];
};

/*
**一些标志位用于控制，线程是否退出
** que:帧数据缓存区
** mtx: 用于互斥访问缓存区
** bIsTerminate: ture检测线程退出，false检测线程运行
** eventLoopWatchVariable: 0进入live555事件循环，不等于０退出live555事件循环
** rtspClient: 用于退出live555事件循环调用shutdownStream关闭
**/
struct Container {
    queue<shared_ptr<FrameData>> que;
    mutex mtx;
    bool bIsTerminate = false;
    char eventLoopWatchVariable = 0;
    RtspClient* rtspClient = NULL;
};

/*
** 检测线程入口
** content: 用于控制线程运行
**/
void detectionThread(Container* content);

#endif
