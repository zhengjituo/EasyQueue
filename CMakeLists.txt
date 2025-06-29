cmake_minimum_required(VERSION 3.10)
project(EasyQueue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置可选参数
option(BUILD_SHARED_LIBS "Build shared libraries (DLLs/SOs)" OFF)
option(BUILD_32BIT "Build for 32-bit architecture" OFF)

# 设置目标架构
if(BUILD_32BIT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
endif()

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 源文件列表
set(SOURCE_FILES
    src/main.cpp
    src/queue_handle_manager.cpp
    src/queue_core.cpp
    producer_module/producer.cpp
    consumer_module/consumer.cpp
)

add_executable(EasyQueue ${SOURCE_FILES})
target_link_libraries(EasyQueue PRIVATE pthread)