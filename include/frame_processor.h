#ifndef FRAME_PROCESSOR_H
#define FRAME_PROCESSOR_H

#include "memory_pool.h"
#include "rk_comm_video.h"
#include <cstdint>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

/**
 * @class FrameProcessor
 * @brief Обрабатывает видео кадры используя OpenCV
 */
class FrameProcessor {
public:
    FrameProcessor(int width = 720, int height = 480);
    ~FrameProcessor();

    /**
     * @brief Инициализирует источник видео (камеру)
     * @return Статус инициализации
     */
    int initVideoCapture();

    void initFrame(const MemoryPool& mem_pool);

    void updateTimeForFrame();

    /**
     * @brief Получает кадр с камеры
     * @param frame Матрица для сохранения кадра
     * @return True если кадр успешно захвачен
     */
    bool captureFrame();

    float getFps() const;

    /**
     * @brief Добавляет текст FPS на кадр
     * @param frame Матрица кадра
     * @param fps Значение FPS
     */
    void drawFpsText(float fps);

    /**
     * @brief Закрывает источник видео
     */
    void releaseCapture();

    /**
     * @brief Получает ширину кадра
     */
    int getWidth() const { return width_; }

    /**
     * @brief Получает высоту кадра
     */
    int getHeight() const { return height_; }

    /**
     * @brief Проверяет открытие камеры
     */
    bool isCaptureOpened() const { return capture_.isOpened(); }

    const VIDEO_FRAME_INFO_S* getFrameForVenc() const;

private:
    int width_;
    int height_;
    cv::VideoCapture capture_;

    VIDEO_FRAME_INFO_S h264_frame;
    RK_U32 H264_TimeRef = 0;

    cv::Mat frame;
};

#endif // FRAME_PROCESSOR_H
