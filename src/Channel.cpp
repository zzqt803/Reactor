#include "Channel.h"
#include "EventLoop.h"

#include <epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;


Channel::Channel(EventLoop *loop, int fd) : loop(loop), fd_(fd) {}

Channel::~Channel() {
  //析构时不要close(fd),
}

void Channel::handleEvent() {
  if (revents & EPOLLHUP) {
    closeCallback_();
  }
  if (revents & EPOLLERR) {
    errorCallback_();
  }
  if (revents & kReadEvent) {
    readCallback_();
  }
  if (revents & kWriteEvent) {
    writeCallback_();
  }
}

void update() { loop_->updateChannel(this); }

