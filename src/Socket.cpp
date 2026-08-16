#include "Socket.h"
#include "InetAddr.h"
#include "util/Logger.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

Socket::~Socket() { close(sockfd_); }

void Socket::bindAddress(const InetAddr &localaddr) {
  bindOrDie(sockfd_, localaddr.getSockAddr());
}

void Socket::listen() { listenOrDie(sockfd_); }

int Socket::accept(InetAddr *peeraddr) {
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof addr);
  int connfd = accept(sockfd_, &addr);
  if (connfd >= 0) {
    peeraddr->setSockAddrInet6(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() { shutdownWrite(sockfd_); }

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,
               static_cast<socklen_t>(sizeof optval));
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
               static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                         static_cast<socklen_t>(sizeof optval));
  if (ret < 0 && on) {
    LOG_SYSERR("SO_REUSEPORT failed.");
  }
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
               static_cast<socklen_t>(sizeof optval));
}

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

int Socket::accept(int sockfd, struct sockaddr_in6 *addr) {
  socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
  int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen,
                         SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd < 0) {
    int savedErrno = errno;
    LOG_SYSERR("Socket::accept");
    switch (savedErrno) {
    case EAGAIN:
    case ECONNABORTED:
    case EINTR:
    case EPROTO: // ???
    case EPERM:
    case EMFILE: // per-process lmit of open file desctiptor ???
      // expected errors
      errno = savedErrno;
      break;
    case EBADF:
    case EFAULT:
    case EINVAL:
    case ENFILE:
    case ENOBUFS:
    case ENOMEM:
    case ENOTSOCK:
    case EOPNOTSUPP:
      // unexpected errors
      LOG_FATAL("unexpected error of ::accept {}", savedErrno);
      break;
    default:
      LOG_FATAL("unknown error of ::accept {} ", savedErrno);
      break;
    }
  }
  return connfd;
}

ssize_t Socket::read(int sockfd, void *buf, size_t count) {
  return ::read(sockfd, buf, count);
}
ssize_t Socket::readv(int sockfd, const struct iovec *iov, int iovcnt) {
  return ::readv(sockfd, iov, iovcnt);
}

ssize_t Socket::write(int sockfd, const void *buf, size_t count) {
  return ::write(sockfd, buf, count);
}

void Socket::shutdownWrite(int sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    LOG_SYSERR("Socket::shutdownWrite");
  }
}

void Socket::toIpPort(char *buf, size_t size, const struct sockaddr *addr) {
  if (addr->sa_family == AF_INET6) {
    buf[0] = '[';
    toIp(buf + 1, size - 1, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
    uint16_t port = networkToHost16(addr6->sin6_port);
    assert(size > end);
    snprintf(buf + end, size - end, "]:%u", port);
    return;
  }
  toIp(buf, size, addr);
  size_t end = ::strlen(buf);
  const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
  uint16_t port = networkToHost16(addr4->sin_port);
  assert(size > end);
  snprintf(buf + end, size - end, ":%u", port);
}

void Socket::toIp(char *buf, size_t size, const struct sockaddr *addr) {
  if (addr->sa_family == AF_INET) {
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
  } else {
    assert(size >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
  }
}

void Socket::fromIpPort(const char *ip, u_int16_t port,
                        struct sockaddr_in *addr) {
  addr->sin_family = AF_INET;
  addr->sin_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    LOG_SYSERR("Socket::fromIpPort");
  }
}

void Socket::fromIpPort(const char *ip, u_int16_t port,
                        struct sockaddr_in6 *addr) {
  addr->sin6_family = AF_INET6;
  addr->sin6_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
    LOG_SYSERR("Socket::fromIpPort");
  }
}

const struct sockaddr *Socket::sockaddr_cast(const struct sockaddr_in *addr) {
  return static_cast<const struct sockaddr *>(static_cast<const void *>(addr));
}

const struct sockaddr *Socket::sockaddr_cast(const struct sockaddr_in6 *addr) {
  return static_cast<const struct sockaddr *>(static_cast<const void *>(addr));
}

struct sockaddr *Socket::sockaddr_cast(struct sockaddr_in6 *addr) {
  return static_cast<struct sockaddr *>(static_cast<void *>(addr));
}

const struct sockaddr_in *
Socket::sockaddr_in_cast(const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in *>(
      static_cast<const void *>(addr));
}

const struct sockaddr_in6 *
Socket::sockaddr_in6_cast(const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in6 *>(
      static_cast<const void *>(addr));
}

struct sockaddr_in6 Socket::getLocalAddr(int sockfd) {
  struct sockaddr_in6 localaddr;
  memset(&localaddr, 0, sizeof localaddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
    LOG_SYSERR("Socket::getLocalAddr");
  }
  return localaddr;
}

struct sockaddr_in6 Socket::getPeerAddr(int sockfd) {
  struct sockaddr_in6 peeraddr;
  memset(&peeraddr, 0, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getsockname(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
    LOG_SYSERR("Socket::getPeerAddr");
  }
  return peeraddr;
}

bool Socket::isSelfConnect(int sockfd) {
  struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
  struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
  if (localaddr.sin6_family == AF_INET) {
    const struct sockaddr_in *laddr4 =
        reinterpret_cast<struct sockaddr_in *>(&localaddr);
    const struct sockaddr_in *raddr4 =
        reinterpret_cast<struct sockaddr_in *>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port &&
           laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  } else if (localaddr.sin6_family == AF_INET6) {
    return localaddr.sin6_port == peeraddr.sin6_port &&
           memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr,
                  sizeof localaddr.sin6_addr) == 0;
  } else {
    return false;
  }
}
