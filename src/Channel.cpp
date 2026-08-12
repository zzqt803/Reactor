#include "Channel.h"
#include "EventLoop.h"

#include <assert.h>
#include <memory>
#include <sstream>

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false),
      eventHandling_(false), addedToLoop_(false) {}

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