#ifndef UTILITIES_H
#define UTILITIES_H

#include <cstdint>

/**
 * @class TimerUtils
 * @brief Утилиты для работы со временем
 */
class TimerUtils {
public:
    /**
     * @brief Получает текущее время в микросекундах
     * @return Время в микросекундах
     */
    static uint64_t getCurrentTimeUs();

    /**
     * @brief Рассчитывает FPS на основе двух временных меток
     * @param prevTimeUs Предыдущее время в микросекундах
     * @param currTimeUs Текущее время в микросекундах
     * @return FPS
     */
    static float calculateFps(uint64_t prevTimeUs, uint64_t currTimeUs);
};

/**
 * @class SystemUtils
 * @brief Утилиты для системных операций
 */
class SystemUtils {
public:
    /**
     * @brief Инициализирует Rockchip MPI систему
     * @return Статус инициализации
     */
    static int initMpiSystem();

    /**
     * @brief Завершает работу Rockchip MPI системы
     */
    static void exitMpiSystem();

    /**
     * @brief Запускает системную команду
     * @param command Команда для выполнения
     * @return Код возврата команды
     */
    static int executeSystemCommand(const char *command);
};

#endif // UTILITIES_H
