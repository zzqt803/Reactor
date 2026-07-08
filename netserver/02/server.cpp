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


class CTcpServer {
public:
  int listen_fd_{-1};
  int client_fd_{-1};
  uint16_t port_{};
  std::string clien_ip_{};

public:
  CTcpServer() : listen_fd_(-1), client_fd_(-1){};

  bool init(uint16_t port) {
    port_ = port;

    if ((listen_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      return false;
    }

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_);

    if ((bind(listen_fd_, (sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
      close(listen_fd_);
      listen_fd_ = -1;
      return false;
    }

    if ((listen(listen_fd_, 5)) == -1) {
      close(listen_fd_);
      listen_fd_ = -1;
      return false;
    }

    return true;
  }


  bool accept() {
    sockaddr_in caddr;
    socklen_t addrlen = sizeof(caddr);

    if ((client_fd_ = ::accept(listen_fd_, (sockaddr*)&caddr, &addrlen)) == -1) {
      return false;
    }

    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(caddr.sin_family, &caddr.sin_addr, ipstr, sizeof(ipstr));
    clien_ip_ = ipstr;
    
    return true;
  }

  bool send(const std::string &buffer) {
    if (::send(client_fd_, buffer.data(), buffer.size(), 0) <= 0) {
      return false;
    }

    return true;
  }

  bool recv(std::string &buffer, uint32_t maxlen) {
    buffer.clear();
    buffer.resize(maxlen);
    int rn = ::recv(client_fd_, &buffer[0], buffer.size(), 0);
    if (rn <= 0) {
      return false;
    }
    buffer.resize(rn);

    return true;
  }

  const std::string &get_client_ip() const {
    return clien_ip_;    
  }

  ~CTcpServer() {
    if (listen_fd_ != -1) {
      close(listen_fd_);
    }
    if (client_fd_ != -1) {
      close(client_fd_);
    }
  }
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: ./main port\nExample: ./main 8888\n";
    return 0;
  }

  CTcpServer server;
  server.init(atoi(argv[1]));
  if (!server.accept()) {
    perror("accept");
    return -1;
  }

  std::cout<<"from: "<<server.get_client_ip()<<std::endl;

  std::string buffer;
  while (true) {
    if (!server.recv(buffer, 1024)) {
      perror("recv");
      break;
    }
    std::cout << "recv: " << buffer << std::endl;

    buffer = "ok";
    if (!server.send(buffer)) {
      perror("send");
      break;
    }
    std::cout << "send: " << buffer << std::endl;
  }

  return 0;
}