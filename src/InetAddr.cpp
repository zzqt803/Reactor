#include "InetAddr.h"
#include "Socket.h"
#include "util/Logger.h"

#include <netinet/in.h>
#include <sys/socket.h>

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

InetAddr::InetAddr(uint16_t portArg, bool loopbackOnly, bool ipv6) {
  if (ipv6) {
    memset(&addr6_, 0, sizeof addr6_);
    addr6_.sin6_family = AF_INET6;
    in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
    addr6_.sin6_addr = ip;
    addr6_.sin6_port = Socket::hostToNetwork16(portArg);
  } else {
    memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
    addr_.sin_addr.s_addr = Socket::hostToNetwork32(ip);
    addr_.sin_port = Socket::hostToNetwork16(portArg);
  }
}

InetAddr::InetAddr(string ip, uint16_t portArg, bool ipv6) {
  if (ipv6 || strchr(ip.c_str(), ':')) {
    memset(&addr6_, 0, sizeof addr6_);
    Socket::fromIpPort(ip.c_str(), portArg, &addr6_);
  } else {
    memset(&addr_, 0, sizeof addr_);
    Socket::fromIpPort(ip.c_str(), portArg, &addr_);
  }
}

string InetAddr::toIpPort() const {
  char buf[64] = "";
  Socket::toIpPort(buf, sizeof buf, getSockAddr());
  return buf;
}

string InetAddr::toIp() const {
  char buf[64] = "";
  Socket::toIp(buf, sizeof buf, getSockAddr());
  return buf;
}

uint32_t InetAddr::ipv4NetEndian() const {
  assert(family() == AF_INET);
  return addr_.sin_addr.s_addr;
}

uint16_t InetAddr::port() const {
  return Socket::networkToHost16(portNetEndian());
}

const struct sockaddr *InetAddr::getSockAddr() const {
  return Socket::sockaddr_cast(&addr6_);
}