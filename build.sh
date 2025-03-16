#!/bin/bash

# 创建必要的目录
mkdir -p build
mkdir -p output
mkdir -p output/log

# 进入构建目录
cd build

# 运行cmake
cmake ..

# 编译
make

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "Build successful!"
    
    # 回到项目根目录
    cd ..
    
    # 移动可执行文件到output目录
    mv build/webserver output/
    
    # 复制资源文件到output目录
    if [ -d "resources" ]; then
        echo "Copying resources to output directory..."
        cp -r resources/* output/
        mkdir -p output/pic
        cp -r resources/pic/* output/pic/
    fi
    
    # 获取服务器公网IP
    PUBLIC_IP=$(curl -s http://ipinfo.io/ip)
    echo "Server public IP: ${PUBLIC_IP}"
    
    # 检查并开放端口（默认使用9006端口）
    echo "Checking firewall status..."
    if command -v ufw &> /dev/null; then
        sudo ufw status | grep -q "9006/tcp" || {
            echo "Opening port 9006..."
            sudo ufw allow 9006/tcp
            sudo ufw reload
        }
    fi
    
    echo "Build completed! Executable and resources are in output directory."
    echo "To test the server:"
    echo "1. Start the server: cd output && ./webserver"
    echo "2. Access from another computer using: http://${PUBLIC_IP}:9006"
else
    echo "Build failed!"
    exit 1
fi

sudo netstat -tulpn | grep 9006

sudo service nginx stop

sudo ufw status 