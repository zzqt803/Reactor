#pragma once

#include "Channel.h"
#include "Buffer.h"

class TcpConnection {
public:
  TcpConnection();
  ~TcpConnection();

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

private:
  
};