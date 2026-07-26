#include "Channel.h"
#include "EventLoop.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd) {}

Channel::~Channel() {
  //析构时不要close(fd),
}

void Channel::handleEvent() {
  if (revents_ & EPOLLHUP) {
    closeCallback_();
  }
  if (revents_ & EPOLLERR) {
    errorCallback_();
  }
  if (revents_ & kReadEvent) {
    readCallback_();
  }
  if (revents_ & kWriteEvent) {
    writeCallback_();
  }
}

void Channel::update() { loop_->updateChannel(this); }
