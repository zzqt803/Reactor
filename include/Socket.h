#pragma once

#include <sys/socket.h>
class InetAddr;
class Socket {
public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  int fd() const { return sockfd_; }

  void bindAddress(const InetAddr &localaddr);

  void listen();

  int accept();

  void shutdownWrite();

  // Enable/disable TCP_NODELAY
  void setTcpNoDelay(bool on);
  
  // Enable/disable SO_REUSEADDR
  void setReuseAddr(bool on);

  //Enable/disable SO_REUSEPORT
  void setReusePort(bool on);
  
  // Enable/disable SO_KEEPALIVE
  void setKeepAlive(bool on);

  /**
   * @brief 静态工具方法
   *
   */
  static int createNoblockingOrDie(sa_family_t family);

  
private:
  const int sockfd_;
};