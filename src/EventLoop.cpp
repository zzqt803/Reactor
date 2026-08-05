#include "EventLoop.h"
#include "Channel.h"
#include "EpollPoller.h"

EventLoop::EventLoop() { poller_ = std::make_unique<EpollPoller>(this); }

EventLoop::~EventLoop() {}

void EventLoop::loop() {
  while (1) {
    activeChannels.clear();
    poller_->poll(10000, &activeChannels);

    for (auto channel : activeChannels) {
      currentActiveChannel_ = channel;
      currentActiveChannel_->handleEvent();
    }
  }
}

void EventLoop::updateChannel(Channel *channel) {
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  poller_->removeChannel(channel);
}