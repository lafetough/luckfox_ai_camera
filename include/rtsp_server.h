#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <cstdint>
#include <memory>
#include "rtsp_demo.h"

/**
 * @class RtspServer
 * @brief Управляет RTSP сервером для потокового вещания видео
 */
class RtspServer {
public:
    RtspServer(int port = 554);
    ~RtspServer();

    /**
     * @brief Инициализирует RTSP сервер
     * @return Статус инициализации
     */
    int init();

    /**
     * @brief Создает новую RTSP сессию
     * @param path Путь потока (например, "/live/0")
     * @return Статус создания
     */
    int createSession(const char *path);

    /**
     * @brief Настраивает видео кодек для сессии
     * @param codecId ID кодека
     * @param codecData Данные кодека (SPS+PPS для H264)
     * @param dataLen Длина данных кодека
     * @return Статус настройки
     */
    int setVideoCodec(int codecId, const uint8_t *codecData, int dataLen);

    /**
     * @brief Отправляет видео кадр в RTSP сессию
     * @param frame Указатель на данные кадра
     * @param len Размер кадра
     * @param ts Временная метка
     * @return Статус отправки
     */
    int sendVideoFrame(const uint8_t *frame, int len, uint64_t ts);

    /**
     * @brief Обрабатывает события RTSP
     * @return Статус обработки
     */
    int processEvents();

    /**
     * @brief Синхронизирует временные метки видео
     * @param ts Временная метка
     * @param ntpTime NTP время
     * @return Статус синхронизации
     */
    int syncVideoTimestamp(uint64_t ts, uint64_t ntpTime);

    /**
     * @brief Выключает сервер
     */
    void shutdown();

    /**
     * @brief Получает порт сервера
     */
    int getPort() const { return port_; }

    /**
     * @brief Проверяет статус инициализации
     */
    bool isInitialized() const { return initialized_; }

private:
    int port_;
    rtsp_demo_handle handle_;
    rtsp_session_handle session_;
    bool initialized_;
};

#endif // RTSP_SERVER_H
