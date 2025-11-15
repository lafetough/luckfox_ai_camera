#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>


#include "memory_pool.h"
#include "frame_processor.h"
#include "rtsp_server.h"
#include "video_encoder.h"


class App {
public:

    App(uint16_t width, uint16_t height)
        :_width(width), _height(height),
        _mem_pool(width * height * 3, 1),
        _frame_processor(width, height),
        _venc(width, height)
    {}

    int main() {
        RK_S32 s32Ret = 0;

        int sX,sY,eX,eY;

        float fps = 0;
        RK_U64 nowUs;


        // Инициализация модуля захвата кадров OpenCV
        _frame_processor.initVideoCapture();

        // rkmpi init
        if (RK_MPI_SYS_Init() != RK_SUCCESS) {
            RK_LOGE("rk mpi sys init fail!");
            return -1;
        }
        //h264_frame
        VENC_STREAM_S stFrame;
        stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));

        // Создать буффер
        _mem_pool.init();

        // Создаем frame
        _frame_processor.initFrame(_mem_pool);


        int err = 0;
        err = _rtsp_server.init();
        err = _rtsp_server.createSession("/live/0");
        err = _rtsp_server.setVideoCodec(RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
        err = _rtsp_server.syncVideoTimestamp(rtsp_get_reltime(), rtsp_get_ntptime());

        if(err != 0) {
            goto app_free;
        }

        // venc init
        _venc.init(0, RK_VIDEO_ID_AVC);

        printf("init success\n");

        while(1)
        {
            if (!_frame_processor.captureFrame()) {
                printf("Cant capture frame, Breaking...\n");
                break;
            }

            _frame_processor.drawFpsText(fps);

            // send stream
            // encode H264
            _venc.sendFrame(_frame_processor.getFrameForVenc());

            // rtsp

            s32Ret = _venc.getStream(&stFrame);
            if(s32Ret == RK_SUCCESS)
            {
                    // printf("len = %d PTS = %d \n",stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS);
                void *pData = _mem_pool.getVirtualAddress(stFrame.pstPack->pMbBlk);

                _rtsp_server.sendVideoFrame(reinterpret_cast<uint8_t*>(pData), stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS);
                _rtsp_server.processEvents();

                fps = _frame_processor.getFps();
            }

            s32Ret = _venc.releaseStream(&stFrame);
            if (s32Ret != RK_SUCCESS) {
                RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
            }

        }

app_free:

        free(stFrame.pstPack);

        RK_MPI_SYS_Exit();

        return 0;
    }
private:
    MemoryPool _mem_pool;
    FrameProcessor _frame_processor;
    RtspServer _rtsp_server;
    VideoEncoder _venc;

    uint16_t _width;
    uint16_t _height;
};

int main(int argc, char *argv[]) {
    system("RkLunch-stop.sh");
    App app(720, 480);

    return app.main();
}
