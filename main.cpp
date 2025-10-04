#include <stdio.h>
#include <stdlib.h>

// Заголовочные файлы Rockchip Media Processing Platform (MPP)
#include "rk_type.h"        // Базовые типы данных Rockchip (RK_U64, RK_S32 и т.д.)
#include "rk_mpi_sys.h"     // Системные функции MPI (инициализация/деинициализация)
#include "rk_comm_venc.h"   // Структуры и константы для видеокодирования
#include "rk_mpi_venc.h"    // API для работы с видеокодером (VENC)
#include "rk_mpi_mb.h"      // API для работы с буферами памяти (Memory Buffer)
#include "rtsp_demo.h"      // Библиотека для работы с RTSP сервером

// Заголовочные файлы OpenCV для работы с изображениями
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;

/**
 * Функция инициализации видеокодера (Video Encoder)
 * @param chnId - ID канала кодирования (обычно 0)
 * @param width - ширина видео в пикселях
 * @param height - высота видео в пикселях
 * @param enType - тип кодека (H264/H265/MJPEG и т.д.)
 * @return 0 при успешной инициализации
 */
int venc_init(int chnId, int width, int height, RK_CODEC_ID_E enType) {
    printf("%s\n", __func__);

    VENC_RECV_PIC_PARAM_S stRecvParam;  // Параметры приема кадров
    VENC_CHN_ATTR_S stAttr;             // Атрибуты канала кодирования
    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));

    // Настройка параметров кодирования для RTSP H264
    stAttr.stVencAttr.enType = enType;  // Тип кодека (H264/AVC)

    // Формат пикселей входного изображения
    // RK_FMT_YUV420SP - формат YUV (закомментирован)
    stAttr.stVencAttr.enPixelFormat = RK_FMT_RGB888;  // RGB формат с 3 байтами на пиксель

    // Профиль H.264 - определяет набор возможностей кодека
    stAttr.stVencAttr.u32Profile = H264E_PROFILE_MAIN;  // Main Profile для баланса качества/производительности

    // Размеры изображения
    stAttr.stVencAttr.u32PicWidth = width;    // Ширина кадра
    stAttr.stVencAttr.u32PicHeight = height;  // Высота кадра
    stAttr.stVencAttr.u32VirWidth = width;    // Виртуальная ширина (для выравнивания памяти)
    stAttr.stVencAttr.u32VirHeight = height;  // Виртуальная высота

    // Количество буферов для потоковой передачи
    stAttr.stVencAttr.u32StreamBufCnt = 2;  // Двойная буферизация

    // Размер буфера (для YUV420 требуется width*height*3/2 байт)
    stAttr.stVencAttr.u32BufSize = width * height * 3 / 2;

    stAttr.stVencAttr.enMirror = MIRROR_NONE;  // Без зеркального отражения изображения

    // Режим управления битрейтом - Constant Bit Rate (CBR) для стабильного потока
    stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;

    // Целевой битрейт: 3 Мбит/с (3 * 1024 кбит/с)
    stAttr.stRcAttr.stH264Cbr.u32BitRate = 3 * 1024;

    // Group of Pictures - количество кадров между ключевыми кадрами (I-frames)
    // GOP=1 означает каждый кадр - ключевой (хорошо для низкой задержки, но больше размер)
    stAttr.stRcAttr.stH264Cbr.u32Gop = 1;

    // Создание канала кодирования с заданными атрибутами
    RK_MPI_VENC_CreateChn(chnId, &stAttr);

    // Настройка параметров приема кадров
    memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
    stRecvParam.s32RecvPicNum = -1;  // -1 означает бесконечный прием кадров

    // Запуск приема кадров на канале кодирования
    RK_MPI_VENC_StartRecvFrame(chnId, &stRecvParam);

    return 0;
}

/**
 * Функция получения текущего времени в микросекундах
 * Используется для расчета PTS (Presentation Time Stamp) и FPS
 * @return время в микросекундах с момента старта системы
 */
RK_U64 TEST_COMM_GetNowUs() {
    struct timespec time = {0, 0};

    // Получение монотонного времени (не зависит от изменения системных часов)
    clock_gettime(CLOCK_MONOTONIC, &time);

    // Конвертация в микросекунды: секунды * 1000000 + наносекунды / 1000
    return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000;
}

int main() {
    // Остановка фоновых процессов Rockchip (если запущены)
    system("RkLunch-stop.sh");

    // Переменные для работы с API Rockchip
    RK_S32 s32Ret = 0;  // Код возврата функций

    // Неиспользуемые переменные (возможно для будущих расширений)
    int sX, sY, eX, eY;

    // Разрешение видео
    int width = 720;
    int height = 480;

    // Переменные для отображения FPS
    char fps_text[16];
    float fps = 0;
    memset(fps_text, 0, 16);

    RK_U64 nowUs;  // Текущее время в микросекундах

    // Инициализация камеры через OpenCV
    cv::VideoCapture cap;
    cv::Mat bgr;  // Матрица для хранения кадра в формате BGR

    // Настройка параметров камеры
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.open(0);  // Открытие камеры /dev/video0

    // Инициализация системы Rockchip MPI
    if (RK_MPI_SYS_Init() != RK_SUCCESS) {
        RK_LOGE("rk mpi sys init fail!");
        return -1;
    }

    // === Настройка структур для H264 кодирования ===

    // Структура для хранения закодированного H264 потока
    VENC_STREAM_S stFrame;
    stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));

    // Счетчики для временных меток видео
    RK_U64 H264_PTS = 0;      // Presentation Time Stamp (не используется в коде)
    RK_U32 H264_TimeRef = 0;  // Счетчик кадров

    VIDEO_FRAME_INFO_S stVpssFrame;  // Неиспользуемая структура

    // === Создание пула памяти DMA для эффективной работы с видео ===

    MB_POOL_CONFIG_S PoolCfg;
    memset(&PoolCfg, 0, sizeof(MB_POOL_CONFIG_S));

    // Размер блока памяти: width * height * 3 байта (для RGB888)
    PoolCfg.u64MBSize = width * height * 3;

    // Количество блоков памяти в пуле
    PoolCfg.u32MBCnt = 1;

    // Тип выделения памяти - DMA (Direct Memory Access) для аппаратного доступа
    PoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;

    // Создание пула памяти
    MB_POOL src_Pool = RK_MPI_MB_CreatePool(&PoolCfg);
    printf("Create Pool success !\n");

    // Получение блока памяти из пула (RK_TRUE = блокирующий вызов)
    MB_BLK src_Blk = RK_MPI_MB_GetMB(src_Pool, width * height * 3, RK_TRUE);

    // === Настройка структуры кадра для H264 кодирования ===

    VIDEO_FRAME_INFO_S h264_frame;
    h264_frame.stVFrame.u32Width = width;           // Ширина кадра
    h264_frame.stVFrame.u32Height = height;         // Высота кадра
    h264_frame.stVFrame.u32VirWidth = width;        // Виртуальная ширина
    h264_frame.stVFrame.u32VirHeight = height;      // Виртуальная высота
    h264_frame.stVFrame.enPixelFormat = RK_FMT_RGB888;  // Формат пикселей
    h264_frame.stVFrame.u32FrameFlag = 160;         // Флаги кадра
    h264_frame.stVFrame.pMbBlk = src_Blk;           // Привязка блока памяти

    // Получение виртуального адреса памяти для записи данных
    unsigned char *data = (unsigned char *)RK_MPI_MB_Handle2VirAddr(src_Blk);

    // Создание OpenCV Mat, который использует DMA память напрямую (zero-copy)
    cv::Mat frame(cv::Size(width, height), CV_8UC3, data);

    // === Инициализация RTSP сервера ===

    rtsp_demo_handle g_rtsplive = NULL;
    rtsp_session_handle g_rtsp_session;

    // Создание RTSP сервера на порту 554 (стандартный порт RTSP)
    g_rtsplive = create_rtsp_demo(554);

    // Создание RTSP сессии с URL: rtsp://<IP>:554/live/0
    g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");

    // Установка видеопотока как H264 (без SPS/PPS в параметрах)
    rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);

    // Синхронизация временных меток RTSP с системным временем
    rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());

    // === Инициализация видеокодера ===

    RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;  // AVC = H.264
    venc_init(0, width, height, enCodecType);

    printf("init success\n");

    // === Главный цикл захвата и кодирования видео ===

    while(1) {
        // Установка временных меток для текущего кадра
        h264_frame.stVFrame.u32TimeRef = H264_TimeRef++;  // Инкремент счетчика кадров
        h264_frame.stVFrame.u64PTS = TEST_COMM_GetNowUs();  // Временная метка в микросекундах

        // Захват кадра с камеры в формате BGR
        cap >> bgr;

        // Подготовка текста FPS для отображения
        sprintf(fps_text, "fps = %.2f", fps);

        // Наложение текста FPS на изображение
        cv::putText(bgr, fps_text,
                    cv::Point(40, 40),                // Позиция текста
                    cv::FONT_HERSHEY_SIMPLEX, 1,      // Шрифт и размер
                    cv::Scalar(0, 255, 0), 2);        // Зеленый цвет, толщина 2

        // Конвертация BGR -> RGB (OpenCV использует BGR, а кодер ожидает RGB)
        // Результат записывается напрямую в DMA память
        cv::cvtColor(bgr, frame, cv::COLOR_BGR2RGB);

        // === Отправка кадра на кодирование ===

        // Отправка кадра в канал видеокодера (0), -1 = бесконечный таймаут
        RK_MPI_VENC_SendFrame(0, &h264_frame, -1);

        // Получение закодированного H264 потока
        s32Ret = RK_MPI_VENC_GetStream(0, &stFrame, -1);

        // Если кодирование успешно
        if(s32Ret == RK_SUCCESS) {
            // Если RTSP сервер и сессия активны
            if(g_rtsplive && g_rtsp_session) {
                // Получение указателя на закодированные данные H264
                void *pData = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);

                // Отправка H264 пакета через RTSP
                rtsp_tx_video(g_rtsp_session,
                              (uint8_t *)pData,              // Данные H264
                              stFrame.pstPack->u32Len,       // Длина пакета
                              stFrame.pstPack->u64PTS);      // Временная метка

                // Обработка событий RTSP сервера (прием команд клиентов)
                rtsp_do_event(g_rtsplive);
            }

            // Расчет FPS: 1000000 мкс / время обработки кадра
            nowUs = TEST_COMM_GetNowUs();
            fps = (float) 1000000 / (float)(nowUs - h264_frame.stVFrame.u64PTS);
        }

        // Освобождение закодированного потока (возврат буфера в пул)
        s32Ret = RK_MPI_VENC_ReleaseStream(0, &stFrame);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
        }
    }

    // === Очистка ресурсов (код после цикла не выполнится из-за while(1)) ===

    // Освобождение блока памяти
    RK_MPI_MB_ReleaseMB(src_Blk);

    // Уничтожение пула памяти
    RK_MPI_MB_DestroyPool(src_Pool);

    // Остановка приема кадров на канале кодирования
    RK_MPI_VENC_StopRecvFrame(0);

    // Уничтожение канала кодирования
    RK_MPI_VENC_DestroyChn(0);

    // Освобождение памяти для структуры потока
    free(stFrame.pstPack);

    // Удаление RTSP сервера
    if (g_rtsplive)
        rtsp_del_demo(g_rtsplive);

    // Деинициализация системы Rockchip MPI
    RK_MPI_SYS_Exit();

    return 0;
}
