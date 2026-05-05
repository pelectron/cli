#ifndef CLI_RING_BUFFER_HPP
#define CLI_RING_BUFFER_HPP
#include <cstddef>
#include <utility>

namespace cli {

  /**
   * A view of a FIFO ring buffer, i.e. this is non owning.
   *
   * @tparam T the element type
   */
  template<typename T>
  class RingBufView {
    T *arr_{};
    // the head points to the position that was last written to
    T *head{};
    // the tail points to the position that can be read from
    T *tail{};
    std::size_t size_{};
    std::size_t capacity_{};

    constexpr T *incr(T *p) {
      ++p;
      if (p == (arr_ + capacity_))
        return arr_;
      else
        return p;
    }

  public:
    using value_type = std::remove_cv_t<T>;

    /**
     * construct a ring buffer view from an array
     *
     * @param arr start of the array
     * @param capacity the size of the array
     */
    constexpr RingBufView(T *arr, std::size_t capacity)
      : arr_(arr), head(arr_), tail(arr_), size_(0), capacity_(capacity) {}

    /**
     * check if the buffer is full
     */
    constexpr bool is_full() noexcept { return size_ == capacity_; }

    /**
     * check if the buffer is empty
     */
    constexpr bool is_empty() noexcept { return size_ == 0; }

    /**
     * returns the number of elements stored
     */
    constexpr std::size_t size() const { return size_; }

    /**
     * returns the maximum amount of elements that can be stored
     */
    constexpr std::size_t capacity() const { return capacity_; }

    /**
     * empties the buffer
     */
    constexpr void clear() {
      head = arr_;
      tail = arr_;
      size_ = 0;
    }

    /**
     * adds a new element to the buffer
     *
     * @param t the element
     * @return true if the operation succeded. If the buffer was full, false is
     * returned.
     */
    constexpr bool push_back(const value_type &t) {
      auto s = size_;
      if (s == capacity_)
        return false;
      *head = t;
      size_ = s + 1;
      head = incr(head);
      return true;
    }

    /**
     * adds a new element to the buffer
     *
     * @param t the element
     * @return true if the operation succeded. If the buffer was full, false is
     * returned.
     */
    constexpr bool push_back(value_type &&t) {
      if (size_ == capacity_)
        return false;
      *head = std::move(t);
      ++size_;
      head = incr(head);
      return true;
    }

    /**
     * removes elements from the back
     *
     * @param n the number of elements to remove
     */
    constexpr void remove_last(std::size_t n) {
      if (n > size_)
        n = size_;

      if (head - arr_ >= n) {
        head -= n;
      } else {
        n -= head - arr_;
        head = arr_ + capacity_ - n;
      }
    }

    /**
     * removes an element from the front.
     *
     * @param t where to put the value that is popped
     * @return true if a value could be popped, else false.
     */
    constexpr bool pop(value_type &t) {
      if (size_ == 0)
        return false;
      t = *tail;
      --size_;
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

  /**
   * An FIFO ring buffer.
   *
   * @tparam T the element type
   * @tparam the maximum amount of elements the buffer can hold
   */
  template<typename T, std::size_t Capacity>
  class RingBuffer : public RingBufView<T> {
    static_assert(std::is_constructible_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    T values_[Capacity]{};

  public:
    constexpr RingBuffer()
      : RingBufView<T>(values_, Capacity) {}
  };
} // namespace cli

#endif
