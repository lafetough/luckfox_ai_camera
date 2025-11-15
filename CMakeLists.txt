cmake_minimum_required(VERSION 3.5)

project(video_luckfox LANGUAGES CXX)

set(SDK_PATH "/media/user/01DC32B96D4CA3B0/LuckFoxProject/luckfox-pico")
set(SDK_MEDIA_OUT "${SDK_PATH}/media/out")
set(S "/media/user/01DC32B96D4CA3B0/LuckFoxProject/luckfox-pico/media/samples/example/common/isp3.x")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

link_directories(${SDK_MEDIA_OUT}/lib)

set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(OpenCV_DIR ${CMAKE_CURRENT_LIST_DIR}/../opencv-mobile-4.12.0-luckfox-pico/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/src/luckfox_mpi.cc"
)
set(HEADERS
    "${INCLUDE_DIR}/luckfox_mpi.h"
    "${INCLUDE_DIR}/rtsp_demo.h"
    "${INCLUDE_DIR}/loadbmp.h"
    "${INCLUDE_DIR}/sample_comm.h"
    "${INCLUDE_DIR}/sample_comm_isp.h"
)


find_package(Threads REQUIRED)

add_compile_definitions(RV1106_1103)

add_executable(video_luckfox
    ${SOURCES}
    ${HEADERS}
    )

add_compile_options(-g -Wall
                    -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF
                    -DROCKIVA -DUAPI2
                    -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
                    )

target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    rknnmrt
    Threads::Threads
    rknnmrt
    rockiva
    rockit
    rockchip_mpp
    rkaiq
    pthread
    rga
    rtsp
    ${S}/sample_comm_isp.o
)

# Заголовочные файлы из SDK
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${SDK_MEDIA_OUT}/include
    ${CMAKE_CURRENT_LIST_DIR}/rknn/include
    ${SDK_MEDIA_OUT}/include/rga
    ${SDK_MEDIA_OUT}/include/rkaiq
    ${SDK_MEDIA_OUT}/include/rockiva
    ${SDK_MEDIA_OUT}/include/rockchip
    ${SDK_MEDIA_OUT}/include/libdrm
    ${SDK_MEDIA_OUT}/include/libkms
    ${SDK_MEDIA_OUT}/include/rga
    ${SDK_MEDIA_OUT}/include/rockive
    ${SDK_MEDIA_OUT}/include/rkaiq/common
    ${SDK_MEDIA_OUT}/include/rkaiq/xcore
    ${SDK_MEDIA_OUT}/include/rkaiq/algos
    ${SDK_MEDIA_OUT}/include/rkaiq/iq_parser
    ${SDK_MEDIA_OUT}/include/rkaiq/iq_parser_v2
    ${SDK_MEDIA_OUT}/include/rkaiq/uAPI2
    ${SDK_MEDIA_OUT}/include/rkaiq/
    ${SDK_MEDIA_OUT}/rkisp_demo/demo/sample
    ${SDK_MEDIA_OUT}/rkisp_demo/demo

    ${CMAKE_CURRENT_LIST_DIR}/include
    ${S}/sample_comm_isp.h
)

include(GNUInstallDirs)
install(TARGETS video_luckfox
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
