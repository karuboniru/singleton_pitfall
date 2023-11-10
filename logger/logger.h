#pragma once
#include <string_view>

class logger {
public:
  static logger &get_instance();
  void putlog(std::string_view);

private:
  logger();
  ~logger();
  logger(const logger &) = delete;
  logger &operator=(const logger &) = delete;
  volatile bool am_I_alive{false};
};