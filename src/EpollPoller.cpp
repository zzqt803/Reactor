#include "EpollPoller.h"
#include "Channel.h"
#include "EventLoop.h"

#include <sys/epoll.h>
#include <unistd.h>

EpollPoller::EpollPoller(EventLoop *loop)
    : ownerLoop_(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {}

EpollPoller::~EpollPoller() { ::close(epollfd_); }

void EpollPoller::update(int operation, Channel *channel) {
  struct epoll_event ev;
  ev.events = channel->events();
  ev.data.ptr = channel;

  epoll_ctl(epollfd_, operation, channel->fd(), &ev);
}

void EpollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
  int numEvents =
      ::epoll_wait(epollfd_, &*events_.begin(), events_.size(), timeoutMs);
  if (numEvents > 0) {
    fillActiveChannels(numEvents, activeChannels);
  }
}

void EpollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const {
  for (int i = 0; i < numEvents; i++) {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    channel->setRevent(events_[i].events);

    activeChannels->push_back(channel);
  }
}

void EpollPoller::updateChannel(Channel *channel) {
  int fd = channel->fd();
  if (channels_.find(fd) != channels_.end()) {
    update(EPOLL_CTL_MOD, channel);
  } else {
    channels_[fd] = channel;
    update(EPOLL_CTL_ADD, channel);
  }
}

void EpollPoller::removeChannel(Channel *channel) {
  int fd = channel->fd();
  update(EPOLL_CTL_DEL, channel);
  channels_.erase(fd);
}