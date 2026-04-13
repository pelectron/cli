#ifndef CLI_VECTOR_HPP
#define CLI_VECTOR_HPP
#include <cstddef>
#include <memory>
#include <utility>

namespace cli {

template <typename T> class VecView {
protected:
  T *values_;
  std::size_t size_;
  std::size_t capacity_;

public:
  using value_type = T;
  using iterator = T *;
  using const_iterator = const T *;

  constexpr VecView(T *arr, std::size_t capacity)
      : values_(arr), size_(0), capacity_(capacity) {}

  constexpr bool push_back(const T &t) {
    if (size_ == capacity_)
      return false;
    values_[size_++] = t;
    return true;
  }

  constexpr bool push_back(T &&t) {
    if (size_ == capacity_)
      return false;
    values_[size_++] = std::move(t);
    return true;
  }

  constexpr void remove_last(size_t n) {
    if (size_ < n)
      size_ = 0;
    else
      size_ -= n;
  }

  constexpr void clear() {
    if constexpr (std::is_trivially_destructible_v<T>) {
      this->size_ = 0;
    } else {
      for (auto ptr = values_ + size_ - 1; ptr >= values_; --ptr) {
        ptr->~T();
      }
      this->size_ = 0;
    }
  }
  constexpr std::size_t size() const { return size_; }
  constexpr std::size_t capacity() const { return capacity_; }

  constexpr T *data() { return values_; }
  constexpr const T *data() const { return values_; }

  constexpr T *begin() { return values_; }
  constexpr const T *begin() const { return values_; }

  constexpr T *end() { return values_ + size_; }
  constexpr const T *end() const { return values_ + size_; }

  constexpr T &operator[](std::size_t i) { return values_[i]; }
  constexpr const T &operator[](std::size_t i) const { return values_[i]; }
};

template <typename T, std::size_t Capacity>
class FixedSizeVector : public VecView<T> {
  static_assert(std::is_constructible_v<T>);
  static_assert(std::is_trivially_destructible_v<T>);
  T values_[Capacity]{};

public:
  constexpr FixedSizeVector() : VecView<T>(values_, Capacity) {}

  constexpr FixedSizeVector(const FixedSizeVector &other)
      : VecView<T>(values_, Capacity) {
    for (std::size_t i = 0; i < other.size(); ++i) {
      this->push_back(other[i]);
    }
  }

  constexpr FixedSizeVector(FixedSizeVector &&other)
      : VecView<T>(values_, Capacity) {
    for (std::size_t i = 0; i < other.size(); ++i) {
      this->push_back(std::move(other[i]));
    }
    other.clear();
  }

  constexpr FixedSizeVector(std::initializer_list<T> il) : FixedSizeVector() {
    for (const auto &v : il) {
      if (this->size_ >= Capacity)
        return;
      values_[this->size_++] = v;
    }
  }

  constexpr ~FixedSizeVector() = default;

  constexpr FixedSizeVector &operator=(const FixedSizeVector &other) {
    this->clear();
    while (this->size_ < other.size_) {
      this->values_[this->size_++] = other.values_[this->size_];
    }
    return *this;
  }
  constexpr FixedSizeVector &operator=(FixedSizeVector &&other) {
    this->clear();
    while (this->size_ < other.size_) {
      this->values_[this->size_++] = std::move(other.values_[this->size_]);
    }
    other.clear();
    return *this;
  }
  constexpr bool operator==(const FixedSizeVector &other) const {
    if (this->size_ != other.size_)
      return false;

    for (std::size_t i = 0; i < this->size_; ++i) {
      if (values_[i] != other.values_[i])
        return false;
    }

    return true;
  }
};
} // namespace cli
#endif
