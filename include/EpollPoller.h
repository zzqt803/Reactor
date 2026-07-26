#pragma once

#include <unordered_map>
#include <vector>

struct epoll_event;
class Channel;
class EventLoop;
class EpollPoller {
public:
  EpollPoller(EventLoop *loop);
  ~EpollPoller();

  EpollPoller(const EpollPoller &) = delete;
  EpollPoller &operator=(const EpollPoller &) = delete;

  using ChannelList = std::vector<Channel *>;
  //进行epoll_wait，返回活跃连接
  void poll(int timeoutMs, ChannelList *activeChannels);
  //更新监听事件
  void updateChannel(Channel *channel);
  //从epoll中移除该连接
  void removeChannel(Channel *channel);

private:
  //将内核事件更新到列表中
  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
  //封装epoll_ctl
  void update(int operation, Channel *channel);

  static const int kInitEventListSize = 16;

  //所属EventLoop
  EventLoop *ownerLoop_;

  // epoll句柄
  int epollfd_;
  // epoll返回的就绪事件列表
  std::vector<struct epoll_event> events_;
  // fd到Channel的映射
  std::unordered_map<int, Channel *> channels_;
};