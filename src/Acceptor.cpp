#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddr.h"
#include "util/Logger.h"

#include <fcntl.h>
#include <unistd.h>

Acceptor::Acceptor(EventLoop *loop, const InetAddr &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(Socket::createNoblockingOrDie(listenAddr.family())),
      acceptChannel_(loop, acceptSocket_.fd()), listening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.remove();
  ::close(idleFd_);
}

void Acceptor::listen() {
  loop_->assertInLoopThread();
  listening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
  LOG_TRACE("Acceptor::listen listenfd={}", acceptChannel_.fd());
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddr peeraddr;
  int connfd = acceptSocket_.accept(&peeraddr);
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peeraddr);
    } else {
      Socket::close(connfd);
    }
  } else {
    LOG_SYSERR("Acceptor::handleRead");
    if (errno == EMFILE) {
      ::close(idleFd_);
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}