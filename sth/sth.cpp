#include "sth.h"
#include "logger.h"
#include <iostream>

sth::sth() {
  // I suppose in this way we can order the initialization and deinitialization
  // of the singletons.
  std::cerr << "Start of sth::sth(), I will try to put logs" << std::endl;
  logger::get_instance().putlog("std is being initialized");
  std::cerr << "End of sth::sth(), the runtime will consider I am fully "
               "constructed then"
            << std::endl;
}

sth::~sth() { logger::get_instance().putlog("std is being deinitialized"); }

sth &sth::get_instance() {
  static sth instance;
  return instance;
}

void sth::do_something() {
  logger::get_instance().putlog("sth::do_something() is called");
}
