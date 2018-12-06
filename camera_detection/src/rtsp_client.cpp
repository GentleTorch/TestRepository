#include "rtsp_client.h"
#include "detection_thread.h"
#include <thread>

using namespace std;

// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we
// still need to receive it. Define the size of the buffer that we'll use:

DummySink* DummySink::createNew(UsageEnvironment& env,
    MediaSubsession& subsession,
    char const* streamId)
{
    cout << "createNew()" << endl;
    return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env,
    MediaSubsession& subsession,
    char const* streamId)
    : MediaSink(env)
    , m_subsession(subsession)
{
    cout << "DummySink()" << endl;
    m_streamId = strDup(streamId);
    m_receiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

void DummySink::setFFmpeg(FFH264* ffmpeg)
{
    cout << "SetFFmpeg()" << endl;
    m_ffmpeg = ffmpeg;
}

void DummySink::setContainer(Container* con)
{
    content = con;
}

DummySink::~DummySink()
{
    cout << "~DummySink()" << endl;
    delete[] m_receiveBuffer;
    delete[] m_streamId;
}

// If you don't want to see debugging output for each received frame, then
// comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
SPropRecord* p_record = NULL;
void DummySink::afterGettingFrame(unsigned frameSize,
    unsigned numTruncatedBytes,
    struct timeval presentationTime,
    unsigned durationInMicroseconds)
{
    cout << "afterGettingFrame()" << endl;

    while (true) {
        if (NULL == p_record) {
            unsigned int Num = 0;
            unsigned int& SPropRecords = Num;
            p_record = parseSPropParameterSets(m_subsession.fmtp_spropparametersets(),
                SPropRecords);
            if (2 != Num) {
                p_record = NULL;
                break;
            }
        }
        SPropRecord& sps = p_record[0];
        SPropRecord& pps = p_record[1];
        // std::cout << sps.sPropBytes << "\t" << sps.sPropLength  << std::endl;
        cout << "frame size " << frameSize << endl;

        m_ffmpeg->decodeFrame(sps.sPropBytes,
            sps.sPropLength,
            pps.sPropBytes,
            pps.sPropLength,
            m_receiveBuffer,
            frameSize,
            presentationTime.tv_sec,
            presentationTime.tv_usec / 1000,
            content);
        break;
    }
    this->continuePlaying();
}

Boolean DummySink::continuePlaying()
{
    cout << "continuePlaying()" << endl;
    envir().taskScheduler().rescheduleDelayedTask(taskToken, 15 * 1000000,
        (TaskFunc*)restart,
        (void*)content);
    if (fSource == NULL)
        return False;
    fSource->getNextFrame(m_receiveBuffer,
        DUMMY_SINK_RECEIVE_BUFFER_SIZE,
        afterGettingFrame,
        this,
        onSourceClosure,
        this);
    return True;
}

static inline RtspClient* p_this(RTSPClient* rtspClient)
{
    return static_cast<RtspClient*>(rtspClient);
}
#define P_THIS p_this(rtspClient)

RtspClient::RtspClient(UsageEnvironment& env,
    char const* rtspURL,
    char const* sUser,
    char const* sPasswd,
    FFH264** ffmpeg,
    Container* con)
    : RTSPClient(env, rtspURL, 255, NULL, 0, -1)
    , m_Authenticator(sUser, sPasswd)
{
    cout << "RtspClient()" << endl;
    m_ffmpeg = *ffmpeg;
    m_subSessionIter = NULL;
    m_subSession = NULL;
    m_session = NULL;
    content = con;
}

void RtspClient::play()
{
    //  taskToken = envir().taskScheduler().scheduleDelayedTask(
    //      15 * 1000000, (TaskFunc*)restart, nullptr);
    //  id = envir().taskScheduler().createEventTrigger((TaskFunc*)restart);

    taskToken = envir().taskScheduler().scheduleDelayedTask(15 * 1000000, (TaskFunc*)restart, (void*)content);
    id = envir().taskScheduler().createEventTrigger((TaskFunc*)restart);

    cout << "Play()" << endl;
    this->sendNextCommand();
}

RtspClient::~RtspClient() {}

void RtspClient::shutdownStream()
{
    UsageEnvironment& env = this->envir();

    if (nullptr != this->m_subSession) {
        Boolean someSubsessionWereActive = False;
        MediaSubsession* subsession;

        while ((subsession = this->m_subSessionIter->next()) != nullptr) {
            if (subsession->sink != nullptr) {
                Medium::close(subsession->sink);
                subsession->sink = nullptr;
            }

            if (subsession->rtcpInstance() != nullptr) {
                subsession->rtcpInstance()->setByeHandler(nullptr, nullptr);
            }

            someSubsessionWereActive = True;
        }

        if (someSubsessionWereActive) {
            this->sendTeardownCommand(*(this->m_subSession), nullptr);
        }
    }

    // env<<*this<<"Closing the stream.\n";

    //    if(nullptr!=this)
    //    {
    //        Medium::close(this);
    //    }

    if (nullptr != this->m_session) {
        // delete this->m_session;
        this->m_session = NULL;
    }

    if (nullptr != this->m_subSession) {
        // delete this->m_subSession;
        this->m_subSession = NULL;
    }

    if (nullptr != this->m_subSessionIter) {
        delete this->m_subSessionIter;
        this->m_subSessionIter = NULL;
    }

    cout << "end of shutdown" << endl;
}

void RtspClient::continueAfterDescribe(int resultCode, char* resultString)
{
    cout << "continueAfterDescribe()" << endl;
    if (resultCode != 0) {
        std::cout << "Failed to DESCRIBE: " << resultString << std::endl;
        envir().taskScheduler().triggerEvent(id, (void*)content);

    } else {
        std::cout << "Got SDP: " << resultString << std::endl;
        m_session = MediaSession::createNew(envir(), resultString);
        m_subSessionIter = new MediaSubsessionIterator(*m_session);
        this->sendNextCommand();
    }
    delete[] resultString;
}
void RtspClient::continueAfterSetup(int resultCode, char* resultString)
{
    cout << "continueAfterSetup()" << endl;
    if (resultCode != 0) {
        std::cout << "Failed to SETUP: " << resultString << std::endl;
        envir().taskScheduler().triggerEvent(id, (void*)content);
    } else {
        // Live555CodecType codec = GetSessionCodecType(m_subSession->mediumName(),
        // m_subSession->codecName());
        m_subSession->sink = DummySink::createNew(envir(), *m_subSession, this->url());
        ((DummySink*)(m_subSession->sink))->setFFmpeg(m_ffmpeg);
        ((DummySink*)(m_subSession->sink))->setContainer(content);

        if (m_subSession->sink == NULL) {
            std::cout << "Failed to create a data sink for "
                      << m_subSession->mediumName() << "/"
                      << m_subSession->codecName()
                      << " subsession: " << envir().getResultMsg() << "\n";
        } else {
            std::cout << "Created a data sink for the \""
                      << m_subSession->mediumName() << "/"
                      << m_subSession->codecName() << "\" subsession";
            m_subSession->sink->startPlaying(
                *(m_subSession->readSource()), NULL, NULL);
        }
    }
    delete[] resultString;
    this->sendNextCommand();
}

void restart(void* clientData)
{
    cout << "Restart" << endl;
    Container* content = (Container*)clientData;
    content->rtspClient->shutdownStream();
    content->eventLoopWatchVariable = 1;
    content->bIsTerminate = true;
}

void RtspClient::continueAfterPlay(int resultCode, char* resultString)
{
    cout << "continueAfterPlay()" << endl;
    if (resultCode != 0) {
        std::cout << "Failed to PLAY: \n"
                  << resultString << std::endl;

        envir().taskScheduler().triggerEvent(id, (void*)content);
    } else {
        std::cout << "PLAY OK\n"
                  << std::endl;
    }

    envir().taskScheduler().unscheduleDelayedTask(taskToken);
    delete[] resultString;
}

void RtspClient::continueAfterSetup(RTSPClient* rtspClient,
    int resultCode,
    char* resultString)
{
    P_THIS->continueAfterSetup(resultCode, resultString);
}

void RtspClient::continueAfterPlay(RTSPClient* rtspClient,
    int resultCode,
    char* resultString)
{
    P_THIS->continueAfterPlay(resultCode, resultString);
}

void RtspClient::continueAfterDescribe(RTSPClient* rtspClient,
    int resultCode,
    char* resultString)
{
    P_THIS->continueAfterDescribe(resultCode, resultString);
}

void RtspClient::sendNextCommand()
{
    cout << "sendNextCommand()" << endl;
    if (m_subSessionIter == NULL) {
        // no SDP, send DESCRIBE
        cout << "send Describe" << endl;
        this->sendDescribeCommand(continueAfterDescribe, &m_Authenticator);
    } else {
        cout << "Iter is not null" << endl;
        m_subSession = m_subSessionIter->next();
        if (m_subSession != NULL) {
            // still subsession to SETUP
            if (!m_subSession->initiate()) {
                std::cout << "Failed to initiate " << m_subSession->mediumName() << "/"
                          << m_subSession->codecName()
                          << " subsession: " << envir().getResultMsg() << std::endl;
                this->sendNextCommand();
            } else {
                std::cout << "Initiated " << m_subSession->mediumName() << "/"
                          << m_subSession->codecName() << " subsession" << std::endl;
            }

            /* Change the multicast here */
            cout << "send Setup" << endl;
            this->sendSetupCommand(*m_subSession,
                continueAfterSetup,
                False,
                m_isTcp,
                False,
                &m_Authenticator);
        } else {
            // no more subsession to SETUP, send PLAY
            cout << "send Play" << endl;
            this->sendPlayCommand(*m_session,
                continueAfterPlay,
                (double)0,
                (double)-1,
                (float)0,
                &m_Authenticator);
        }
    }
}

void RtspThread::run()
{
    cout << "Run()" << endl;
    openCameraPlay();
}

void RtspThread::openCameraPlay()
{
    cout << "OpenCameraPlay()" << endl;
    ffmpegH264 = new FFH264();

    if (!ffmpegH264->initH264DecodeEnv()) {
        std::cout << "Error:----> FFmpeg AV_CODEC_ID_H264 Init Error" << std::endl;
        return;
    }

    m_scheduler = BasicTaskScheduler::createNew();
    m_env = BasicUsageEnvironment::createNew(*m_scheduler);
    m_rtspClient = new RtspClient(*m_env,
        m_url.c_str(),
        m_user.c_str(),
        m_passwd.c_str(),
        &ffmpegH264,
        content);

    content->rtspClient = m_rtspClient;

    m_rtspClient->play();

    m_env->taskScheduler().doEventLoop(&(content->eventLoopWatchVariable));
}

RtspThread::~RtspThread()
{
    if (nullptr != m_rtspClient) {
        Medium::close(m_rtspClient);
        m_rtspClient = nullptr;
    }

    if (nullptr != m_env) {
        m_env->reclaim();
        m_env = nullptr;
    }

    if (nullptr != m_scheduler) {
        delete m_scheduler;
        m_scheduler = nullptr;
    }

    if (nullptr != ffmpegH264) {
        delete ffmpegH264;
        ffmpegH264 = nullptr;
    }
}
