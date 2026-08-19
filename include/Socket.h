#pragma once

#include <cstddef>
#include <endian.h>
#include <sys/socket.h>
#include <sys/types.h>
class InetAddr;
class Socket {
public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  int fd() const { return sockfd_; }

  bool getTcpInfo(struct tcp_info *) const;

  bool getTcpInfoString(char *buf, int len) const;
  
  void bindAddress(const InetAddr &localaddr);

  void listen();

  int accept(InetAddr *peeraddr);

  void shutdownWrite();

  // Enable/disable TCP_NODELAY
  void setTcpNoDelay(bool on);

  // Enable/disable SO_REUSEADDR
  void setReuseAddr(bool on);

  // Enable/disable SO_REUSEPORT
  void setReusePort(bool on);

  // Enable/disable SO_KEEPALIVE
  void setKeepAlive(bool on);

  /**
   * @brief 静态工具方法
   *
   */
  static int createNoblockingOrDie(sa_family_t family);

  static void bindOrDie(int sockfd, const struct sockaddr *addr);

  static void listenOrDie(int sockfd);

  static void close(int sockfd);

  static int accept(int sockfd, struct sockaddr_in6 *addr);

  static ssize_t read(int sockfd, void *buf, size_t count);

  static ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);

  static ssize_t write(int sockfd, const void *buf, size_t count);

  static void shutdownWrite(int sockfd);

  static void toIpPort(char *buf, size_t size, const struct sockaddr *addr);
  static void toIp(char *buf, size_t size, const struct sockaddr *addr);

  static void fromIpPort(const char *ip, u_int16_t port,
                         struct sockaddr_in *addr);
  static void fromIpPort(const char *ip, u_int16_t port,
                         struct sockaddr_in6 *addr);

  static const struct sockaddr *sockaddr_cast(const struct sockaddr_in *addr);
  static const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
  static struct sockaddr *sockaddr_cast(struct sockaddr_in6 *addr);

  static const struct sockaddr_in *
  sockaddr_in_cast(const struct sockaddr *addr);
  static const struct sockaddr_in6 *
  sockaddr_in6_cast(const struct sockaddr *addr);

  static int getSocketError(int sockfd);

  static struct sockaddr_in6 getLocalAddr(int sockfd);
  static struct sockaddr_in6 getPeerAddr(int sockfd);
  static bool isSelfConnect(int sockfd);

  static u_int64_t hostToNetwork64(u_int64_t host64) { return htobe64(host64); }
  static u_int32_t hostToNetwork32(u_int32_t host32) { return htobe32(host32); }
  static u_int16_t hostToNetwork16(u_int16_t host16) { return htobe16(host16); }
  static u_int64_t networkToHost64(u_int64_t net64) { return be64toh(net64); }
  static u_int32_t networkToHost32(u_int32_t net32) { return be32toh(net32); }
  static u_int16_t networkToHost16(u_int16_t net16) { return be16toh(net16); }

private:
  const int sockfd_;
};