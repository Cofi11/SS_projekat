#ifndef CPU_HPP
#define CPU_HPP

#include <iostream>

const int32_t statusTr = 0x1;
const int32_t statusTl = 0x2;
const int32_t statusI = 0x4;

struct CPU{

  int32_t regs[16];
  int32_t control_regs[3];
  // uint8_t irq;
  
};

#endif

