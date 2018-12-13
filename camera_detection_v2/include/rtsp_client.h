/*
** 定义源帧数据类
** 定义rtsp客户端
** 定义rtsp线程类
** 
** Authour: leaf
*/

#pragma once

#include <iostream>
#include <string.h>
#include <vector>

#include "BasicUsageEnvironment.hh"
#include "DigestAuthentication.hh"
#include "H264VideoRTPSource.hh"
#include "liveMedia.hh"

#include "detection_thread.h"
#include "ffmpeg_h264.h"

using namespace std;

class DummySink : public MediaSink {
public:

    /*
    ** 创建DummySink实例
    ** env: 当前使用环境
    ** subsession: 媒体子会话
    ** streamId: 流ID
    ** 返回：
    **      DummySink* 对象实例
    */
    static DummySink* createNew(UsageEnvironment& env,
        MediaSubsession& subsession,
        char const* streamId = NULL);

    /*
    ** 设置ffmpeg解码类
    ** ffmpeg: FFH264类
    ** 返回：
    **      空
    */
    void setFFmpeg(FFH264* ffmpeg);

    /*
    ** 设置标志结构体
    ** con: 标志结构体
    ** 返回：
    **      空
    */
    void setContainer(Container* con);

private:

    /*
    ** 构造函数
    */
    DummySink(UsageEnvironment& env, MediaSubsession& subsession,
        char const* streamId);

    virtual ~DummySink();

    /*
    ** 处理帧数据回调函数
    ** clientData: 哪个源消费者
    ** frameSize: 帧大小
    ** numTruncatedBytes: 
    ** presentationTime: 展示时间
    ** durationInMicroseconds: 间隔时间
    ** 返回：
    **      空
    */
    static void afterGettingFrame(void* clientData, unsigned frameSize,
        unsigned numTruncatedBytes,
        struct timeval presentationTime,
        unsigned durationInMicroseconds)
    {
        static_cast<DummySink*>(clientData)
            ->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime,
                durationInMicroseconds);
    }

    /*
    ** 当前sink　处理帧数据
    ** frameSize: 帧大小
    ** numTruncatedBytes: 
    ** presentationTime: 展示时间
    ** durationInMicroseconds: 间隔时间
    ** 返回：
    **      空
    */
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
        struct timeval presentationTime,
        unsigned durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

//modify
private:
    u_int8_t* m_receiveBuffer;               // 接收数据的buffer
    MediaSubsession& m_subsession;           // 媒体子会话
    char* m_streamId;                        // 流ID
    FFH264* m_ffmpeg;                       // H264解码类

    Container* content;                     // 标志结构体
    TaskToken taskToken;                    //　延时任务id
};

class RtspClient : public RTSPClient {
public:

    /*
    ** 初始化RtspClient
    ** env: 使用环境
    ** rtspURL: 摄像头URL
    ** sUser: 用户名
    ** sPasswd: 密码
    ** ffmpeg:　解码类
    ** con: 标志结构体
    */
    RtspClient(UsageEnvironment& env, char const* rtspURL, char const* sUser,
        char const* sPasswd, FFH264** ffmpeg, Container* con);

    /*
    ** 发送命令
    */
    void play();

    /*
    ** 发送setup命令后回调函数
    ** resultCode: 结果代码，是否出错
    ** resultString: 附加描述信息
    ** 返回：
    **      空
    */
    void continueAfterSetup(int resultCode, char* resultString);

    /*
    ** 发送play命令后回调函数
    ** resultCode: 结果代码，是否出错
    ** resultString: 附加描述信息
    ** 返回：
    **      空
    */
    void continueAfterPlay(int resultCode, char* resultString);

    /*
    ** 发送describe命令后回调
    ** resultCode: 结果代码，是否出错
    ** resultString: 附加描述信息
    ** 返回：
    **      空
    */
    void continueAfterDescribe(int resultCode, char* resultString);

    /*
    ** 发送一条指令
    */
    void sendNextCommand();

    /*
    ** 设置TCP传输流
    ** istcp: 是否TCP
    */
    inline void setStreamTCP(bool istcp) { m_isTcp = istcp; }

    virtual ~RtspClient();

    /*
    ** 关闭流
    */
    void shutdownStream();

protected:

    /*
    ** 发送setup命令后回调函数
    ** rtspClient: rtsp客户端
    ** resultCode: 结果代码，是否出错
    ** resultString: 附加描述信息
    ** 返回：
    **      空
    */
    static void continueAfterSetup(RTSPClient* rtspClient, int resultCode,
        char* resultString);

    /*
    ** 发送play命令后回调函数
    ** rtspClient: rtsp客户端
    ** resultCode: 结果代码，是否出错
    ** resultString: 附加描述信息
    ** 返回：
    **      空
    */
    static void continueAfterPlay(RTSPClient* rtspClient, int resultCode,
        char* resultString);

    /*
    ** 发送describe命令后回调函数
    ** rtspClient: rtsp客户端
    ** resultCode: 结果代码，是否出错
    ** resultString: 附加描述信息
    ** 返回：
    **      空
    */
    static void continueAfterDescribe(RTSPClient* rtspClient, int resultCode,
        char* resultString);

private:
    FFH264* m_ffmpeg;                                   // 解码类
    Authenticator m_Authenticator;                      // 验证
    bool m_isTcp = true;                                // 设置TCP
    MediaSession* m_session;                            // 当前会话
    MediaSubsession* m_subSession;                      // 当前子会话
    MediaSubsessionIterator* m_subSessionIter;          // 子会话迭代器

    Container* content;                                 // 标志结构体
    TaskToken taskToken;                                // 延时任务id
    EventTriggerId id;                                  // 触发事件id
};

class RtspThread {
public:

    /*
    ** 初始化
    ** pUrl: URL
    ** pUser:　用户名
    ** pPasswd: 密码
    ** con: 标志结构体
    */
    RtspThread(std::string pUrl, std::string pUser, std::string pPasswd,
        Container* con)
    {
        cout << "RtspThread()" << endl;
        m_url = pUrl;
        m_user = pUser;
        m_passwd = pPasswd;
        m_scheduler = NULL;
        m_env = NULL;
        content = con;
    }
    ~RtspThread();

    /*
    ** 执行入口
    */
    void run();

    /*
    ** 打开摄像头发布命令播放
    */
    void openCameraPlay();

    FFH264* ffmpegH264;                     // 解码类

    RtspClient* m_rtspClient;               // rtsp客户端

private:
    TaskScheduler* m_scheduler;             // 调度器
    UsageEnvironment* m_env;                //　使用环境

    std::string m_url;                      //　URL
    std::string m_user;                     //　用户
    std::string m_passwd;                   // 密码

    Container* content;                     // 标志性结构体
};

void restart(void* clientData);
