cmake_minimum_required(VERSION 3.10)
project(my_luckfox_rtsp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# Пути к сторонним библиотекам
set(RKMEDIA_PATH ${CMAKE_SOURCE_DIR}/../luckfox-pico/media/out)
set(RKMEDIA_LIB_PATH ${RKMEDIA_PATH}/lib/)

# Заголовочные файлы
include_directories(
    ${RKMEDIA_PATH}/include
)

# Пути к библиотекам
link_directories(${RKMEDIA_LIB_PATH})

# Исходные файлы
add_executable(${PROJECT_NAME}
    main.cpp
)

# Линковка библиотек (ИСПРАВЛЕНО: добавлены все необходимые библиотеки)
target_link_libraries(${PROJECT_NAME}
    rockit
    rockchip_mpp
    pthread
    rtsp
    rga
)

# Установка RPATH для поиска библиотек на устройстве
set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "${RKMEDIA_LIB_PATH}:$ORIGIN/lib"
    INSTALL_RPATH "$ORIGIN/lib"
)

# Установка (деплой)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Копирование библиотек при деплое
install(DIRECTORY ${RKMEDIA_LIB_PATH}/
        DESTINATION lib
        FILES_MATCHING PATTERN "*.so*")
