cmake_minimum_required(VERSION 3.10)
project(XuanYuanWebServer CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 查找源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(webserver ${SOURCES})

# 链接pthread库
target_link_libraries(webserver pthread)

# 创建必要的目录
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/log)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/resources)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# 复制资源文件到bin目录
file(COPY ${PROJECT_SOURCE_DIR}/resources/ DESTINATION ${PROJECT_SOURCE_DIR}/bin/resources/)