#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddr.h"
#include "Logger.h"

Acceptor::Acceptor(EventLoop *loop, const InetAddr &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(Socket::createNoblockingOrDie(listenAddr.family())),
      acceptChannel_(loop, acceptSocket_.fd()), listening_(false) {
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void Acceptor::listen() {
  listening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
  InetAddr peeraddr;
  int connfd = acceptSocket_.accept(&peeraddr);
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd);
    } else {
      Socket::close(connfd);
    }
  } else {
    LOG_SYSERR("Acceptor::handleRead");
    if (errno == EMFILE) {
      // TODO
    }
  }
}