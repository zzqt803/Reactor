#pragma once


#include <thread>
#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread {
inline thread_local int t_cachedTid = 0;
inline void cacheTid() {
  if (t_cachedTid == 0) {
    // Linux 下 SYS_gettid 比 std::this_thread::get_id() 更加高效且直观
    t_cachedTid = static_cast<int>(::syscall(SYS_gettid));
  }
}
inline int tid() {
  // 分支预测优化,tid==0的值大概率为0
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}
} // namespace CurrentThread