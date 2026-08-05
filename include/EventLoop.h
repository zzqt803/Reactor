#pragma once

#include <memory>
#include <vector>

class Channel;
class EpollPoller;
class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  void loop();

  void updateChannel(Channel *);
  void removeChannel(Channel *);

private:
  using ChannelList = std::vector<Channel *>;
  std::unique_ptr<EpollPoller> poller_;

  ChannelList activeChannels;
  Channel *currentActiveChannel_;
};