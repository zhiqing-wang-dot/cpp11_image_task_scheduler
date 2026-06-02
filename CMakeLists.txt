cmake_minimum_required(VERSION 3.18)

project(image_task_scheduler LANGUAGES CXX)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(image_task_scheduler
    src/main.cpp
    src/timer.cpp
    src/logger.cpp
    src/image_task.cpp
    src/gray_processor.cpp
    src/sobel_processor.cpp
    src/resize_processor.cpp
    src/blur_processor.cpp
    src/factory_processor.cpp
    src/file_utils.cpp
    src/config.cpp
    src/thread_pool.cpp
    src/task_scheduler.cpp
    src/quality_analyzer.cpp
    src/quality_report_write.cpp
    src/quality_scheduler.cpp
    src/dataset_cleaner.cpp
    src/dataset_pipline.cpp
)

target_include_directories(image_task_scheduler PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(image_task_scheduler PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
)
