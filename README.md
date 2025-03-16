# XuanYuan Web Server

重构自WH1024/xuanyuan_webserver仓库；MiniWebServer是一个基于C++11采用Reactor模式设计，使用多线程+非阻塞IO+epoll并发模型，实现了简易高并发的Web服务器

## 功能特点

- 使用 Epoll 边缘触发的 I/O 多路复用技术
- 使用多线程充分利用多核CPU，并使用线程池避免频繁创建销毁线程的开销
- 使用优先队列实现的定时器，用于处理超时连接
- 实现了异步日志系统，记录服务器运行状态
- 使用状态机解析HTTP请求，支持GET和POST请求
- 支持长连接和短连接
- 支持静态资源访问

## 环境要求

- Linux操作系统（推荐Ubuntu 18.04+）
- C++11及以上
- CMake 3.10+
- GCC/G++

## 快速开始

1. 克隆项目：
```bash
git clone git@github.com:hhhweihan/MiniWebsServer.git
cd MiniWebsServer
```

2. 编译项目：
```bash
chmod +x build.sh
./build.sh
```

3. 运行服务器：
```bash
cd output
./webserver
```

## 配置说明

服务器默认配置：
- 端口：9006
- 触发模式：ET
- 日志等级：1
- 最大连接数：65536

## 目录结构

```
MiniWebsServer/
├── include/         # 头文件目录
├── src/            # 源文件目录
├── resources/      # 静态资源目录
├── log/           # 日志输出目录
├── output/        # 构建输出目录
├── build.sh       # 构建脚本
├── CMakeLists.txt # CMake配置文件
└── README.md      # 项目说明文档
```

## 测试验证

1. 本地测试：
```bash
curl http://localhost:9006
```

2. 远程测试：
- 在浏览器中访问：`http://[服务器IP]:9006`
- 使用curl命令：`curl http://[服务器IP]:9006`

## 性能测试

使用Apache Benchmark进行压力测试：
```bash
# 安装ab工具
sudo apt-get install apache2-utils

# 进行压力测试（发送1000个请求，并发数100）
ab -n 1000 -c 100 http://[服务器IP]:9006/
```

## 注意事项

1. 确保防火墙允许9006端口的访问
2. 服务器需要具有足够的权限来创建和写入日志文件
3. 在生产环境中使用时，建议配置反向代理（如Nginx）

## 贡献指南

欢迎提交Issue和Pull Request来帮助改进项目。
