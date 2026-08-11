#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoop;

using ThreadInitCallback = std::function<void(EventLoop *)>;

class EventLoopThread {
public:
  //禁止copy
  EventLoopThread(const EventLoopThread &) = delete;
  EventLoopThread &operator=(const EventLoopThread &) = delete;

  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string &name = std::string());
  ~EventLoopThread();
  EventLoop *startLoop();

private:
  void threadFunc();

  EventLoop *loop_;
  bool exiting_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};

class EventLoopThreadPool {
public:

  //禁止copy
  EventLoopThreadPool(const EventLoopThreadPool &) = delete;
  EventLoopThreadPool &operator=(const EventLoopThreadPool &) = delete;

  EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  //轮转
  EventLoop *getNextLoop();

  //通过hash获取相同loop
  EventLoop *getLoopForHash();

  std::vector<EventLoop *> getAllLoops();

  bool started() const { return started_; };

  const std::string &name() const { return name_; }

private:
  EventLoop *baseLoop_;
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};