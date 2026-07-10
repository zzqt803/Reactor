#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>

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

  bool send(const std::string &buffer) {
    if (socket_fd_ == -1) {
      return false;
    }

    if (::send(socket_fd_, buffer.data(), buffer.size(), 0) <= 0) {
      return false;
    }

    return true;
  }

  bool send(void* buffer,uint16_t buffersize) {
    if (socket_fd_ == -1) {
      return false;
    }

    if (::send(socket_fd_, buffer, buffersize, 0) <= 0) {
      return false;
    }

    return true;
  }

  bool recv(std::string &buffer, uint32_t maxlen) {
    if (socket_fd_ == -1) {
      return false;
    }

    buffer.clear();
    buffer.resize(maxlen);
    int rn = ::recv(socket_fd_, &buffer[0], buffer.size(), 0);
    if (rn <= 0) {
      return false;
    }
    buffer.resize(rn);

    return true;
  }

  ~CTcpClient() {
    if (socket_fd_ == -1) {
      return;
    }
    close(socket_fd_);
  }
};

struct fileinfo {
  char fileneme[256];
  uint64_t filesize;
};

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "Usage: ./main IP port 文件名\nExample: ./main 127.0.0.1 8888 test.txt\n";
    return 0;
  }

  CTcpClient client;
  client.connect(argv[1], atoi(argv[2]));

  //发送文件名和文件大小
  std::ifstream ifs(argv[3], std::ios::in | std::ios::binary);
  if (!ifs.is_open()) {
    std::cout << "文件打开失败：" << argv[3] << std::endl;
    return -1;
  }

  ifs.seekg(0, std::ios::end);
  auto len = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  fileinfo fi;
  strcpy(fi.fileneme, argv[3]);
  fi.filesize = len;

  if (!client.send(&fi, sizeof(fi))) {
    perror("send");
    return -1;
  }

  //等待服务端确认
  std::string res;
  if (!client.recv(res, 1024)) {
    perror("recv");
    return -1;
  }
  if (res != "ok") {
    std::cout << "文件信息发送失败" << std::endl;
    return -1;
  }

  //发送文件内容
  int remain = len;
  int bufferlen = 512; //一次发送512字节
  std::string buffer(bufferlen, 0);
  while (remain>0) {
    uint32_t chunk = (remain < bufferlen) ? remain : bufferlen;
    ifs.read(&buffer[0], chunk);

    // 强制将 string 裁剪到实际读取的字节大小
    buffer.resize(chunk);

    if (!client.send(buffer)) {
      perror("send");
      return -1;
    }
    remain -= chunk;
  }

  //等待服务端确认
  res.clear();
  if (!client.recv(res, 1024)) {
    perror("recv");
    return -1;
  }
  if (res != "ok") {
    std::cout << "文件内容发送失败" << std::endl;
    return -1;
  }

  std::cout<<"文件发送成功\n";
  return 0;
}