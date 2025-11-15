#include "utilities.h"
#include <time.h>
#include <cstdlib>
#include <cstdio>
#include "luckfox_mpi.h"

// TimerUtils implementation
uint64_t TimerUtils::getCurrentTimeUs() {
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (uint64_t)time.tv_sec * 1000000 + (uint64_t)time.tv_nsec / 1000;
}

float TimerUtils::calculateFps(uint64_t prevTimeUs, uint64_t currTimeUs) {
    if (currTimeUs <= prevTimeUs) {
        return 0.0f;
    }

    uint64_t diffUs = currTimeUs - prevTimeUs;
    if (diffUs == 0) {
        return 0.0f;
    }

    return 1000000.0f / (float)diffUs;
}

// SystemUtils implementation
int SystemUtils::initMpiSystem() {
    printf("Initializing RK MPI system...\n");

    int ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_SYS_Init failed: %x\n", ret);
        return ret;
    }

    printf("RK MPI system initialized successfully\n");
    return RK_SUCCESS;
}

void SystemUtils::exitMpiSystem() {
    printf("Exiting RK MPI system...\n");
    RK_MPI_SYS_Exit();
}

int SystemUtils::executeSystemCommand(const char *command) {
    if (!command) {
        printf("ERROR: Command is null\n");
        return -1;
    }

    int ret = system(command);
    return ret;
}
