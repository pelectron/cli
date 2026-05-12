#include "elem.hpp"

Elem::Elem()
  : value_{}, default_constructed_(true), moved_(false), copied_(false) {}

Elem::Elem(int value)
  : value_(value), default_constructed_(false), moved_(false), copied_(false) {}

Elem::Elem(const Elem &o)
  : value_(o.value_),
    default_constructed_(false),
    moved_(false),
    copied_(true) {}

Elem::Elem(Elem &&o)
  : value_(o.value_),
    default_constructed_(false),
    moved_(true),
    copied_(false) {}

Elem &Elem::operator=(const Elem &o) {
  value_ = o.value_;
  default_constructed_ = false;
  moved_ = false;
  copied_ = true;
  return *this;
}

Elem &Elem::operator=(Elem &&o) {
  value_ = o.value_;
  default_constructed_ = false;
  moved_ = true;
  copied_ = false;
  return *this;
}

int Elem::value() const { return value_; }

bool Elem::default_constructed() const { return default_constructed_; }

bool Elem::moved() const { return moved_; }

bool Elem::copied() const { return copied_; }
