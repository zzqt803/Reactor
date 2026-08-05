#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

TcpServer::TcpServer(EventLoop *loop, const InetAddr &listenAddr,
                     const string &nameArg, Option option)
    : loop_(loop), ipPort_(listenAddr.toIpPort()), name_(nameArg),
      acceptor_(new Acceptor(loop,listenAddr,option==kReusePort))
                           {
  
}
