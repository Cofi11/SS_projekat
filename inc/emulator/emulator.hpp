#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include "CPU.hpp"
#include "../instructions.hpp"
#include "timer.hpp"
#include "terminal.hpp"
using namespace std;

const uint32_t start_address = 0x40000000;

class Emulator;

typedef int (Emulator::*OpcodeHandler)(int32_t instruction);

class Emulator{

  private:

      const static OpcodeHandler opcode_handlers[];
      map<uint32_t, uint8_t> memory;
      CPU cpu;
      bool running = true;
      Timer timer;
      Terminal terminal;
      
      Emulator() {
        cpu.regs[REG_PC] = start_address;
      }
      Emulator(const Emulator&) = delete;
      Emulator& operator=(const Emulator&) = delete;

      int readMemory(ifstream &input);

      int handle_next_insruction();

      int32_t readInstructionFromMemory(uint32_t address);
      int32_t read32bitFromMemory(uint32_t address);
      void write32bitToMemory(uint32_t address, int32_t value);

      void printCpuState();

      void checkInterrupts();


      int handle_halt(int32_t instruction);
      int handle_int(int32_t instruction);
      int handle_call(int32_t instruction);
      int handle_jmp(int32_t instruction);
      int handle_xchg(int32_t instruction);
      int handle_arith(int32_t instruction);
      int handle_logic(int32_t instruction);
      int handle_shift(int32_t instruction);
      int handle_store(int32_t instruction);
      int handle_load(int32_t instruction);

      void writeToRegister(int reg, int32_t value, bool control = false);

  public:

      static Emulator& getInstance(){
        static Emulator instance;
        return instance;
      }

      int emulate(ifstream& input);

      ~Emulator() {}

};

#endif