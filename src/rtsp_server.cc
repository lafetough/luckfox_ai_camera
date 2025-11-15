#include "rtsp_server.h"
#include <cstdio>

RtspServer::RtspServer(int port)
    : port_(port), handle_(nullptr), session_(nullptr), initialized_(false) {
}

RtspServer::~RtspServer() {
    shutdown();
}

int RtspServer::init() {
    printf("%s: port=%d\n", __func__, port_);
    
    handle_ = create_rtsp_demo(port_);
    if (!handle_) {
        printf("ERROR: Failed to create RTSP demo\n");
        return -1;
    }
    
    initialized_ = true;
    printf("RTSP server initialized successfully\n");
    return 0;
}

int RtspServer::createSession(const char *path) {
    if (!initialized_) {
        printf("ERROR: RTSP server not initialized\n");
        return -1;
    }
    
    if (!path) {
        printf("ERROR: Path is null\n");
        return -1;
    }
    
    session_ = rtsp_new_session(handle_, path);
    if (!session_) {
        printf("ERROR: Failed to create RTSP session\n");
        return -1;
    }
    
    printf("RTSP session created: %s\n", path);
    return 0;
}

int RtspServer::setVideoCodec(int codecId, const uint8_t *codecData, int dataLen) {
    if (!session_) {
        printf("ERROR: RTSP session not created\n");
        return -1;
    }
    
    int ret = rtsp_set_video(session_, codecId, codecData, dataLen);
    if (ret != 0) {
        printf("ERROR: Failed to set video codec\n");
        return ret;
    }
    
    printf("Video codec set successfully: codec=%d\n", codecId);
    return 0;
}

int RtspServer::sendVideoFrame(const uint8_t *frame, int len, uint64_t ts) {
    if (!session_) {
        printf("ERROR: RTSP session not created\n");
        return -1;
    }
    
    if (!frame || len <= 0) {
        printf("ERROR: Invalid frame data\n");
        return -1;
    }
    
    int ret = rtsp_tx_video(session_, frame, len, ts);
    return ret;
}

int RtspServer::processEvents() {
    if (!handle_) {
        return -1;
    }
    
    rtsp_do_event(handle_);
    return 0;
}

int RtspServer::syncVideoTimestamp(uint64_t ts, uint64_t ntpTime) {
    if (!session_) {
        printf("ERROR: RTSP session not created\n");
        return -1;
    }
    
    int ret = rtsp_sync_video_ts(session_, ts, ntpTime);
    if (ret != 0) {
        printf("ERROR: Failed to sync video timestamp\n");
        return ret;
    }
    
    return 0;
}

void RtspServer::shutdown() {
    if (!initialized_) {
        return;
    }
    
    if (session_) {
        rtsp_del_session(session_);
        session_ = nullptr;
    }
    
    if (handle_) {
        rtsp_del_demo(handle_);
        handle_ = nullptr;
    }
    
    initialized_ = false;
}
