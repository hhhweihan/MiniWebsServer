# MiniWebServer

## 项目简介
重构自WH1024/xuanyuan_webserver仓库。MiniWebServer 是一个基于C++11开发的高性能Web服务器项目；采用Reactor模式设计，使用多线程+非阻塞IO+epoll并发模型，实现了简易高并发的Web服务器

## 核心功能特性
- **并发模型**: 
  - 使用epoll边缘触发(ET)实现IO多路复用
  - 使用多线程充分利用多核CPU资源
  - 使用线程池避免线程频繁创建销毁的开销
  - 使用EPOLLONESHOT保证一个socket连接在任意时刻都只被一个线程处理

- **HTTP服务器**:
  - 支持HTTP GET和POST请求
  - 支持HTTP长连接和短连接
  - 支持静态资源访问
  - 支持HTTP请求报文解析
  - 支持HTTP响应报文生成

- **性能优化**:
  - 使用智能指针管理对象生命周期，避免内存泄漏
  - 使用缓冲区实现异步日志系统
  - 使用定时器处理非活动连接
  - 使用RAII机制管理资源
  - 使用内存映射(mmap)加速文件传输

## 开发环境
- 操作系统: Ubuntu 18.04或更高版本
- 编译器: G++ 7.0或更高版本(支持C++14)
- 构建工具: CMake 3.10或更高版本
- 代码格式化: clang-format
- 版本控制: Git

## 项目结构
```
.
├── output/                    # 输出目录
├── src/                    # 源代码
│   ├── buffer.cpp         # 缓冲区实现
│   ├── http_conn.cpp      # HTTP连接处理
│   ├── log.cpp           # 日志系统
│   └── server.cpp        # 服务器主体
├── include/               # 头文件
│   ├── buffer.h          # 缓冲区
│   ├── http_conn.h       # HTTP连接
│   ├── log.h            # 日志系统
│   ├── threadpool.h      # 线程池
│   └── server.h         # 服务器
├── resources/            # 静态资源目录
├── log/                 # 日志输出目录
├── build.sh            # 构建脚本
├── CMakeLists.txt     # CMake配置文件
└── .clang-format      # 代码格式化配置
```

## 构建和运行
```bash
# 克隆项目
git clone git@github.com:hhhweihan/xuanyuan_webserver.git
cd xuanyuan_webserver

# 构建项目(包含代码格式化)
./build.sh

# 运行服务器
./bin/webserver [port]  # port默认为8080
```

## 代码规范
- 使用Google C++代码风格
- 使用clang-format自动格式化代码
- 遵循现代C++编程实践
- 使用RAII管理资源
- 使用智能指针管理内存

## 性能测试
- 并发连接数: 10000
- 平均响应时间: <10ms
- QPS: 10000+
- 内存占用: <100MB

## 待优化项
1. [ ] 支持HTTPS协议
2. [ ] 实现服务器配置文件
3. [ ] 支持FastCGI
4. [ ] 实现简单的负载均衡
5. [ ] 支持HTTP/2
6. [ ] 完善单元测试
7. [ ] 实现数据库连接池
8. [ ] 支持正则URL匹配

## 参考资料
- 《Linux高性能服务器编程》游双
- 《Linux多线程服务端编程》陈硕
- 《C++并发编程实战》
- [Boost.Asio C++ Network Programming]
- [Modern C++ Tutorial]

## 贡献指南
1. Fork本仓库
2. 创建特性分支
3. 提交代码
4. 创建Pull Request

## 开源协议
本项目采用MIT开源协议。详情请参见[LICENSE](LICENSE)文件。
