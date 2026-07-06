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
  if (argc != 3) {
    cout << "Usage: ./main IP port\nExample: ./main 127.0.0.1 8888\n";
    return 0;
  }

  //第一步
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return -1;
  }

  //第二步
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(atoi(argv[2]));
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("inet_pton");
    close(sockfd);
    return -1;
  }
  if (connect(sockfd, (sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
    perror("connect");
    close(sockfd);
    return -1;
  }

  //第三步
  char buffer[1024];
  for (int i = 1; i <= 3; i++) {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer,sizeof(buffer),"第 %d 条信息",i);
    if ((send(sockfd, buffer, strlen(buffer), 0)) <= 0) {
      perror("send");
      break;
    }
    cout<<"send: "<<buffer<<endl;

    memset(buffer, 0, sizeof(buffer));
    int rn = recv(sockfd, buffer, sizeof(buffer), 0);
    if (rn <= 0) {
      break;
    }
    cout<<"recv: "<<rn<<" bytes"<<endl;
  }

  //第四步
  close(sockfd);
}