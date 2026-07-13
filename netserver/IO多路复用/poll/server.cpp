#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/poll.h>
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

constexpr int MAX_FDS = 1024;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: ./main port\nExample: ./main 8888\n";
    return 0;
  }

  CTcpServer server;
  server.init(atoi(argv[1]));

  struct pollfd fds[MAX_FDS];
  for (int i = 0; i < MAX_FDS; i++) {
    fds[i].fd = -1;
  }

  fds[server.listen_fd_].fd = server.listen_fd_;
  fds[server.listen_fd_].events = POLLIN;

  int max_fd = server.listen_fd_;

  char buffer[1024];
  while (1) {
    int ret = poll(fds,max_fd+1,10000);

    if (ret < 0) {
      perror("poll");
      return -1;
    }

    if (ret == 0) {
      std::cout << "time out" << std::endl;
      continue;
    }

    for (int i = 0; i <= max_fd; i++) {
      if (fds[i].fd<0) {
        continue;
      }

      if (fds[i].revents & POLLIN) {
        if (i == server.listen_fd_) {

          int new_fd = server.accept();
          set_nonblock(new_fd);
          fds[new_fd].fd = new_fd;
          fds[new_fd].events = POLLIN;
          if (max_fd < new_fd) {
            max_fd = new_fd;
          }
          std::cout << "socket " << new_fd << " connect" << std::endl;
        } else {
          std::string str;
          std::cout << "event " << i << " :";
          while (1) {
            int n = ::recv(i, buffer, sizeof(buffer) - 1, 0);
            if (n < 0) {
              if (errno == EAGAIN)
                break; // 缓冲区读完，正常退出循环
              else {
                // 真实错误，关闭连接
                close(i);
                fds[i].fd = -1;
                if (i == max_fd) {
                  for (int j = max_fd; j >= 0; j--) {
                    if (fds[j].fd != -1)
                      max_fd = j;
                  }
                }
                break;
              }
            } else if (n == 0) {
              close(i);
              fds[i].fd = -1;
              if (i == max_fd) {
                for (int j = max_fd; j >= 0; j--) {
                  if(fds[j].fd!=-1) max_fd = j;
                }
              }
              break;
            }

            buffer[n] = '\0';
            str += buffer;
          }
          ::send(i, str.data(), str.size(), 0);
          std::cout << str;
          std::cout << std::endl;
        }
      }
    }
  }
}