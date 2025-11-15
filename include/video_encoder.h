#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <cstdint>
#include "sample_comm.h"

/**
 * @class VideoEncoder
 * @brief Управляет кодированием видео (VENC) на Rockchip RV1106
 */
class VideoEncoder {
public:
    VideoEncoder(int width = 720, int height = 480, int bitrate = 3072);
    ~VideoEncoder();

    /**
     * @brief Инициализирует видео кодер
     * @param channelId ID канала кодирования
     * @param codecType Тип кодека (RK_VIDEO_ID_AVC, RK_VIDEO_ID_HEVC и т.д.)
     * @return Статус инициализации
     */
    int init(int channelId, RK_CODEC_ID_E codecType);

    /**
     * @brief Отправляет кадр на кодирование
     * @param frame Указатель на VIDEO_FRAME_INFO_S
     * @return Статус отправки
     */
    int sendFrame(const VIDEO_FRAME_INFO_S *frame) const;

    /**
     * @brief Получает закодированный поток
     * @param stream Указатель на VENC_STREAM_S
     * @return Статус получения
     */
    int getStream(VENC_STREAM_S *stream);

    /**
     * @brief Освобождает поток кодирования
     * @param stream Указатель на VENC_STREAM_S
     * @return Статус освобождения
     */
    int releaseStream(VENC_STREAM_S *stream);

    /**
     * @brief Останавливает кодер
     */
    void shutdown();

    /**
     * @brief Получает текущий битрейт
     */
    int getBitrate() const { return bitrate_; }

    /**
     * @brief Получает ID канала
     */
    int getChannelId() const { return channelId_; }

private:
    int width_;
    int height_;
    int bitrate_;
    int channelId_;
    bool initialized_;

    int configureEncoder(RK_CODEC_ID_E codecType);
    int startReceivingFrames();
};

#endif // VIDEO_ENCODER_H
