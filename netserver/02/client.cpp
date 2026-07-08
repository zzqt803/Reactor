#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

class CTcpClient {
public:
  int socket_fd_{-1};
  std::string ip_address_{};
  uint16_t port_{};

public:
  CTcpClient() : socket_fd_(-1) {}
  bool connect(const std::string &ip_address, uint16_t port) {
    if (socket_fd_ != -1) {
      return false;
    }
    ip_address_ = ip_address;
    port_ = port;
    //创建socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
      return false;
    }

    //连接
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port_);
    if (inet_pton(AF_INET, ip_address_.data(), &servaddr.sin_addr) <= 0) {
      close(socket_fd_);
      socket_fd_ = -1;
      return false;
    }
    if (::connect(socket_fd_, (sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
      close(socket_fd_);
      socket_fd_ = -1;
      return false;
    }

    return true;
  }
};

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: ./main IP port\nExample: ./main 127.0.0.1 8888\n";
    return 0;
  }

  CTcpClient client;
  client.connect(argv[1], atoi(argv[2]));

  //第三步
  char buffer[1024];
  for (int i = 1; i <= 3; i++) {
    snprintf(buffer, sizeof(buffer), "第 %d 条信息", i);
    if ((send(client.socket_fd_, buffer, strlen(buffer), 0)) <= 0) {
      perror("send");
      break;
    }
    std::cout << "send: " << buffer << std::endl;

    //使用size-1,为'\0'留出空间
    int rn = recv(client.socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (rn <= 0) {
      if (rn == 0)
        std::cout << "Server closed connection." << std::endl;
      else
        perror("recv");
      break;
    }
    buffer[rn] = '\0';
    std::cout << "recv: " << buffer << std::endl;
  }

  //第四步
  close(client.socket_fd_);
  return 0;
}