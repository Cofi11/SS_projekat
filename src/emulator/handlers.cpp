#include "../../inc/emulator/emulator.hpp"
#include <iomanip>

const OpcodeHandler Emulator::opcode_handlers[]= {

  &Emulator::handle_halt,
  &Emulator::handle_int,
  &Emulator::handle_call,
  &Emulator::handle_jmp,
  &Emulator::handle_xchg,
  &Emulator::handle_arith,
  &Emulator::handle_logic,
  &Emulator::handle_shift,
  &Emulator::handle_store,
  &Emulator::handle_load

};

int Emulator::handle_halt(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": HALT" <<endl;

  running = false;
  return 0;

}

int Emulator::handle_int(int32_t instruction){
  
  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": INT" <<endl;

  cpu.regs[REG_SP] -= 4;
  uint32_t sp = static_cast<uint32_t> (cpu.regs[REG_SP]);
  write32bitToMemory(sp, cpu.control_regs[CREG_STATUS]);
  cpu.regs[REG_SP] -= 4;
  sp = static_cast<uint32_t> (cpu.regs[REG_SP]);
  write32bitToMemory(sp, cpu.regs[REG_PC]);
  
  cpu.control_regs[CREG_CAUSE] = 4;
  cpu.control_regs[CREG_STATUS] &= ~(0x1);

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.control_regs[CREG_HANDLER]) << dec;

  cpu.regs[REG_PC] = cpu.control_regs[CREG_HANDLER];

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;


  return 0;

}

int Emulator::handle_call(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout <<": CALL" <<endl;

  cpu.regs[REG_SP] -= 4;
  uint32_t sp = static_cast<uint32_t> (cpu.regs[REG_SP]);
  write32bitToMemory(sp, cpu.regs[REG_PC]);
  uint32_t address;

  switch(GET_MOD(instruction)){

    case(CALL_REGS):
      cpu.regs[REG_PC] = cpu.regs[GET_REGA(instruction)] + cpu.regs[GET_REGB(instruction)] + GET_DISP(instruction);
      break;

    case(CALL_MEM):
      address = static_cast<uint32_t> (cpu.regs[GET_REGA(instruction)] + cpu.regs[GET_REGB(instruction)] + GET_DISP(instruction));
      cpu.regs[REG_PC] = read32bitFromMemory(address);
      break;

  }

  return 0;

}

int Emulator::handle_jmp(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout <<": JMP" <<endl;

  int regA = GET_REGA(instruction);
  int regB = GET_REGB(instruction);
  int regC = GET_REGC(instruction);
  int disp = GET_DISP(instruction);
  uint32_t address;

  switch (GET_MOD(instruction)){
    case (JMP_REG):
      cpu.regs[REG_PC] = cpu.regs[regA] + disp;
      break;
    case (JMP_REG_EQ):
      if (cpu.regs[regB] == cpu.regs[regC]){
        cpu.regs[REG_PC] = cpu.regs[regA] + disp;
      }
      break;
    case (JMP_REG_NE):
      if (cpu.regs[regB] != cpu.regs[regC]){
        cpu.regs[REG_PC] = cpu.regs[regA] + disp;
      }
      break;
    case (JMP_REG_GT):
      if (cpu.regs[regB] > cpu.regs[regC]){
        cpu.regs[REG_PC] = cpu.regs[regA] + disp;
      }
      break;
    case (JMP_MEM):
      address = static_cast<uint32_t> (cpu.regs[regA] + disp);
      cpu.regs[REG_PC] = read32bitFromMemory(address);
      break;
    case (JMP_MEM_EQ):
      if (cpu.regs[regB] == cpu.regs[regC]){
        address = static_cast<uint32_t> (cpu.regs[regA] + disp);
        cpu.regs[REG_PC] = read32bitFromMemory(address);
      }
      break;
    case (JMP_MEM_NE):
      if (cpu.regs[regB] != cpu.regs[regC]){
        address = static_cast<uint32_t> (cpu.regs[regA] + disp);
        cpu.regs[REG_PC] = read32bitFromMemory(address);
      }
      break;
    case (JMP_MEM_GT):
      if (cpu.regs[regB] > cpu.regs[regC]){
        address = static_cast<uint32_t> (cpu.regs[regA] + disp);
        cpu.regs[REG_PC] = read32bitFromMemory(address);
      }
      break;
  }

  return 0;

}

int Emulator::handle_xchg(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": XCHG" <<endl;

  int regB = GET_REGB(instruction);
  int regC = GET_REGC(instruction);

  int32_t temp = cpu.regs[regB];
  writeToRegister(regB, cpu.regs[regC]);
  writeToRegister(regC, temp);

  return 0;

}

int Emulator::handle_arith(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": ARITH" <<endl;

  int regA = GET_REGA(instruction);
  int regB = GET_REGB(instruction);
  int regC = GET_REGC(instruction);

  switch(GET_MOD(instruction)){
    case(ARITH_ADD):
      writeToRegister(regA, cpu.regs[regB] + cpu.regs[regC]);
      break;
    case(ARITH_SUB):
      writeToRegister(regA, cpu.regs[regB] - cpu.regs[regC]);
      break;
    case(ARITH_MUL):
      writeToRegister(regA, cpu.regs[regB] * cpu.regs[regC]);
      break;
    case(ARITH_DIV):
      writeToRegister(regA, cpu.regs[regB] / cpu.regs[regC]);
      break;
  }

  return 0;

}

int Emulator::handle_logic(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": LOGIC" <<endl;

  int regA = GET_REGA(instruction);
  int regB = GET_REGB(instruction);
  int regC = GET_REGC(instruction);

  switch(GET_MOD(instruction)){
    case(LOGIC_NOT):
      writeToRegister(regA, ~cpu.regs[regB]);
      break;
    case(LOGIC_AND):
      writeToRegister(regA, cpu.regs[regB] & cpu.regs[regC]);
      break;
    case(LOGIC_OR):
      writeToRegister(regA, cpu.regs[regB] | cpu.regs[regC]);
      break;
    case(LOGIC_XOR):
      writeToRegister(regA, cpu.regs[regB] ^ cpu.regs[regC]);
      break;
  }

  return 0;

}

int Emulator::handle_shift(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": SHIFT" <<endl;

  int regA = GET_REGA(instruction);
  int regB = GET_REGB(instruction);
  int regC = GET_REGC(instruction);

  switch(GET_MOD(instruction)){
    case(SHIFT_L):
      writeToRegister(regA, cpu.regs[regB] << cpu.regs[regC]);
      break;
    case(SHIFT_R):
      writeToRegister(regA, cpu.regs[regB] >> cpu.regs[regC]);
      break;
  }

  return 0;

}

int Emulator::handle_store(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": STORE" <<endl;

  int regA = GET_REGA(instruction);
  int regB = GET_REGB(instruction);
  int regC = GET_REGC(instruction);
  int disp = GET_DISP(instruction);
  uint32_t address;

  switch(GET_MOD(instruction)){
    case(STORE_MEMDIR):
      address = static_cast<uint32_t> (cpu.regs[regA] + cpu.regs[regB] + disp);
      write32bitToMemory(address, cpu.regs[regC]);
      break;
    case(STORE_MEMIND):
      address = static_cast<uint32_t> (cpu.regs[regA] + cpu.regs[regB] + disp);
      address = static_cast<uint32_t> (read32bitFromMemory(address));
      write32bitToMemory(address, cpu.regs[regC]);
      break;
    case(STORE_PUSH):
      cpu.regs[regA] += disp;
      address = static_cast<uint32_t> (cpu.regs[regA]);
      write32bitToMemory(address, cpu.regs[regC]);
      break;
  }

  return 0;

}

int Emulator::handle_load(int32_t instruction){

  // cout << hex << setfill('0') << static_cast<uint32_t>(cpu.regs[REG_PC]) << dec;
  // cout << ": LOAD" <<endl;

  int regA = GET_REGA(instruction);
  int regB = GET_REGB(instruction);
  int regC = GET_REGC(instruction);
  int disp = GET_DISP(instruction);
  uint32_t address;

  switch(GET_MOD(instruction)){
    case(LOAD_CSRRD):
      writeToRegister(regA, cpu.control_regs[regB]);
      break;
    case (LOAD_REG):
      writeToRegister(regA, cpu.regs[regB] + disp);
      break;
    case (LOAD_REGIND):
      address = static_cast<uint32_t> (cpu.regs[regB] + cpu.regs[regC] + disp);
      writeToRegister(regA, read32bitFromMemory(address));
      break;
    case (LOAD_POP):
      address = static_cast<uint32_t> (cpu.regs[regB]);
      writeToRegister(regA, read32bitFromMemory(address));
      cpu.regs[regB] += disp;
      break;
    case (LOAD_CSRWR):
      writeToRegister(regA, cpu.regs[regB], true);
      break;
    case (LOAD_CREGIND):
      address = static_cast<uint32_t> (cpu.regs[regB] + cpu.regs[regC] + disp);
      writeToRegister(regA, read32bitFromMemory(address), true);
      break;
    case (LOAD_POPC):
      address = static_cast<uint32_t> (cpu.regs[regB]);
      writeToRegister(regA, read32bitFromMemory(address), true);
      cpu.regs[regB] += disp;
      break;
  }

  return 0;

}