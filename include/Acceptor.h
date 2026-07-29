#pragma once

#include <functional>

#include "Channel.h"
#include "Socket.h"


class EventLoop;
class InetAddr;
class Acceptor {

  using NewConnectionCallback = std::function<void(int sockfd)>;

public:
  Acceptor(EventLoop *loop,const InetAddr& listenAddr,bool reuseport);
  ~Acceptor();

  Acceptor(const Acceptor &) = delete;
  Acceptor &operator=(const Acceptor &) = delete;

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }

  void listen();

  bool listening() const {return listening_;}


private:
  void handleRead();

  EventLoop *loop_;
  //监听文件描述符
  int acceptFd_;
  //管理socket
  Socket acceptSocket_;
  //包装监听文件描述符
  Channel acceptChannel_;


  NewConnectionCallback newConnectionCallback_;

  bool listening_;
  
};