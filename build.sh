#!/bin/bash

# 创建并进入构建目录
mkdir -p build
cd build

# 运行cmake
cmake ..

# 编译
make

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "编译成功！"
    
    # 创建output目录
    cd ..
    mkdir -p output
    
    # 移动可执行文件到output目录
    mv build/webserver output/
    
    # 删除build目录
    rm -rf build
    
    echo "可执行文件已移动到output目录"
else
    echo "编译失败！"
    exit 1
fi 