#pragma once

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddr.h"
#include <memory>

class Channel;
class Socket;
class EventLoop;
class TcpConnection {
public:
  TcpConnection();
  ~TcpConnection();

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  EventLoop* getLoop() const{return loop_;}

private:
  EventLoop *loop_;
  bool reading_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddr localAddr_;
  const InetAddr peerAddr_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  CloseCallback closeCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  Buffer inputBuffer_;
  Buffer outputBuffer_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;