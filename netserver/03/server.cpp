#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fstream>
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

    if ((client_fd_ = ::accept(listen_fd_, (sockaddr *)&caddr, &addrlen)) ==
        -1) {
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

  //严格接收len字节
  bool recv(void *buffer, uint32_t len) {
    uint32_t total_recv = 0;
    char *ptr = static_cast<char *>(buffer);
    while (total_recv < len) {
      int rn = ::recv(client_fd_, ptr + total_recv, len - total_recv, 0);
      if (rn <= 0)
        return false; // 连接断开或出错
      total_recv += rn;
    }
    return true;
  }

  const std::string &get_client_ip() const { return clien_ip_; }

  ~CTcpServer() {
    if (listen_fd_ != -1) {
      close(listen_fd_);
    }
    if (client_fd_ != -1) {
      close(client_fd_);
    }
  }
};

struct fileinfo {
  char fileneme[256];
  uint64_t filesize;
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

  std::cout << "from: " << server.get_client_ip() << std::endl;

  //接受文件信息
  fileinfo fi;

  if (!server.recv(static_cast<void *>(&fi), sizeof(fi))) {
    perror("recv");
    return -1;
  }

  //发送确认报文
  if (!server.send("ok")) {
    perror("send");
    return -1;
  }
  //接受文件内容
  std::string filepath = "./out/" + std::string(fi.fileneme);
  std::string buffer;
  std::ofstream ofs(filepath, std::ios::out | std::ios::binary);
  uint64_t remain = fi.filesize;
  while (remain > 0) {
    uint32_t read_len = (remain < 1024) ? remain : 1024;
    if (!server.recv(buffer, read_len)) {
      std::cout << "接收内容中断" << std::endl;
      return -1;
    }
    ofs.write(buffer.data(), buffer.size());
    remain -= buffer.size();
  }

  //发送确认
  if (!server.send("ok")) {
    perror("send");
    return -1;
  }

  std::cout << "文件接受完毕：" << filepath << std::endl;
  return 0;
}