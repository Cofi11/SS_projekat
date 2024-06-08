#ifndef TIMER_HPP
#define TIMER_HPP

#include <cstdint>
#include <chrono>
#include <iostream>
using namespace std;

class Timer {

private:
  chrono::steady_clock::time_point start;
  int32_t timCfg = 0;
  bool interrupt = false;

public:
    Timer(){
        start = chrono::steady_clock::now();
    }
    
    void update();

    int getInterval();

    bool checkInterrupt() const{
        return interrupt;
    }

    void clearInterrupt(){
        interrupt = false;
    }

    void setTimCfg(int32_t value){
        // cout << value;
        timCfg = value;
    }

};


#endif