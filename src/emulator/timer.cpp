#include "../../inc/emulator/timer.hpp"


void Timer::update(){
    auto currentTime = chrono::steady_clock::now();
    auto diff = chrono::duration_cast<chrono::milliseconds>(currentTime - start).count();

    

    if(diff >= getInterval()){
        interrupt = true;
        start = currentTime;
        // cout<<getInterval()<<endl;
    }
}

int Timer::getInterval(){
    switch(timCfg % 8){
        case 0:
            return 500;
        case 1:
            return 1000;
        case 2:
            return 1500;
        case 3:
            return 2000;
        case 4:
            return 5000;
        case 5:
            return 10000;
        case 6:
            return 30000;
        case 7:
            return 60000;
    }
}