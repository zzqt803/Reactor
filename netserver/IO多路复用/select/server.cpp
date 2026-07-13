#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
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

  int accept() {
    sockaddr_in caddr;
    socklen_t addrlen = sizeof(caddr);

    if ((client_fd_ = ::accept(listen_fd_, (sockaddr *)&caddr, &addrlen)) ==
        -1) {
      return -1;
    }

    return client_fd_;
  }

  bool send(const std::string &buffer) {
    if (::send(client_fd_, buffer.data(), buffer.size(), 0) <= 0) {
      return false;
    }

    return true;
  }

  int recv(std::string &buffer, uint32_t maxlen) {
    buffer.clear();
    buffer.resize(maxlen);
    int rn = ::recv(client_fd_, &buffer[0], buffer.size(), 0);
    if (rn <= 0) {
      return rn;
    }
    buffer.resize(rn);

    return rn;
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

void set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: ./main port\nExample: ./main 8888\n";
    return 0;
  }

  CTcpServer server;
  server.init(atoi(argv[1]));

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(server.listen_fd_, &readfds);

  int max_fd = server.listen_fd_;
  timeval tv = {100, 0};
  char buffer[1024];

  while (1) {
    fd_set tmp = readfds;
    int ret = select(max_fd + 1, &tmp, NULL, NULL, &tv);

    if (ret <0) {
      perror("select");
      return -1;
    }

    if (ret == 0) {
      std::cout << "time out" << std::endl;
      continue;
    }

    for (int i = 0; i <= max_fd; i++) {
      if (FD_ISSET(i, &tmp) == 0) {
        continue;
      }

      if (i == server.listen_fd_) {

        int new_fd = server.accept();
        set_nonblock(new_fd);
        FD_SET(new_fd, &readfds);
        if (max_fd < new_fd) {
          max_fd = new_fd;
        }
        std::cout<<"socket "<<new_fd<<" connect"<<std::endl;
      } else {
        std::string str;
        std::cout<<"event "<<i<<" :";
        while (1) {
          int n = ::recv(i, buffer, sizeof(buffer)-1, 0);
          if (n < 0) {
            if (errno == EAGAIN)
              break; // 缓冲区读完，正常退出循环
            else {
              // 真实错误，关闭连接
              close(i);
              FD_CLR(i, &readfds);
              break;
            }
          }
          else if (n == 0) {
            close(i);
            FD_CLR(i, &readfds);
            break;
          }

          buffer[n] = '\0';
          str += buffer;
        }
        ::send(i,str.data(),str.size(),0);
        std::cout<<str;
        std::cout<<std::endl;
      }
    }
  }
}