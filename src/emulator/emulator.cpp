#include "../../inc/emulator/emulator.hpp"
#include <sstream>
#include <iomanip> 

int Emulator::emulate(ifstream& input){

  int ret = readMemory(input);

  input.close();

  if(ret){
    return ret;
  }

  while(running){

    handle_next_insruction();

    timer.update();
    terminal.update();

    checkInterrupts();
    
    // printCpuState();


  }

  printCpuState();

  return 0;

}


int Emulator::readMemory(ifstream& input){

  string line;
  while (getline(input, line)) {
      istringstream iss(line);
      uint32_t address;
      char colon;
      if (!(iss >> hex >> address >> colon)) {
          cout << "ERROR: Impossible to parse input file" << endl;
          return 1; //nemoguce parsirati ulazni fajl
      }

      uint16_t value;
      while (iss >> hex >> value) {
        if (value > 0xFF) {
            cout << "ERROR: Impossible to parse input file" << endl;
            return 1; // Value is larger than a byte
        }
        memory[address++] = static_cast<uint8_t>(value);
      }
  }

  return 0;

}


int Emulator::handle_next_insruction(){

  uint32_t pc = static_cast<uint32_t>(cpu.regs[REG_PC]);
  int32_t instruction = readInstructionFromMemory(pc);

  // cout << hex << setfill('0') << instruction << dec << endl;

  cpu.regs[REG_PC] += 4;

  int opcode = GET_OPCODE(instruction);

  if(opcode < 0 || opcode >= 10){
    cout << "ERROR: Invalid opcode: " << opcode << endl;
    return 3;
  }

  (this->*opcode_handlers[GET_OPCODE(instruction)])(instruction);

  return 0;
  
}



int32_t Emulator::readInstructionFromMemory(uint32_t address){
  int32_t value = 0;
  value |= memory[address + 3];
  value |= memory[address + 2] << 8;
  value |= memory[address + 1] << 16;
  value |= memory[address] << 24;
  return value;
}


int32_t Emulator::read32bitFromMemory(uint32_t address){

  if(address >= 0xFFFFFF04 && address <= 0xFFFFFF07){//terminal reg
    return terminal.getTermIn();
  }

  int32_t value = 0;
  value |= (int32_t)memory[address + 3] << 24;
  value |= (int32_t)memory[address + 2] << 16;
  value |= (int32_t)memory[address + 1] << 8;
  value |= (int32_t)memory[address];
  return value;
}

void Emulator::write32bitToMemory(uint32_t address, int32_t value){

  if(address >= 0xFFFFFF10 && address <= 0xFFFFFF13){//tajmer reg
    timer.setTimCfg(value);
    return;
  }
  else if(address >= 0xFFFFFF00 && address <= 0xFFFFFF03){//terminal reg
    terminal.write(value);
    return;
  }

  memory[address] = value & 0xFF;
  memory[address + 1] = (value >> 8) & 0xFF;
  memory[address + 2] = (value >> 16) & 0xFF;
  memory[address + 3] = (value >> 24) & 0xFF;
}


void Emulator::writeToRegister(int reg, int32_t value, bool control){

  if(control){
    cpu.control_regs[reg] = value;
  }else{
    if(reg == 0)return;
    cpu.regs[reg] = value;
  }

}







void Emulator::printCpuState() {

  cout<<endl;
  cout<<"-----------------CPU STATE-----------------"<<endl;
  cout<<"Emulated processor executed halt instrcution:"<<endl;
  cout<<"Emulated processor state:"<<endl;

    for (int i = 0; i < 16; ++i) {
        
        stringstream ss;
        ss << "r" << i;
       string regName = ss.str();

        cout << setw(3) << right <<  setfill(' ') << regName;

        cout << ": 0x" <<setw(8) << setfill('0') << hex << cpu.regs[i] << "  ";
        
        if ((i + 1) % 4 == 0) {
            cout << "\n";
        }
    }

    // for(int i = 0 ; i < 3 ; i++){
    //   stringstream ss;
    //   ss << "cr" << i;
    //   string regName = ss.str();

    //   cout << setw(3) << right <<  setfill(' ') << regName;

    //   cout << ": 0x" <<setw(8) << setfill('0') << hex << cpu.control_regs[i] << "  ";
    // }
}



void Emulator::checkInterrupts(){
  
  if(!running || (cpu.control_regs[CREG_STATUS] & statusI)){// 1 znaci maskirani prekidi
    return;
  }

  if(terminal.checkInterrupt() && !(cpu.control_regs[CREG_STATUS] & statusTl)){

    cpu.regs[REG_SP] -= 4;
    uint32_t sp = static_cast<uint32_t> (cpu.regs[REG_SP]);
    write32bitToMemory(sp, cpu.control_regs[CREG_STATUS]);
    cpu.regs[REG_SP] -= 4;
    sp = static_cast<uint32_t> (cpu.regs[REG_SP]);
    write32bitToMemory(sp, cpu.regs[REG_PC]);
  
    cpu.control_regs[CREG_CAUSE] = 3;
    cpu.control_regs[CREG_STATUS] &= ~(0x1);

    cpu.regs[REG_PC] = cpu.control_regs[CREG_HANDLER];

    terminal.clearInterrupt();
    return;
    
  }
  if(timer.checkInterrupt() && !(cpu.control_regs[CREG_STATUS] & statusTr)){

    cpu.regs[REG_SP] -= 4;
    uint32_t sp = static_cast<uint32_t> (cpu.regs[REG_SP]);
    write32bitToMemory(sp, cpu.control_regs[CREG_STATUS]);
    cpu.regs[REG_SP] -= 4;
    sp = static_cast<uint32_t> (cpu.regs[REG_SP]);
    write32bitToMemory(sp, cpu.regs[REG_PC]);
  
    cpu.control_regs[CREG_CAUSE] = 2;
    cpu.control_regs[CREG_STATUS] &= ~(0x1);

    cpu.regs[REG_PC] = cpu.control_regs[CREG_HANDLER];

    timer.clearInterrupt();
    return;
    
  }
}