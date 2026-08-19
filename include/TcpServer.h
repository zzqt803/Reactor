#pragma once

#include "InetAddr.h"
#include "TcpConnection.h"

#include <atomic>
#include <map>

class Acceptor;
class Eventloop;
class EventLoopThreadPool;

class TcpServer {
public:
  typedef std::function<void(EventLoop *)> ThreadInitCallback;
  enum Option { kNoReusePort, kReusePort };

  //禁止copy
  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;

  TcpServer(EventLoop *loop, const InetAddr &listenAddr, const string &nameArg,
            Option option = kNoReusePort);
  ~TcpServer();

  const string &ipPort() const { return ipPort_; }
  const string &name() const { return name_; }
  EventLoop *getLoop() const { return loop_; }

  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback &cb) {
    threadInitCallback_ = cb;
  }
  std::shared_ptr<EventLoopThreadPool> threadPool() { return threadPool_; }

  void start();

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

private:
  //给Acceptor使用
  void newConnection(int sockfd, const InetAddr &peerAddr);
  //给TcpConnection的close使用
  void removeConnection(const TcpConnectionPtr &conn);
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

  using ConnectionMap = std::map<string, TcpConnectionPtr>;

  EventLoop *loop_;
  const string ipPort_;
  const string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> threadPool_;

  //
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  ThreadInitCallback threadInitCallback_;

  std::atomic<int> started_{0};
  int nextConnId_;
  ConnectionMap connections_;
};