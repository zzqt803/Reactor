#include "Channel.h"
#include "EventLoop.h"
#include "util/Logger.h"

#include <assert.h>
#include <memory>
#include <sstream>

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false),
      eventHandling_(false), addedToLoop_(false),logHup_(true) {}

Channel::~Channel() {
  //析构时不要close(fd),fd由Socket类进行RAII管理
  assert(!eventHandling_);
  assert(!addedToLoop_);
  if (loop_->isInLoopThread()) {
    assert(!loop_->hasChannel(this));
  }
}

void Channel::tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::handleEvent() {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      handleEventWithGuard();
    }
  } else {
    handleEventWithGuard();
  }
}

void Channel::handleEventWithGuard() {
  eventHandling_ = true;
  LOG_TRACE("{}", reventsToString());
  // 1. 对端挂断 (EPOLLHUP) 且当前缓冲区内没有数据可读 (EPOLLIN)
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    if (logHup_) {
      LOG_WARN("fd = {} Channel::handle_event() EPOLLHUP",fd_);
    }
    if (closeCallback_)
      closeCallback_();
  }

  // 2. 错误事件 (EPOLLERR)
  if (revents_ & EPOLLERR) {
    if (errorCallback_)
      errorCallback_();
  }

  // 3. 可读事件：包含普通数据 (EPOLLIN)、带外数据 (EPOLLPRI) 以及对端关闭半连接
  // (EPOLLRDHUP)
  if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    if (readCallback_)
      readCallback_();
  }

  // 4. 可写事件 (EPOLLOUT)
  if (revents_ & EPOLLOUT) {
    if (writeCallback_)
      writeCallback_();
  }

  eventHandling_ = false;
}

void Channel::remove() {
  assert(isNoneEvent());
  addedToLoop_ = false;
  loop_->removeChannel(this);
}

void Channel::update() {
  addedToLoop_ = true;
  loop_->updateChannel(this);
}

std::string Channel::reventsToString() const {
  return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString() const {
  return eventsToString(fd_, events_);
}

std::string Channel::eventsToString(int fd, int ev) {
  std::ostringstream oss;
  oss << fd << ": ";

  if (ev & EPOLLIN)
    oss << "IN ";
  if (ev & EPOLLPRI)
    oss << "PRI ";
  if (ev & EPOLLOUT)
    oss << "OUT ";
  if (ev & EPOLLHUP)
    oss << "HUP ";
  if (ev & EPOLLRDHUP)
    oss << "RDHUP ";
  if (ev & EPOLLERR)
    oss << "ERR ";
  if (ev & EPOLLET)
    oss << "ET "; // 边缘触发 (Edge Triggered)
  if (ev & EPOLLONESHOT)
    oss << "ONESHOT "; // 单次触发

  return oss.str();
}