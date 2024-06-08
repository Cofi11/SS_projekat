#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <cstdint>
#include <termios.h>

class Terminal {

private:

  bool interrupt = false;
  int32_t termIn;

  termios t;
  tcflag_t old_c_lflag;

public:

  Terminal();

  void update();

  void write(int32_t value);

  bool checkInterrupt() const {
    return interrupt;
  }

  void clearInterrupt() {
    interrupt = false;
  }

  int32_t getTermIn() const {
    return termIn;
  }

  ~Terminal();

};

#endif