#include "memory_pool.h"
#include <cstdio>
#include <cstring>

MemoryPool::MemoryPool(uint64_t bufferSize, uint32_t bufferCount)
    : poolId_(0), bufferSize_(bufferSize), bufferCount_(bufferCount),
      initialized_(false) {
}

MemoryPool::~MemoryPool() {
    destroy();
}

int MemoryPool::init() {
    printf("%s: size=%llu, count=%u\n", __func__, bufferSize_, bufferCount_);

    MB_POOL_CONFIG_S PoolCfg;
    memset(&PoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
    PoolCfg.u64MBSize = bufferSize_ ;
    PoolCfg.u32MBCnt = bufferCount_;
    PoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;

    poolId_ = RK_MPI_MB_CreatePool(&PoolCfg);
    printf("Pool id: %d\n", poolId_);

    initialized_ = true;
    printf("Memory pool created successfully\n");
    return 0;
}

MB_BLK MemoryPool::getMemoryBlock(bool cached) const {
    if (!initialized_) {
        printf("ERROR: Memory pool not initialized\n");
        return nullptr;
    }

    MB_BLK block = RK_MPI_MB_GetMB(poolId_, bufferSize_, static_cast<RK_BOOL>(cached));
    if (!block) {
        printf("ERROR: Failed to get memory block from pool\n");
        return nullptr;
    }

    return block;
}

void* MemoryPool::getVirtualAddress(MB_BLK block) const {
    if (!block) {
        printf("ERROR: Block is null\n");
        return nullptr;
    }

    void *addr = RK_MPI_MB_Handle2VirAddr(block);
    if (!addr) {
        printf("ERROR: Failed to get virtual address\n");
        return nullptr;
    }

    return addr;
}

void MemoryPool::releaseMemoryBlock(MB_BLK block) {
    if (!block) {
        return;
    }

    RK_MPI_MB_ReleaseMB(block);
}

void MemoryPool::destroy() {
    if (!initialized_) {
        return;
    }

    if (poolId_) {
        releaseMemoryBlock(getMemoryBlock());
        RK_MPI_MB_DestroyPool(poolId_);
        poolId_ = 0;
    }

    initialized_ = false;
}
