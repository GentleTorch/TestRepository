/*
**定义ffmpeg H264解码类
**
** Authour: leaf
**/

#pragma once
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#include "detection_thread.h"

// 接收帧缓存区大小
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 500000

class FFH264 {
public:
    FFH264();

    ~FFH264();

    /*
    ** 初始化H264解码环境
    */
    bool initH264DecodeEnv();

    /*
    ** 设置播放流状态
    */
    void setPlayState(bool pause);

    /*
    ** 解码当前H264帧
    ** sPropBytes: sps帧内容
    ** sPropLength: sps帧长度
    ** ppsPropBytes: pps帧内容
    ** ppsPropLength: pps帧长度
    ** frameBuffer: H264帧内容
    ** frameLength: H264帧长度
    ** second: H264帧时间
    ** microSecond: H264帧时间
    ** content: 将解码后的帧放入缓存区
    */
    void decodeFrame(unsigned char* sPropBytes, int sPropLength,
        unsigned char* ppsPropBytes, int ppsPropLength,
        uint8_t* frameBuffer, int frameLength, long second,
        long microSecond, Container* content);

    /*
    ** 获取解码后的帧数据
    ** data: 数据存放位置
    */
    void getDecodedFrameData(unsigned char* data, int& length);

    //delete
    void getDecodedFrameInfo(int& width, int& heigth);

//modify
public:
    int frameWidth;
    int frameHeight;

private:
    AVCodec* m_codec;                   //解码器
    AVCodecContext* m_codecContext;     //解码上下文
    AVFrame* m_frame;                   //帧
    AVFrame* m_frameBGR;                //BGR格式帧

    uint8_t* m_outBuffer;                //解码后输出缓冲区
    AVFormatContext* m_outFmtCtx;       //输出格式上下文
    SwsContext* m_swsContext;           //
    AVPicture* m_pAVPicture;

    std::mutex m_playMutex;             //播放互斥量     
    unsigned char* m_outBufferRGB;       //输出RGB格式缓冲区
};
