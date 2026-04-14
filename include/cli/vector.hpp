#ifndef CLI_VECTOR_HPP
#define CLI_VECTOR_HPP

#include "cli/concepts.hpp"

#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>

namespace cli {

/**
 * A view of a vector
 *
 * @tparam T the element type
 */
template <typename T> class VecView {
protected:
  T *values_;
  std::size_t size_;
  std::size_t capacity_;

public:
  using value_type = T;
  using iterator = T *;
  using const_iterator = const T *;

  /**
   * creates a VecView from an array
   *
   * @param arr the start of the array
   * @param capacity the array size
   */
  constexpr VecView(T *arr, std::size_t capacity)
      : values_(arr), size_(0), capacity_(capacity) {}

  /**
   * add a new element to the back
   *
   * @param t the element
   * @return true if the operation succeded, i.e. the vector is not full.
   */
  constexpr bool push_back(const T &t) {
    if (size_ == capacity_)
      return false;
    std::construct_at(values_ + size_++, t);
    return true;
  }

  /**
   * add a new element to the back
   *
   * @param t the element
   * @return true if the operation succeded, i.e. the vector is not full.
   */
  constexpr bool push_back(T &&t) {
    if (size_ == capacity_)
      return false;
    std::construct_at(values_ + size_++, std::move(t));
    return true;
  }

  /**
   * removes elements from the back
   *
   * @param n the number of elements to remove
   */
  constexpr void remove_last(size_t n) {
    if (size_ <= n)
      clear();
    else {
      auto new_size = size_ - n;
      for (auto p = values_ + size_; p > values_ + new_size; --p) {
        std::destroy_at(p);
      }
      size_ = new_size;
    }
  }

  /**
   * removes all elements from the vector
   */
  constexpr void clear() {
    if constexpr (std::is_trivially_destructible_v<T>) {
      this->size_ = 0;
    } else {
      for (auto ptr = values_ + size_ - 1; ptr >= values_; --ptr) {
        std::destroy_at(ptr);
      }
      this->size_ = 0;
    }
  }

  /**
   * returns the number of elemnts in the vector
   */
  constexpr std::size_t size() const { return size_; }

  /**
   * returns the capacity of the vector
   */
  constexpr std::size_t capacity() const { return capacity_; }

  /**
   * returns the pointer to the first element of the vector
   */
  constexpr T *data() { return values_; }

  /**
   * returns the pointer to the first element of the vector
   */
  constexpr const T *data() const { return values_; }

  /**
   * returns an iterator to the first element of the vector
   */
  constexpr T *begin() { return values_; }

  /**
   * returns an iterator to the first element of the vector
   */
  constexpr const T *begin() const { return values_; }

  /**
   * returns an iterator to the last element of the vector
   */
  constexpr T *end() { return values_ + size_; }

  /**
   * returns an iterator to the last element of the vector
   */
  constexpr const T *end() const { return values_ + size_; }

  /**
   * access the i-th element of the vector.
   * @note this does not bound check
   *
   * @param i the index
   */
  constexpr T &operator[](std::size_t i) { return values_[i]; }

  /**
   * access the i-th element of the vector.
   * @note this does not bound check
   *
   * @param i the index
   */
  constexpr const T &operator[](std::size_t i) const { return values_[i]; }
};

/**
 * A vector of T with a fixed capacity. T must be constructible without any
 * arguments.
 *
 * @tparam T the element type
 * @tparam Capacity the vectors capacity
 */
template <std::constructible_from<> T, std::size_t Capacity>
class FixedCapacityVector : public VecView<T> {
  T values_[Capacity]{};

public:
  constexpr FixedCapacityVector() : VecView<T>(values_, Capacity) {}

  constexpr FixedCapacityVector(const FixedCapacityVector &other)
      : VecView<T>(values_, Capacity) {
    for (std::size_t i = 0; i < other.size(); ++i) {
      this->push_back(other[i]);
    }
  }

  constexpr FixedCapacityVector(FixedCapacityVector &&other)
      : VecView<T>(values_, Capacity) {
    for (std::size_t i = 0; i < other.size(); ++i) {
      this->push_back(std::move(other[i]));
    }
    other.clear();
  }

  constexpr FixedCapacityVector(std::initializer_list<T> il)
      : FixedCapacityVector() {
    for (const auto &v : il) {
      if (this->size_ >= Capacity)
        return;
      values_[this->size_++] = v;
    }
  }

  constexpr ~FixedCapacityVector()
    requires TriviallyDestructible<T>
  = default;

  constexpr ~FixedCapacityVector() { this->clear(); }

  constexpr FixedCapacityVector &operator=(const FixedCapacityVector &other) {
    this->clear();
    while (this->size_ < other.size_) {
      this->values_[this->size_++] = other.values_[this->size_];
    }
    return *this;
  }

  constexpr FixedCapacityVector &operator=(FixedCapacityVector &&other) {
    this->clear();
    while (this->size_ < other.size_) {
      this->values_[this->size_++] = std::move(other.values_[this->size_]);
    }
    other.clear();
    return *this;
  }

  constexpr bool operator==(const FixedCapacityVector &other) const {
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
