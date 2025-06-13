cmake_minimum_required(VERSION 3.10)
project(queue_core C CXX)

# 设置 C++ 标准
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

# # 添加模块
# add_library(producer src/producer/producer.cpp)
# add_library(consumer src/consumer/consumer.cpp)
# add_library(fifo src/fifo/fifo.cpp)
# add_library(global_flags src/global_flags/global_flags.cpp)

# 设置头文件路径
# target_include_directories(producer PUBLIC include)
# target_include_directories(consumer PUBLIC include)
#target_include_directories(queue PUBLIC include)
# target_include_directories(global_flags PUBLIC include)  # 确保其他模块可以访问 global_flags.h

# 添加可执行文件
add_executable(${PROJECT_NAME} src/main.cpp )

target_include_directories(${PROJECT_NAME} PUBLIC include)
# 链接所有模块
# target_link_libraries(${PROJECT_NAME}
#     queue_core
# )


# target_link_libraries(${PROJECT_NAME}
#     producer
#     consumer
#     fifo
#     global_flags
#     pthread
# )

# 设置生成共享库（如果启用）
# if(BUILD_SHARED_LIBS)
#     set_target_properties(producer PROPERTIES COMPILE_FLAGS "-fPIC")
#     set_target_properties(consumer PROPERTIES COMPILE_FLAGS "-fPIC")
#     set_target_properties(fifo PROPERTIES COMPILE_FLAGS "-fPIC")
#     set_target_properties(global_flags PROPERTIES COMPILE_FLAGS "-fPIC")
#     set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")

#     set_target_properties(producer PROPERTIES POSITION_INDEPENDENT_CODE ON)
#     set_target_properties(consumer PROPERTIES POSITION_INDEPENDENT_CODE ON)
#     set_target_properties(fifo PROPERTIES POSITION_INDEPENDENT_CODE ON)
#     set_target_properties(global_flags PROPERTIES POSITION_INDEPENDENT_CODE ON)
#     set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
# endif()