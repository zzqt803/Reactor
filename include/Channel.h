#pragma once

#include <functional>
#include <memory>
#include <string>

class EventLoop;

class Channel {

public:
  using EventCallback = std::function<void()>;

  Channel(EventLoop *loop, int fd);
  ~Channel();

  //禁止拷贝构造和赋值，防止fd被重复触发或析构
  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;

  // epoll_wait返回事件后，EventLoop调用此函数来处理事件
  void handleEvent();

  // 注册回调函数
  void setReadCallback(EventCallback cb) { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  // 事件修改接口（改变 fd 监听状态，并通知 EventLoop 更新到 epoll 中）
  void enableReading() {
    events_ |= kReadEvent;
    update();
  }
  void disableReading() {
    events_ &= ~kReadEvent;
    update();
  }
  void enableWriting() {
    events_ |= kWriteEvent;
    update();
  }
  void disableWriting() {
    events_ &= ~kWriteEvent;
    update();
  }
  void disableAll() {
    events_ = kNoneEvent;
    update();
  }
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  //防止回调时回调函数所属类被析构
  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; }
  int events() const { return events_; }
  void setRevent(int revt) { revents_ = revt; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  // for poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  std::string reventsToString() const;
  std::string eventsToString() const;

  EventLoop *ownerLoop() const { return loop_; }
  void remove();

private:
  static std::string eventsToString(int fd,int ev);
  //调用loop的函数更新epoll状态
  void update();

  void handleEventWithGuard();

  //事件
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  //该channel所属的事件循环
  EventLoop *loop_;

  //原生文件描述符
  const int fd_;

  //关注的事件
  int events_;

  //发生的事件
  int revents_;

  //用于标记在epoll中的状态
  int index_;

  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;
  bool addedToLoop_;

  //对应事件发生时的回调函数
  EventCallback readCallback_;

  EventCallback writeCallback_;

  EventCallback closeCallback_;

  EventCallback errorCallback_;
};