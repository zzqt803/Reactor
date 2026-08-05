#pragma once

#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

using std::string;
class InetAddr {
public:
  ~InetAddr() = default;

  explicit InetAddr(uint16_t port = 0, bool looopbackonly = false,
                    bool ipv6 = false);

  InetAddr(string ip, uint16_t port, bool ipv6 = false);

  explicit InetAddr(const struct sockaddr_in &addr) : addr_(addr) {}

  explicit InetAddr(const struct sockaddr_in6 &addr) : addr6_(addr) {}

  sa_family_t family() const { return addr_.sin_family; }
  string toIp() const;
  string toIpPort() const;
  uint16_t port() const;

private:
  struct sockaddr_in addr_;
  struct sockaddr_in6 addr6_;

};