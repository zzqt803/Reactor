#include "EventLoop.h"
#include "Channel.h"
#include "EpollPoller.h"
#include "Socket.h"

#include "sys/eventfd.h"
#include "util/Logger.h"
#include <mutex>
#include <sys/types.h>

//创建一个事件描述符，用来跨线程唤醒线程
namespace {
thread_local EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

int createEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); //非阻塞，执行时关闭
  if (evtfd < 0) {
    LOG_SYSFATAL("Failed in eventfd");
  }
  return evtfd;
}
} // namespace

EventLoop *EventLoop::getEventLoopOfCurrentThread() {
  return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), eventHandling_(false),
      callingPendingFunctors_(false), iteration_(0),
      threadId_(CurrentThread::tid()),
      poller_(std::make_unique<EpollPoller>(this)), wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr) {
  LOG_DEBUG("EventLoop created {0} in thread {1}", (void *)this, threadId_);
  if (t_loopInThisThread) {
    LOG_FATAL("Another EventLoop {0} exsits in this thread {1}",
              (void *)t_loopInThisThread, threadId_);
  } else {
    t_loopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  LOG_DEBUG("EventLoop {0} of thread {1} destructs in thread {2}", (void *)this,
            threadId_, CurrentThread::tid());
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  while (!quit_) {
    activeChannels_.clear();
    poller_->poll(kPollTimeMs, &activeChannels_);
    ++iteration_;

    eventHandling_ = true;
    for (auto channel : activeChannels_) {
      currentActiveChannel_ = channel;
      currentActiveChannel_->handleEvent();
    }
    currentActiveChannel_ = nullptr;
    eventHandling_ = false;

    doPendingFunctors();
  }

  LOG_TRACE("EventLoop {} stop looping", (void *)this);
  looping_ = false;
}

void EventLoop::quit() {
  quit_.store(true);
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingFunctors_.push_back(std::move(cb));
  }

  if (!isInLoopThread() || callingPendingFunctors_) {
    wakeup();
  }
}

void EventLoop::updateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_) {
    assert(currentActiveChannel_ == channel ||
           std::find(activeChannels_.begin(), activeChannels_.end(), channel) ==
               activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL("EventLoop::abortNotInLoopThread - EventLoop {} was created in "
            "thead {}, curent thread id= {}",
            (void *)this, threadId_, CurrentThread::tid());
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = Socket::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR("EventLoop::wakeup() writes {} bytes instead of 8", n);
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = Socket::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR("EventLoop::handleRead() reads {} bytes instead of 8", n);
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Functor &functor : functors) {
    functor();
  }
  callingPendingFunctors_ = false;
}