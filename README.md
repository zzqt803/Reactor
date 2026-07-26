- netserver/01 第一个通信程序
- netserver/02 封装的客户端和服务端
- netserver/02 实现文件传输功能

- nerserver/IO多路复用/select select服务端-非阻塞IO
- nerserver/IO多路复用/poll poll服务端-非阻塞IO
- nerserver/IO多路复用/epoll epoll服务端-非阻塞IO

//包装原生fd与发生在其上的事件

- include/Channel.h
- src/Channel.cpp

//进行epoll_wait并返回就绪channel

- include/EpollPoller.h
- src/EpollPoller.cpp

//循环调用poll，执行就绪channel的handleEvent

- include/EventLoop.h
- src/EventLoop.cpp

//用户缓冲区，给业务层读写

- include/Buffer.h
- src/Buffer.cpp
