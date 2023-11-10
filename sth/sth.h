#pragma once

class sth {
public:
  static sth &get_instance();
  void do_something();

private:
  sth();
  ~sth();
  sth(const sth &) = delete;
  sth &operator=(const sth &) = delete;
};