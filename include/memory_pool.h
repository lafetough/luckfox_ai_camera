#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstdint>
#include "sample_comm.h"

/**
 * @class MemoryPool
 * @brief Управляет пулом памяти для видео буферов
 */
class MemoryPool {
public:
    MemoryPool(uint64_t bufferSize, uint32_t bufferCount);
    ~MemoryPool();

    /**
     * @brief Инициализирует пул памяти
     * @return Статус инициализации
     */
    int init();

    /**
     * @brief Получает блок памяти из пула
     * @param cached True для использования кэша
     * @return Блок памяти (MB_BLK)
     */
    MB_BLK getMemoryBlock(bool cached = true) const;

    /**
     * @brief Получает виртуальный адрес блока памяти
     * @param block Блок памяти
     * @return Виртуальный адрес
     */
    void* getVirtualAddress(MB_BLK block) const;

    /**
     * @brief Освобождает блок памяти
     * @param block Блок памяти
     */
    void releaseMemoryBlock(MB_BLK block);

    /**
     * @brief Уничтожает пул памяти
     */
    void destroy();

    /**
     * @brief Получает ID пула
     */
    MB_POOL getPoolId() const { return poolId_; }

    /**
     * @brief Проверяет инициализацию
     */
    bool isInitialized() const { return initialized_; }

private:
    MB_POOL poolId_;
    uint64_t bufferSize_;
    uint32_t bufferCount_;
    bool initialized_;
};

#endif // MEMORY_POOL_H
