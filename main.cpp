#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

extern "C" {
#include "rk_mpi_sys.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_mb.h"
#include "rk_common.h"
#include "rk_comm_video.h"
#include "rtsp_demo.h"
}

// Глобальные переменные
static bool g_running = true;
static rtsp_demo_handle g_rtsplive = NULL;
static rtsp_session_handle g_rtsp_session = NULL;

// Параметры видео
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define VIDEO_FPS 30

void signal_handler(int sig) {
    fprintf(stderr, "Signal %d received, exiting...\n", sig);
    g_running = false;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("=== Luckfox RTSP Server ===\n");
    printf("Video: %dx%d @ %d fps\n", VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_FPS);

    int ret = 0;
    MPP_CHN_S src_chn, dst_chn;
    VENC_STREAM_S stFrame;
    int frame_count = 0;

    // Инициализация RKMPI
    if (RK_MPI_SYS_Init() != RK_SUCCESS) {
        printf("ERROR: RK_MPI_SYS_Init failed\n");
        return -1;
    }

    // Конфигурация VI (Video Input)
    VI_CHN_ATTR_S vi_attr;
    memset(&vi_attr, 0, sizeof(vi_attr));
    vi_attr.stSize.u32Width = VIDEO_WIDTH;
    vi_attr.stSize.u32Height = VIDEO_HEIGHT;
    vi_attr.enPixelFormat = RK_FMT_YUV420SP;
    vi_attr.enCompressMode = COMPRESS_MODE_NONE;
    vi_attr.u32Depth = 2;
    vi_attr.stFrameRate.s32SrcFrameRate = -1;
    vi_attr.stFrameRate.s32DstFrameRate = -1;

    if (RK_MPI_VI_SetChnAttr(0, 0, &vi_attr) != RK_SUCCESS) {
        printf("ERROR: VI_SetChnAttr failed\n");
        goto cleanup;
    }

    if (RK_MPI_VI_EnableChn(0, 0) != RK_SUCCESS) {
        printf("ERROR: VI_EnableChn failed\n");
        goto cleanup;
    }

    // Конфигурация VENC (Video Encoder)
    VENC_CHN_ATTR_S venc_attr;
    memset(&venc_attr, 0, sizeof(venc_attr));
    venc_attr.stVencAttr.enType = RK_VIDEO_ID_AVC;  // H.264
    venc_attr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
    venc_attr.stVencAttr.u32Profile = 77;  // Main Profile
    venc_attr.stVencAttr.u32PicWidth = VIDEO_WIDTH;
    venc_attr.stVencAttr.u32PicHeight = VIDEO_HEIGHT;
    venc_attr.stVencAttr.u32VirWidth = VIDEO_WIDTH;
    venc_attr.stVencAttr.u32VirHeight = VIDEO_HEIGHT;

    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = VIDEO_FPS;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = VIDEO_WIDTH * VIDEO_HEIGHT * VIDEO_FPS / 14;
    venc_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = VIDEO_FPS;
    venc_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = VIDEO_FPS;

    if (RK_MPI_VENC_CreateChn(0, &venc_attr) != RK_SUCCESS) {
        printf("ERROR: VENC_CreateChn failed\n");
        goto cleanup;
    }

    VENC_RECV_PIC_PARAM_S recv_param;
    memset(&recv_param, 0, sizeof(recv_param));
    recv_param.s32RecvPicNum = -1;  // Непрерывный прием

    if (RK_MPI_VENC_StartRecvFrame(0, &recv_param) != RK_SUCCESS) {
        printf("ERROR: VENC_StartRecvFrame failed\n");
        goto cleanup;
    }

    // Связывание VI -> VENC
    src_chn.enModId = RK_ID_VI;
    src_chn.s32DevId = 0;
    src_chn.s32ChnId = 0;

    dst_chn.enModId = RK_ID_VENC;
    dst_chn.s32DevId = 0;
    dst_chn.s32ChnId = 0;

    if (RK_MPI_SYS_Bind(&src_chn, &dst_chn) != RK_SUCCESS) {
        printf("ERROR: Bind VI->VENC failed\n");
        goto cleanup;
    }

    // Инициализация RTSP сервера
    g_rtsplive = create_rtsp_demo(554);
    if (!g_rtsplive) {
        printf("ERROR: Failed to create RTSP server\n");
        goto cleanup;
    }

    g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");
    if (!g_rtsp_session) {
        printf("ERROR: Failed to create RTSP session\n");
        goto cleanup;
    }

    if (rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0) != 0) {
        printf("ERROR: Failed to set RTSP video codec\n");
        goto cleanup;
    }

    rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());

    printf("\n✓ System initialized successfully\n");
    printf("✓ RTSP stream: rtsp://<IP>:554/live/0\n");
    printf("✓ Press Ctrl+C to exit\n\n");

    // Главный цикл
    while (g_running) {
        ret = RK_MPI_VENC_GetStream(0, &stFrame, 1000);

        if (ret == RK_SUCCESS) {
            // Получить указатель на данные
            void *pData = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack[0].pMbBlk);

            // Отправить все пакеты через RTSP
            for (RK_U32 i = 0; i < stFrame.u32PackCount; i++) {
                rtsp_tx_video(g_rtsp_session,
                              (uint8_t *)pData,
                              stFrame.pstPack[i].u32Len,
                              stFrame.pstPack[i].u64PTS);
            }

            rtsp_do_event(g_rtsplive);
            RK_MPI_VENC_ReleaseStream(0, &stFrame);

            frame_count++;
            if (frame_count % 100 == 0) {
                printf("Streamed %d frames\n", frame_count);
            }
        }
    }

cleanup:
    printf("\nCleaning up...\n");

    // Отвязка модулей
    RK_MPI_SYS_UnBind(&src_chn, &dst_chn);

    // Остановка VENC
    RK_MPI_VENC_StopRecvFrame(0);
    RK_MPI_VENC_DestroyChn(0);

    // Остановка VI
    RK_MPI_VI_DisableChn(0, 0);

    // Очистка RTSP
    if (g_rtsp_session) {
        rtsp_del_session(g_rtsp_session);
    }
    if (g_rtsplive) {
        rtsp_del_demo(g_rtsplive);
    }

    // Выход из системы
    RK_MPI_SYS_Exit();

    printf("Done\n");
    return 0;
}
