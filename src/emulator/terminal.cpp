#include "../../inc/emulator/terminal.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
using namespace std;


Terminal::Terminal(){
  interrupt = false;
  termIn = 0;

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  tcgetattr(STDIN_FILENO, &t);

  old_c_lflag = t.c_lflag;
  t.c_lflag &= ~(ICANON | ECHO);

  tcsetattr(STDIN_FILENO, TCSANOW, &t);

}

void Terminal::update(){
  char rd;
  if(read(STDIN_FILENO, &rd, 1) >= 0){
    termIn = rd;
    interrupt = true;
  }
}

void Terminal::write(int32_t value){
  cout<<static_cast<char>(value)<<flush;
}

Terminal::~Terminal(){
  {
    t.c_lflag = old_c_lflag;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &t);
  }
}