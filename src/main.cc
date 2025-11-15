#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "rtsp_demo.h"
#include "luckfox_mpi.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "memory_pool.h"
#include "frame_processor.h"


class App {
public:

    App(uint16_t width, uint16_t height)
        :_width(width), _height(height),
        _mem_pool(width * height * 3, 1),
        _frame_processor(width, height)
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
        RK_U64 H264_PTS = 0;
        RK_U32 H264_TimeRef = 0;
        VIDEO_FRAME_INFO_S stVpssFrame;

        // Создать буффер
        _mem_pool.init();

        // Создаем frame
        _frame_processor.initFrame(_mem_pool);

        // rtsp init
        rtsp_demo_handle g_rtsplive = NULL;
        rtsp_session_handle g_rtsp_session;
        g_rtsplive = create_rtsp_demo(554);
        g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");
        rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
        rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());

        // venc init
        RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;
        venc_init(0, _width, _height, enCodecType);

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
            RK_MPI_VENC_SendFrame(0, _frame_processor.getFrameForVenc(), -1);

            // rtsp
            s32Ret = RK_MPI_VENC_GetStream(0, &stFrame, -1);
            if(s32Ret == RK_SUCCESS)
            {
                if(g_rtsplive && g_rtsp_session)
                {
                    // printf("len = %d PTS = %d \n",stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS);
                    void *pData = _mem_pool.getVirtualAddress(stFrame.pstPack->pMbBlk);
                    rtsp_tx_video(g_rtsp_session, (uint8_t *)pData, stFrame.pstPack->u32Len,
                                  stFrame.pstPack->u64PTS);
                    rtsp_do_event(g_rtsplive);

                }

                fps = _frame_processor.getFps();
            }

            s32Ret = RK_MPI_VENC_ReleaseStream(0, &stFrame);
            if (s32Ret != RK_SUCCESS) {
                RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
            }

        }

        RK_MPI_VENC_StopRecvFrame(0);
        RK_MPI_VENC_DestroyChn(0);

        free(stFrame.pstPack);

        if (g_rtsplive)
            rtsp_del_demo(g_rtsplive);

        RK_MPI_SYS_Exit();

        return 0;
    }
private:
    MemoryPool _mem_pool;
    FrameProcessor _frame_processor;

    uint16_t _width;
    uint16_t _height;
};

int main(int argc, char *argv[]) {
    system("RkLunch-stop.sh");
    App app(720, 480);

    return app.main();
}
