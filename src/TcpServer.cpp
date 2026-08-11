#include "TcpServer.h"
#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"

TcpServer::TcpServer(EventLoop *loop, const InetAddr &listenAddr,
                     const string &nameArg, Option option)
    : loop_(loop), ipPort_(listenAddr.toIpPort()), name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(ConnectionCallback()),
      messageCallback_(MessageCallback()), nextConnId_(1) {
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {
  LOG_TRACE("TcpServer::~TcpServer [{}] destructing", name_);

  for (auto &[_,connPtr] : connections_) {
    TcpConnectionPtr conn(connPtr);
    connPtr.reset();
    conn->getLoop()->runInLoop(
        
    );
  }
}