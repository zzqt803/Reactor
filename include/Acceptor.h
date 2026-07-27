#pragma once

#include <functional>

#include "Channel.h"
#include "EventLoop.h"

class EventLoop;
class Acceptor {

  using NewConnectionCallback = std::function<void(int sockfd)>;

public:
  Acceptor();
  ~Acceptor();

  Acceptor(const Acceptor &) = delete;
  Acceptor &operator=(const Acceptor &) = delete;

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }

  void listen();


private:
  void handleRead();
  
  EventLoop *loop_;
  //监听文件描述符
  int acceptFd_;
  //包装监听文件描述符
  Channel acceptChannel_;

  NewConnectionCallback newConnectionCallback_;

  bool listening_;
  
};