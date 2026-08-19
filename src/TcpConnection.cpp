#include "TcpConnection.h"
#include "Callbacks.h"
#include "Channel.h"
#include "EventLoop.h"
#include "InetAddr.h"
#include "Socket.h"
#include "util/Logger.h"

#include <errno.h>
#include <sys/types.h>

void defaultConnectionCallback(const TcpConnectionPtr &conn) {
  LOG_TRACE("{} -> {} is {}", conn->localAddr().toIpPort(),
            conn->peerAddr().toIpPort(), conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buffer) {
  buffer->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, const string &nameArg, int sockfd,
                             const InetAddr &localAddr,
                             const InetAddr &peerAddr)
    : loop_(loop), name_(nameArg), state_(kConnecting), reading_(true),
      socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr), peerAddr_(peerAddr) {
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this)); //没有时间戳
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
  LOG_DEBUG("TcpConnection::ctor[{}] at {} fd= {}", name_, this, sockfd);
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG("TcpConnection::dtor[{}] at {} state={}", name_, this,
            stateToString());
  assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) const {
  return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const {
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof buf);
  return buf;
}

void TcpConnection::send(const void *message, int len) {
  send(string(static_cast<const char *>(message), len));
}

void TcpConnection::send(const string &message) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      void (TcpConnection::*fp)(const string &message) =
          &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp,
                                 this, // FIXME
                                 message));
    }
  }
}

void TcpConnection::send(Buffer *buf) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    } else {
      void (TcpConnection::*fp)(const string &message) =
          &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp,
                                 this, // FIXME
                                 buf->retrieveAllAsString()));
      // std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(const string &message) {
  sendInLoop(message.data(), message.size());
}

void TcpConnection::shutdown() {
  // FIXME: use compare and swap
  if (state_ == kConnected) {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) {
    // we are not writing
    socket_->shutdownWrite();
  }
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if (state_ == kDisconnected) {
    LOG_WARN("disconnected, give up writing");
    return;
  }
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = Socket::write(channel_->fd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_) {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR("TcpConnection::sendInLoop");
        if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
        {
          faultError = true;
        }
      }
    }
  }

  //没写完，丢到writeable区，监听写事件
  assert(remaining <= len);
  if (!faultError && remaining > 0) {
    outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::forceClose() {
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnecting);
    loop_->queueInLoop(
        std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop() {
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

const char *TcpConnection::stateToString() const {
  switch (state_) {
  case kDisconnected:
    return "kDisconnected";
  case kConnecting:
    return "kConnecting";
  case kConnected:
    return "kConnected";
  case kDisconnecting:
    return "kDisconnecting";
  default:
    return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

void TcpConnection::startRead() {
  loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop() {
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading()) {
    channel_->enableReading();
    reading_ = true;
  }
}

void TcpConnection::stopRead() {
  loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop() {
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading()) {
    channel_->disableReading();
    reading_ = false;
  }
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->disableAll();

    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead() {
  loop_->assertInLoopThread();
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    LOG_SYSERR("TcpConnection::handleRead");
    handleError();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = Socket::write(channel_->fd(), outputBuffer_.peek(),
                              outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_SYSERR("TcpConnection::handleWrite");
      // if (state_ == kDisconnecting)
      // {
      //   shutdownInLoop();
      // }
    }
  } else {
    LOG_TRACE("Connection fd = {} is down, no more writing", channel_->fd());
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG_TRACE("fd ={} state = {}", channel_->fd(), stateToString());
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  // must be the last line
  closeCallback_(guardThis);
}

void TcpConnection::handleError() {
  int err = Socket::getSocketError(channel_->fd());
  LOG_ERROR("TcpConnection::handleError [  {} ] - SO_ERROR = {} {}", name_, err,
            strerror(err));
}