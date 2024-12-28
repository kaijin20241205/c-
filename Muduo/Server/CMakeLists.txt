cmake_minimum_required(VERSION 3.10)

project(Server)

add_executable(server server.cpp)
# 设置动态库的目录
link_directories(${CMAKE_SOURCE_DIR}/build/lib)

# 将动态库链接到可执行文件
target_link_libraries(server cmuduo)