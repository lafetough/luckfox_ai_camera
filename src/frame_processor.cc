#include "frame_processor.h"
#include <cstdio>
#include "luckfox_mpi.h"

FrameProcessor::FrameProcessor(int width, int height)
    : width_(width), height_(height) {
}

FrameProcessor::~FrameProcessor() {
    releaseCapture();
}

int FrameProcessor::initVideoCapture() {
    printf("%s: %dx%d\n", __func__, width_, height_);

    capture_.set(cv::CAP_PROP_FRAME_WIDTH, width_);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, height_);

    if (!capture_.open(0)) {
        printf("ERROR: Failed to open video capture device\n");
        return -1;
    }

    printf("Video capture initialized successfully\n");
    return 0;
}

void FrameProcessor::updateTimeForFrame() {
    h264_frame.stVFrame.u32TimeRef = H264_TimeRef++;
    h264_frame.stVFrame.u64PTS = TEST_COMM_GetNowUs();
}

void FrameProcessor::initFrame(const MemoryPool& mem_pool) {

    MB_BLK src_Blk = mem_pool.getMemoryBlock();

    h264_frame.stVFrame.u32Width = width_;
    h264_frame.stVFrame.u32Height = height_;
    h264_frame.stVFrame.u32VirWidth = width_;
    h264_frame.stVFrame.u32VirHeight = height_;
    h264_frame.stVFrame.enPixelFormat =  RK_FMT_BGR888;
    h264_frame.stVFrame.u32FrameFlag = 160;
    h264_frame.stVFrame.pMbBlk = src_Blk;

    void *data = mem_pool.getVirtualAddress(src_Blk);

    frame = cv::Mat(
        cv::Size(width_, height_),
        CV_8UC3,
        data
    );
}

bool FrameProcessor::captureFrame() {
    if (!capture_.isOpened()) {
        printf("ERROR: Video capture not opened\n");
        return false;
    }
    updateTimeForFrame();

    capture_ >> frame;

    if (frame.empty()) {
        printf("ERROR: Failed to capture frame\n");
        return false;
    }

    return true;
}

float FrameProcessor::getFps() const {
    RK_U64 nowUs = TEST_COMM_GetNowUs();
    return  (float) 1000000 / (float)(nowUs - h264_frame.stVFrame.u64PTS);
}

void FrameProcessor::drawFpsText(float fps) {
    if (frame.empty()) {
        return;
    }

    char fpsText[16];
    snprintf(fpsText, sizeof(fpsText), "fps = %.2f", fps);

    cv::putText(frame, fpsText,
                cv::Point(40, 40),
                cv::FONT_HERSHEY_SIMPLEX, 1,
                cv::Scalar(0, 255, 0), 2);
}

const VIDEO_FRAME_INFO_S* FrameProcessor::getFrameForVenc() const {
    return &h264_frame;
}


void FrameProcessor::releaseCapture() {
    if (capture_.isOpened()) {
        capture_.release();
    }
}
