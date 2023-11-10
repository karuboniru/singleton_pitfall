#include "logger.h"
#include <iostream>

logger::logger() {
  std::cerr << "logger::logger() you can start putting logs now" << std::endl;
  am_I_alive = true;
}

logger::~logger() {
  std::cerr << "logger::~logger() you should not being putting logs anymore"
            << std::endl;
  am_I_alive = false;
}

logger &logger::get_instance() {
  static logger instance;
  return instance;
}

void logger::putlog(std::string_view log) {
  if (am_I_alive)
    std::cerr << "LOG\t" << log << std::endl;
  else
    std::cerr << "LOG\t" << "I am dead, I cannot put logs anymore \n\t" << 
      "But I will try to put this log: " << log << std::endl;
}
