#pragma once

#include <vector>
#include <unordered_map>

struct epoll_event;
class Channel;
class EventLoop;
class EpollPoller {
private:
  //epoll句柄
  int epollfd_;
  // epoll返回的就绪事件列表
  std::vector<struct epoll_event> events_;
  // fd到Channel的映射
  std::unordered_map<int, Channel *> channels_;
  //所属EventLoop
  EventLoop *eventloop_;

public:
  using ChannelList = std::vector<Channel*>;
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
};