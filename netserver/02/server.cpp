#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Usage: ./main port\nExample: ./main 8888\n";
    return 0;
  }

  //第一步
  int listensock = socket(AF_INET, SOCK_STREAM, 0);
  if (listensock == -1) {
    perror("socket");
    return -1;
  }

  //第二步，绑定
  sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(atoi(argv[1]));
  if ((bind(listensock, (sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
    perror("bind");
    close(listensock);
    return -1;
  }

  //第三步，监听
  if ((listen(listensock, 5)) == -1) {
    perror("listen");
    close(listensock);
    return -1;
  }

  //第四步，接受
  int clientsock = accept(listensock, 0, 0);
  if (clientsock == -1) {
    perror("accept");
    close(listensock);
    return -1;
  }

  //第五步，通信
  char buffer[1024];
  while (true) {
    memset(buffer, 0, sizeof(buffer));
    int rn = recv(clientsock, buffer, sizeof(buffer) - 1, 0);
    if (rn <= 0) {
      break;
    }
    buffer[rn] = '\0';
    cout << "recv: " << buffer << endl;

    strcpy(buffer, "ok");
    if (send(clientsock, buffer, strlen(buffer), 0) <= 0) {
      perror("send");
      break;
    }
    cout << "send: " << buffer << endl;
  }

  //第六步，关闭
  close(listensock);
  close(clientsock);
}