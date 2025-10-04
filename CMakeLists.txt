cmake_minimum_required(VERSION 3.5)

project(video_luckfox LANGUAGES CXX)

set(SDK_PATH "/media/user/01DC32B96D4CA3B0/LuckFoxProject/luckfox-pico")
set(SDK_MEDIA_OUT "${SDK_PATH}/media/out")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

link_directories(${SDK_MEDIA_OUT}/lib)

set(OpenCV_DIR ${CMAKE_CURRENT_LIST_DIR}/../opencv-mobile-4.12.0-luckfox-pico/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


add_executable(video_luckfox main.cpp)

add_compile_options(-g -Wall
                    -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF
                    -DROCKIVA -DUAPI2
                    -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
                    )

target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    rknnmrt
    Threads::Threads
    rockiva
    rockit
    rockchip_mpp
    rkaiq
    pthread
    rga
    rtsp
)

# Заголовочные файлы из SDK
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${SDK_MEDIA_OUT}/include
    ${SDK_MEDIA_OUT}/include/rga
    ${SDK_MEDIA_OUT}/include/rkaiq
    ${SDK_MEDIA_OUT}/include/rockiva
    ${SDK_MEDIA_OUT}/include/rockchip
    ${SDK_MEDIA_OUT}/include/libdrm
  )

include(GNUInstallDirs)
install(TARGETS video_luckfox
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
