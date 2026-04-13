#ifndef CLI_RING_BUFFER_HPP
#define CLI_RING_BUFFER_HPP
#include <cstddef>
#include <utility>

namespace cli {

template <typename T> class RingBufView {
  T *arr_;
  std::size_t capacity_;
  // the head points to the position that was last written to
  T *head;
  // the tail points to the position that can be read from
  T *tail;
  std::size_t size_;

  constexpr T *incr(T *p) {
    ++p;
    if (p == (arr_ + capacity_))
      return arr_;
    else
      return p;
  }

public:
  using value_type = std::remove_cv_t<T>;

  constexpr RingBufView(T *arr, std::size_t capacity)
      : arr_(arr), capacity_(capacity), head(arr_), tail(arr_), size_(0) {}

  constexpr bool is_full() noexcept { return size_ == capacity_; }
  constexpr bool is_empty() noexcept { return size_ == 0; }

  constexpr std::size_t size() const { return size_; }
  constexpr std::size_t capacity() const { return capacity_; }

  constexpr void clear() {
    head = arr_;
    tail = arr_;
    size_ = 0;
  }

  constexpr bool push_back(const value_type &t) {
    auto s = size_;
    if (s == capacity_)
      return false;
    *head = t;
    size_ = s + 1;
    head = incr(head);
    return true;
  }

  constexpr bool push_back(value_type &&t) {
    auto s = size_;
    if (s == capacity_)
      return false;
    *head = std::move(t);
    size_ = s + 1;
    head = incr(head);
    return true;
  }

  constexpr void remove_last(std::size_t n) {
    auto s = size_;
    if (n > s)
      n = s;

    if (head - arr_ >= n) {
      head -= n;
    } else {
      n -= head - arr_;
      head = arr_ + capacity_ - n;
    }
  }

  constexpr bool pop(value_type &t) {
    auto s = size_;
    if (s == 0)
      return false;
    t = *tail;
    size_ = s - 1;
    tail = incr(tail);
    return true;
  }

  struct write_iterator {
    RingBufView *owner;
    T t{};
    constexpr T &operator*() { return t; }
    constexpr write_iterator &operator++() {
      owner->push_back(t);
      t = {};
      return *this;
    }
  };

  constexpr write_iterator output() { return {this, 0}; }
};

template <typename T, std::size_t Capacity>
class RingBuffer : public RingBufView<T> {
  static_assert(std::is_constructible_v<T>);
  static_assert(std::is_trivially_destructible_v<T>);
  T values_[Capacity]{};

public:
  constexpr RingBuffer() : RingBufView<T>(values_, Capacity) {}
};
} // namespace cli

#endif
