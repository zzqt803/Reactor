#include "EventLoopThreadPool.h"
#include "EventLoop.h"

#include <assert.h>

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr), name_(name), exiting_(false), thread_(), mutex_(),
      cond_(), callback_(cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (thread_.joinable()) {
    if (loop_) {
      loop_->quit();
    }
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  assert(!thread_.joinable());

  thread_ = std::thread(std::bind(&EventLoopThread::threadFunc, this));

  EventLoop *loop = NULL;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return loop_ != nullptr; });
    loop = loop_;
  }

  return loop;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;

  if (callback_) {
    callback_(&loop);
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }

  loop.loop();
  // assert(exiting_);
  std::unique_lock<std::mutex> lock(mutex_);
  loop_ = nullptr;
}

// EventLoopThreadPool

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop,
                                         const std::string &nameArg)
    : baseLoop_(baseLoop), name_(nameArg), started_(false), numThreads_(0),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
  assert(!started_);
  baseLoop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < numThreads_; i++) {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
    EventLoopThread *t = new EventLoopThread(cb, buf);
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());
  }

  if (numThreads_ == 0 && cb) {
    cb(baseLoop_);
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop *loop = baseLoop_;

  if (!loops_.empty()) {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (static_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashCode) {
  baseLoop_->assertInLoopThread();
  EventLoop *loop = baseLoop_;

  if (!loops_.empty()) {
    loop = loops_[hashCode % loops_.size()];
  }
  return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
  baseLoop_->assertInLoopThread();
  assert(started_);
  if (loops_.empty()) {
    return std::vector<EventLoop *>(1, baseLoop_);
  } else {
    return loops_;
  }
}
