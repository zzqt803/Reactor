#pragma once

#include <vector>
#include <algorithm>

using std::size_t;
using std::vector;
class Buffer {
public:
  Buffer() = default;
  ~Buffer() = default;
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend) {}

  void swap(Buffer &rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }

  size_t readableBytes() const { return writerIndex_ - readerIndex_; }

  size_t writeableBytes() const { return buffer_.size() - writerIndex_; }

  size_t prependableBytes() const {
    return readerIndex_;
  }

      private : vector<char> buffer_;
  //可读数据起始位置
  size_t readerIndex_;
  //可写数据起始位置
  size_t writerIndex_;
};