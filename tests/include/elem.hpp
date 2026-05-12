#ifndef CLI_TEST_ELEM_HPP
#define CLI_TEST_ELEM_HPP

class Elem {
  int value_;
  bool default_constructed_;
  bool moved_;
  bool copied_;

public:
  Elem();
  Elem(int value);
  Elem(const Elem &o);
  Elem(Elem &&o);
  Elem &operator=(const Elem &o);
  Elem &operator=(Elem &&o);
  int value() const;
  bool default_constructed() const;
  bool moved() const;
  bool copied() const;
};
#endif
