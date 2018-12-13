#include "ffmpeg_h264.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <unistd.h>

#include "detection_thread.h"

using namespace std;

FFH264::FFH264()
{
    // cout<<"FFH264()"<<endl;
    m_swsContext = nullptr;
    m_codecContext = nullptr;
    av_register_all();
    m_frame = av_frame_alloc();
    m_frameBGR = av_frame_alloc();
}
FFH264::~FFH264()
{
    // cout<<"~FFH264()"<<endl;
    av_frame_free(&m_frame);
    av_frame_free(&m_frameBGR);
}
bool FFH264::initH264DecodeEnv()
{
    // cout<<"initH264DecodeEnv()"<<endl;
    do {
        m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!m_codec) {
            break;
        }
        m_codecContext = avcodec_alloc_context3(m_codec);
        if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0) {
            break;
        }
        return true;
    } while (0);
    return false;
}

void FFH264::setPlayState(bool pause)
{
    // cout<<"setPlayState()"<<endl;
    if (pause) {
        m_playMutex.lock();
    } else {
        m_playMutex.unlock();
    }
}

void FFH264::decodeFrame(unsigned char* sPropBytes, int sPropLength,
    unsigned char* ppsPropBytes, int ppsPropLength,
    uint8_t* frameBuffer, int frameLength, long second,
    long microSecond, Container* content)
{
    // cout<<"DecodeFrame()" << sPropLength << ppsPropLength <<endl;
    if (frameLength <= 0)
        return;

    //
    unsigned char nalu_header[4] = { 0x00, 0x00, 0x00, 0x01 };
    int totalSize = 4 + sPropLength + 4 + ppsPropLength + 4 + frameLength;
    unsigned char* tmp = new unsigned char[totalSize];
    int idx = 0;
    memcpy(tmp + idx, nalu_header, 4);
    idx += 4;
    memcpy(tmp + idx, sPropBytes, sPropLength);
    idx += sPropLength;
    memcpy(tmp + idx, nalu_header, 4);
    idx += 4;
    memcpy(tmp + idx, ppsPropBytes, ppsPropLength);
    idx += ppsPropLength;
    memcpy(tmp + idx, nalu_header, 4);
    idx += 4;
    memcpy(tmp + idx, frameBuffer, frameLength);
    int frameFinished = 0;
    AVPacket framePacket;
    av_init_packet(&framePacket);
    framePacket.size = totalSize;
    framePacket.data = tmp;
    // framePacket.size=frameLength;
    // framePacket.data=frameBuffer;

    int ret = avcodec_decode_video2(m_codecContext, m_frame, &frameFinished,
        &framePacket);
    // if(ret < 0) {
    // std::cout << "Decodec Error!" << std::endl;
    //}
    if (frameFinished) {
        // std::cout << "Decodec succ!" << std::endl;

        m_playMutex.lock();
        frameWidth = m_frame->width;
        frameHeight = m_frame->height;
        if (m_swsContext == nullptr) {
            m_swsContext = sws_getCachedContext(
                m_swsContext, frameWidth, frameHeight,
                AVPixelFormat::AV_PIX_FMT_YUV420P, frameWidth, frameHeight,
                AVPixelFormat::AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
            int size = avpicture_get_size(AV_PIX_FMT_BGR24, m_codecContext->width,
                m_codecContext->height);
            m_outBuffer = (uint8_t*)av_malloc(size);
            avpicture_fill((AVPicture*)m_frameBGR, m_outBuffer, AV_PIX_FMT_BGR24,
                m_codecContext->width, m_codecContext->height);
        }
        sws_scale(m_swsContext, (const uint8_t* const*)m_frame->data,
            m_frame->linesize, 0, frameHeight, m_frameBGR->data,
            m_frameBGR->linesize);
        m_playMutex.unlock();

        cv::Mat m_frameMat, imgDst;
        if (frameWidth > 0 && frameHeight > 0) {
            if (m_frameMat.empty()) {
                m_frameMat.create(cv::Size(frameWidth, frameHeight), CV_8UC3);
                imgDst.create(cv::Size(600, 480), CV_8UC3);
            }
            int length;

            getDecodedFrameData(m_frameMat.data, length);
            //      for (int i = 0; i < length; i += 3) {
            //        uchar temp = m_frameMat.data[i];
            //        m_frameMat.data[i] = m_frameMat.data[i + 2];
            //        m_frameMat.data[i + 2] = temp;
            //      }
            // cv::imshow("Camera", m_frameMat);
            // cv::waitKey(1);

            cv::resize(m_frameMat, imgDst, imgDst.size());
            // cameraDetection(imgDst);

            // cout<<"We have a smaller picture"<<endl;

            // 添加帧数据到缓存区
            shared_ptr<FrameData> data(new FrameData);
            length = 600 * 480 * 3;
            memcpy(data->buffer, imgDst.data, length);

            unique_lock<mutex> lock(content->mtx);
            while (content->que.size() >= 10)
                content->que.pop();
            content->que.push(data);
            lock.unlock();
        }
    }
    av_free_packet(&framePacket);
    delete[] tmp;
    tmp = nullptr; //防止产生悬垂指针使程序产生没必要的错误
}

void FFH264::getDecodedFrameData(unsigned char* data, int& length)
{
    // cout<<"GetDecodeFrameData()"<<endl;
    
    m_playMutex.lock();
    length = frameWidth * frameHeight * 3;
    memcpy(data, m_outBuffer, length);
    m_playMutex.unlock();
}

void FFH264::getDecodedFrameInfo(int& width, int& heigth)
{
    // cout<<"GetDecodedFrameInfo()"<<endl;
    width = frameWidth;
    heigth = frameHeight;
    cout << "Width:" << width << endl;
    cout << "Height:" << heigth << endl;
}
