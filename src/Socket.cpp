#include "Socket.h"
#include "Logger.h"

#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

Socket::~Socket() { close(sockfd_); }

void Socket::bindAddress(const InetAddr &localaddr) {}

void Socket::listen() {}

int Socket::accept(InetAddr *peeraddr) { return 0; }

void Socket::shutdownWrite() {}

void Socket::setTcpNoDelay(bool on) {}

void Socket::setReuseAddr(bool on) {}

void Socket::setReusePort(bool on) {}

void Socket::setKeepAlive(bool on) {}

/**
 * @brief 静态工具方法
 *
 */
int Socket::createNoblockingOrDie(sa_family_t family) {
  int sockfd =
      ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

  if (sockfd < 0) {
    LOG_FATAL("Socket::createNoblockingOrDie");
  }

  return sockfd;
}

void Socket::bindOrDie(int sockfd, const struct sockaddr *addr) {
  int ret =
      ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG_SYSFATAL("Socket::bindOrDie");
  }
}

void Socket::listenOrDie(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    LOG_SYSFATAL("Socket::listenOrDie");
  }
}

void Socket::close(int sockfd) {
  if (::close(sockfd) < 0) {
    LOG_SYSERR("Socket::close");
  }
}

int Socket::accept(int sockfd, struct sockaddr_in6 *addr) { return 0; }