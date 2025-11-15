#include "video_encoder.h"
#include <cstring>
#include <cstdio>

VideoEncoder::VideoEncoder(int width, int height, int bitrate)
    : width_(width), height_(height), bitrate_(bitrate),
      channelId_(-1), initialized_(false) {
}

VideoEncoder::~VideoEncoder() {
    shutdown();
}

int VideoEncoder::init(int channelId, RK_CODEC_ID_E codecType) {
    printf("%s: channel=%d, codec=%d\n", __func__, channelId, codecType);

    channelId_ = channelId;

    int ret = configureEncoder(codecType);
    if (ret != RK_SUCCESS) {
        printf("ERROR: Failed to configure encoder\n");
        return ret;
    }

    ret = startReceivingFrames();
    if (ret != RK_SUCCESS) {
        printf("ERROR: Failed to start receiving frames\n");
        return ret;
    }

    initialized_ = true;
    printf("Encoder initialized successfully\n");
    return RK_SUCCESS;
}

int VideoEncoder::configureEncoder(RK_CODEC_ID_E codecType) {
    VENC_CHN_ATTR_S stAttr;
    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));

    stAttr.stVencAttr.enType = codecType;
    stAttr.stVencAttr.enPixelFormat = RK_FMT_BGR888;
    stAttr.stVencAttr.u32Profile = H264E_PROFILE_MAIN;
    stAttr.stVencAttr.u32PicWidth = width_;
    stAttr.stVencAttr.u32PicHeight = height_;
    stAttr.stVencAttr.u32VirWidth = width_;
    stAttr.stVencAttr.u32VirHeight = height_;
    stAttr.stVencAttr.u32StreamBufCnt = 2;
    stAttr.stVencAttr.u32BufSize = width_ * height_ * 3 / 2;
    stAttr.stVencAttr.enMirror = MIRROR_NONE;

    stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    stAttr.stRcAttr.stH264Cbr.u32BitRate = bitrate_;
    stAttr.stRcAttr.stH264Cbr.u32Gop = 1;

    int ret = RK_MPI_VENC_CreateChn(channelId_, &stAttr);
    if (ret != RK_SUCCESS) {
        printf("RK_MPI_VENC_CreateChn failed: %x\n", ret);
        return ret;
    }

    return RK_SUCCESS;
}

int VideoEncoder::startReceivingFrames() {
    VENC_RECV_PIC_PARAM_S stRecvParam;
    memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));

    stRecvParam.s32RecvPicNum = -1;  // Continuous reception

    int ret = RK_MPI_VENC_StartRecvFrame(channelId_, &stRecvParam);
    if (ret != RK_SUCCESS) {
        printf("RK_MPI_VENC_StartRecvFrame failed: %x\n", ret);
        return ret;
    }

    return RK_SUCCESS;
}

int VideoEncoder::sendFrame(const VIDEO_FRAME_INFO_S *frame) const {
    if (!initialized_) {
        printf("ERROR: Encoder not initialized\n");
        return -1;
    }

    if (!frame) {
        printf("ERROR: Frame pointer is null\n");
        return -1;
    }

    int ret = RK_MPI_VENC_SendFrame(channelId_, frame, -1);
    if (ret != RK_SUCCESS) {
        printf("RK_MPI_VENC_SendFrame failed: %x\n", ret);
        return ret;
    }

    return RK_SUCCESS;
}

int VideoEncoder::getStream(VENC_STREAM_S *stream) {
    if (!initialized_) {
        printf("ERROR: Encoder not initialized\n");
        return -1;
    }

    if (!stream) {
        printf("ERROR: Stream pointer is null\n");
        return -1;
    }

    int ret = RK_MPI_VENC_GetStream(channelId_, stream, -1);
    return ret;
}

int VideoEncoder::releaseStream(VENC_STREAM_S *stream) {
    if (!initialized_) {
        return -1;
    }

    if (!stream) {
        return -1;
    }

    int ret = RK_MPI_VENC_ReleaseStream(channelId_, stream);
    if (ret != RK_SUCCESS) {
        printf("RK_MPI_VENC_ReleaseStream failed: %x\n", ret);
        return ret;
    }

    return RK_SUCCESS;
}

void VideoEncoder::shutdown() {
    if (!initialized_) {
        return;
    }

    RK_MPI_VENC_StopRecvFrame(channelId_);
    RK_MPI_VENC_DestroyChn(channelId_);
    initialized_ = false;
}
