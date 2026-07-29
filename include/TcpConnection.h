#pragma once

#include "Buffer.h"

class Channel;
class Socket;
class EventLoop;
class TcpConnection {
public:
  TcpConnection();
  ~TcpConnection();

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

private:
  
};