#include "Buffer.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "InetAddr.h"
#include "TcpServer.h"
#include "util/Logger.h"

int main() {
  initLogger();
  EventLoop loop;
  InetAddr listenAddr(8888);
  TcpServer server(&loop, listenAddr, "test");
  server.setThreadNum(5);
  server.setConnectionCallback([](const TcpConnectionPtr &conn) {
    LOG_INFO("testServer [{} <-> {}] is {}", conn->peerAddr().toIpPort(),
             conn->localAddr().toIpPort(), conn->connected() ? "UP" : "DOWN");
  });
  server.setMessageCallback([](const TcpConnectionPtr &conn, Buffer *buf) {
    std::string msg(buf->retrieveAllAsString());
    LOG_INFO("{} echo {} bytes", conn->name(), msg.size());
    conn->send(msg);
  });
  server.start();
  loop.loop();
  return 0;
}