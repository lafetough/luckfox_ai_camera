#include "app.h"
#include "utilities.h"

App::App(uint16_t width, uint16_t height, uint16_t rtsp_port)
    :_width(width), _height(height), _rtsp_port(rtsp_port),
    _initialized(false)
{}

App::~App() {
    shutdown();
}

bool App::init() {
    printf("Initializing RTSP Video Streaming Application...\n");
    printf("Resolution: %dx%d, RTSP Port: %d\n", _width, _height, _rtsp_port);

    if (SystemUtils::executeSystemCommand("RkLunch-stop.sh") != 0) {
        printf("ERROR: cant stop default rtsp\n");
        return -1;
    }

    if (SystemUtils::initMpiSystem() != RK_SUCCESS) {
        printf("ERROR: Failed to initialize MPI system\n");
        return false;
    }

    if (!_initComponents()) {
        printf("ERROR: Failed to initialize components\n");
        _cleanupResources();
        return false;
    }

    _initialized = true;
    printf("Application initialized successfully\n");
    return true;
}


bool App::_initComponents() {

    _frame_processor = std::make_unique<FrameProcessor>(_width, _height);
    if (_frame_processor->initVideoCapture() != 0) {
        printf("ERROR: Video capture initialization failed\n");
        return false;
    }

    // 1. Mem init
    _mem_pool = std::make_unique<MemoryPool>(_width * _height * 3, 1);
    if (_mem_pool->init() != 0) {
        printf("ERROR: Memory pool initialization failed\n");
        return false;
    }

    // 3. Frame processor init

    _frame_processor->initFrame(*_mem_pool);

    // 4. RTSP init
    _rtsp_server = std::make_unique<RtspServer>(_rtsp_port);
    if (_rtsp_server->init() != 0) {
        printf("ERROR: RTSP server initialization failed\n");
        return false;
    }

    if (_rtsp_server->createSession("/live/0") != 0) {
        printf("ERROR: RTSP session creation failed\n");
        return false;
    }

    if (_rtsp_server->setVideoCodec(RTSP_CODEC_ID_VIDEO_H264, nullptr, 0) != 0) {
        printf("ERROR: Failed to set video codec\n");
        return false;
    }

    if (_rtsp_server->syncVideoTimestamp(rtsp_get_reltime(), rtsp_get_ntptime()) != 0) {
        printf("ERROR: Failed to sync video timestamp\n");
        return false;
    }

    // 2. VENC init
    _venc = std::make_unique<VideoEncoder>(_width, _height);
    if (_venc->init(0, RK_VIDEO_ID_AVC) != 0) {
        printf("ERROR: Video encoder initialization failed\n");
        return false;
    }


    printf("Succsessfull initialization\n");
    return true;
}

int App::run() {
    if (!_initialized) {
        printf("ERROR: Application not initialized\n");
        return -1;
    }

    uint64_t startTimeUs = TimerUtils::getCurrentTimeUs();
    uint64_t prevFrameTimeUs = startTimeUs;

    uint32_t frameCount = 0;
    float fps = 0.0f;

    VENC_STREAM_S stFrame;
    stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));


    RK_S32 s32Ret = 0;

    while(true) {
        uint64_t currentTimeUs = TimerUtils::getCurrentTimeUs();

        if (!_frame_processor->captureFrame()) {
            printf("Cant capture frame, Breaking...\n");
            return -1;
        }

        _frame_processor->drawFpsText(fps);

            // send stream
            // encode H264
        _venc->sendFrame(_frame_processor->getFrameForVenc());

        s32Ret = _venc->getStream(&stFrame);

        if(s32Ret == RK_SUCCESS)
        {
            // printf("len = %d PTS = %d \n",stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS);
            void *pData = _mem_pool->getVirtualAddress(stFrame.pstPack->pMbBlk);

            _rtsp_server->sendVideoFrame(
                reinterpret_cast<uint8_t*>(pData),
                stFrame.pstPack->u32Len,
                stFrame.pstPack->u64PTS);
            _rtsp_server->processEvents();

            fps = TimerUtils::calculateFps(prevFrameTimeUs, currentTimeUs);
            prevFrameTimeUs = currentTimeUs;
        }

        s32Ret = _venc->releaseStream(&stFrame);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
        }
    }

    return 0;
}

void App::shutdown() {
    if (!_initialized) {
        return;
    }

    printf("Shutting down application...\n");
    _cleanupResources();
    SystemUtils::exitMpiSystem();
    _initialized = false;
    printf("Application shut down complete\n");
}

void App::_cleanupResources() {
    if (_frame_processor) {
        _frame_processor->releaseCapture();
        _frame_processor.reset();
    }

    if (_rtsp_server) {
        _rtsp_server->shutdown();
        _rtsp_server.reset();
    }

    if (_venc) {
        _venc->shutdown();
        _venc.reset();
    }

    if (_mem_pool) {
        _mem_pool->destroy();
        _mem_pool.reset();
    }

}

























