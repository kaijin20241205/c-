cmake_minimum_required(VERSION 3.10)
project(CMUDUO)

# CMAKE_BINARY_DIR变量：运行cmke命令的目录
# 设置生成的可执行文件存放到什么位置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 设置生成的动态库文件存放到什么位置
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 设置参与编译的头文件目录
include_directories("/include")
# 获取参与编译的所有库文件的列表
file(GLOB SOURCES "src/*.cpp")
# 编译生成动态库
add_library(cmuduo SHARED ${SOURCES})


add_subdirectory(Server)
add_subdirectory(Client)