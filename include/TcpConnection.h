#pragma once

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddr.h"
#include <memory>

class Channel;
class Socket;
class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(EventLoop *loop, const string &name, int sockfd,
                const InetAddr &localAddr, const InetAddr &peerAddr);
  ~TcpConnection();

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  EventLoop *getLoop() const { return loop_; }
  const string &name() const { return name_; };
  const InetAddr &localAddr() const { return localAddr_; }
  const InetAddr &peerAddr() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }
  bool disconnected() const { return state_ == kDisconnected; }
  bool getTcpInfo(struct tcp_info *) const;
  string getTcpInfoString() const;

  void send(const void *message, int len);
  void send(const string &message);
  void send(Buffer *message);
  void shutdown();
  void forceClose();
  // void forseCloseWithDelay();
  void setTcpNoDelay(bool on);
  void startRead();
  void stopRead();
  bool isReading() const { return reading_; }

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }
  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

  void connectEstablished();
  void connectDestroyed();

private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void handleRead();
  void handleWrite();
  void handleClose();
  void handleError();
  void sendInLoop(const string &);
  void sendInLoop(const void *message, size_t len);
  void shutdownInLoop();
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char *stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop *loop_;
  string name_;
  StateE state_;
  bool reading_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddr localAddr_;
  const InetAddr peerAddr_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  CloseCallback closeCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  Buffer inputBuffer_;
  Buffer outputBuffer_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;