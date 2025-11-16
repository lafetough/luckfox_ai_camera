#ifndef APP_H
#define APP_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>

#include "memory_pool.h"
#include "frame_processor.h"
#include "rtsp_server.h"
#include "video_encoder.h"
#include "rknn_interface.h"

class App
{
public:
    App(uint16_t width, uint16_t height, uint16_t rtsp_port);
    ~App();

    bool init();

    int run();

    void shutdown();

private:
    bool _initComponents();
    void _cleanupResources();

private:
    std::unique_ptr<MemoryPool> _mem_pool;
    std::unique_ptr<FrameProcessor> _frame_processor;
    std::unique_ptr<RtspServer> _rtsp_server;
    std::unique_ptr<VideoEncoder> _venc;
    std::unique_ptr<RKNNInference> _inferance;

    uint16_t _width;
    uint16_t _height;
    uint16_t _rtsp_port;
    bool _initialized;

    int _model_width;      // ← Добавить
    int _model_height;     // ← Добавить
    int _model_channel;    // ← Добавит
};

#endif // APP_H
