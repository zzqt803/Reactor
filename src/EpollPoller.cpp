#include "EpollPoller.h"
#include "Channel.h"
#include "EventLoop.h"
#include "util/Logger.h"

#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>

namespace {
const int kNew = -1;    //不在channel，没有加入epoll
const int kAdded = 1;   //在channel中，也在epoll
const int kDeleted = 2; //在channel列表中，不在epoll中
} // namespace

EpollPoller::EpollPoller(EventLoop *loop)
    : ownerLoop_(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    LOG_SYSFATAL("EPollPoller::EPollPoller");
  }
}

EpollPoller::~EpollPoller() { ::close(epollfd_); }

void EpollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
  LOG_TRACE("fd total count {}", channels_.size());
  int numEvents =
      ::epoll_wait(epollfd_, &*events_.begin(), events_.size(), timeoutMs);
  int saveErrno = errno;
  if (numEvents > 0) {
    LOG_TRACE("{} events happened", numEvents);
    fillActiveChannels(numEvents, activeChannels);

    //动态扩容
    if (static_cast<size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents == 0) {
    LOG_TRACE("nothing happened");
  } else {
    if (saveErrno != EINTR) { //忽略系统中断信号引发的错误
      errno = saveErrno;
      LOG_SYSERR("EPollPoller::poll()");
    }
  }
}

void EpollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const {
  assert(static_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; i++) {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    channel->setRevent(events_[i].events);

    activeChannels->push_back(channel);
  }
}

//更新fd在epoll中的状态，会从epoll中删除，但channel列表保留
void EpollPoller::updateChannel(Channel *channel) {
  assertInLoopThread();
  const int index = channel->index();
  LOG_TRACE("fd = {} index= {} events = {}", channel->fd(), channel->events(),
            index);
  // 说明当前它不在epoll 内核监听列表中
  if (index == kNew || index == kDeleted) {
    int fd = channel->fd();
    if (index == kNew) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    } else // index == kDeleted
    {
      // 曾经删过：它应该已经在 map 里了
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    // 标记状态为：已加入 epoll
    channel->set_index(kAdded);
    // 向内核发起 syscall: epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, event)
    update(EPOLL_CTL_ADD, channel);
  } else {
    // index == kAdded，说明它已经在epoll 内核监听列表中了
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);

    // 如果用户把这个 Channel 的事件全部关掉了（比如 disableAll()）
    if (channel->isNoneEvent()) {
      // 从内核 epoll 中暂时注销它
      update(EPOLL_CTL_DEL, channel);
      // 标记状态为：已注销/已删除
      channel->set_index(kDeleted);
    } else {
      // 只是修改了关注的事件（例如从只读改为可读可写）
      // 向内核发起 syscall: epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, event)
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

//从epoll和channel列表中一起删除
void EpollPoller::removeChannel(Channel *channel) {
  assertInLoopThread();
  int fd = channel->fd();
  LOG_TRACE("fd = {}", fd);
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->isNoneEvent());
  int index = channel->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);
  if (index == kAdded) {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

bool EpollPoller::hasChannel(Channel *channel) const {
  assertInLoopThread();
  auto it = channels_.find(channel->fd());
  return it!=channels_.end() && it->second == channel;
}

void EpollPoller::update(int operation, Channel *channel) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof ev);
  ev.events = channel->events();
  ev.data.ptr = channel;

  int fd = channel->fd();
  LOG_TRACE("epoll_ctl op = {} fd = {} event = {}",
            operationToString(operation), fd, channel->eventsToString());
  if (::epoll_ctl(epollfd_, operation, fd, &ev) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_SYSERR("epoll_ctl op = {} fd = {}", operationToString(operation), fd);
    } else {
      LOG_SYSFATAL("epoll_ctl op = {} fd = {}", operationToString(operation),
                   fd);
    }
  }
}

const char *EpollPoller::operationToString(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}