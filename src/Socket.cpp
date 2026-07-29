#include "Socket.h"

#include <netinet/in.h>

#include "spdlog/spdlog.h"

/**
 * @brief 静态工具方法
 *
 */
int Socket::createNoblockingOrDie(sa_family_t family) {
  int sockfd =
      ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

  if (sockfd < 0) {
    spdlog::critical("Socket::createNoblockingOrDie");
    std::abort();
  }

  return sockfd;
}