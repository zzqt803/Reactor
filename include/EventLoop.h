#pragma once

#include "util/CurrentThread.h"

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

class Channel;
class EpollPoller;
class EventLoop {
public:
  using Functor = std::function<void()>;
  EventLoop();
  ~EventLoop();

  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  void loop();

  void quit();

  void runInLoop(Functor cb);
  void queueInLoop(Functor cb);

  void wakeup();
  void updateChannel(Channel *);
  void removeChannel(Channel *);

  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); };
  bool eventHandling() const { return eventHandling_; }
  static EventLoop *getEventLoopOfCurrentThread();

private:
  void abortNotInLoopThread();
  void handleRead(); // waked up
  void doPendingFunctors();

  using ChannelList = std::vector<Channel *>;

  bool looping_;
  std::atomic<bool> quit_;
  bool eventHandling_;
  bool callingPendingFunctors_;
  int64_t iteration_;
  const pid_t threadId_;
  std::unique_ptr<EpollPoller> poller_;
  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;

  ChannelList activeChannels_;
  Channel *currentActiveChannel_;

  std::mutex mutex_;
  std::vector<Functor> pendingFunctors_;
};