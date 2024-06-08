#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#define SET_DISP(x, y) ((x) = ((x) & 0xFFFFF000) | ((y) & 0x0FFF))
#define SET_REGC(x, y) ((x) = ((x) & 0xFFFF0FFF) | (((y) & 0x0F) << 12))
#define SET_REGB(x, y) ((x) = ((x) & 0xFFF0FFFF) | (((y) & 0x0F) << 16))
#define SET_REGA(x, y) ((x) = ((x) & 0xFF0FFFFF) | (((y) & 0x0F) << 20))
#define SET_MOD(x, y) ((x) = ((x) & 0xF0FFFFFF) | (((y) & 0x0F) << 24))
#define SET_OPCODE(x, y) ((x) = ((x) & 0x0FFFFFFF) | (((y) & 0x0F) << 28))

#define GET_DISP(x) ((x & 0x800 ? (x | 0xFFFFF000) : (x & 0x0FFF))) //sign expanded 12bit to 32bit
#define GET_REGC(x) (((x) >> 12) & 0x0F)
#define GET_REGB(x) (((x) >> 16) & 0x0F)
#define GET_REGA(x) (((x) >> 20) & 0x0F)
#define GET_MOD(x) (((x) >> 24) & 0x0F)
#define GET_OPCODE(x) (((x) >> 28) & 0x0F)


enum Opcode{
  OPC_HALT = 0x00,
  OPC_INT = 0x01,
  OPC_CALL = 0x02,
  OPC_JMP = 0x03,
  OPC_XCHG = 0x04,
  OPC_ARITH = 0x05,
  OPC_LOGIC = 0x06,
  OPC_SHIFT = 0x07,
  OPC_STORE = 0x08,
  OPC_LOAD = 0x09
};

enum Mode_call{
  CALL_REGS = 0x00,
  CALL_MEM = 0x01
};

enum Mode_jmp{
  JMP_REG = 0x00,
  JMP_REG_EQ = 0x01,
  JMP_REG_NE = 0x02,
  JMP_REG_GT = 0x03,
  JMP_MEM = 0x08,
  JMP_MEM_EQ = 0x09,
  JMP_MEM_NE = 0x0A,
  JMP_MEM_GT = 0x0B
};

enum Mode_arith{
  ARITH_ADD = 0x00,
  ARITH_SUB = 0x01,
  ARITH_MUL = 0x02,
  ARITH_DIV = 0x03
};

enum Mode_logic{
  LOGIC_NOT = 0x00,
  LOGIC_AND = 0x01,
  LOGIC_OR = 0x02,
  LOGIC_XOR = 0x03
};

enum Mode_shift{
  SHIFT_L = 0x00,
  SHIFT_R = 0x01
};

enum Mode_store{
  STORE_MEMDIR = 0x00,
  STORE_MEMIND = 0x02,
  STORE_PUSH = 0x01
};

enum Mode_load{
  LOAD_CSRRD = 0x00,
  LOAD_REG = 0x01,
  LOAD_REGIND = 0x02,
  LOAD_POP = 0x03,
  LOAD_CSRWR = 0x04,
  LOAD_CREGIND = 0x06,
  LOAD_POPC = 0x07
};


enum regs{
  REG_SP = 14,
  REG_PC = 15,

  CREG_STATUS = 0,
  CREG_HANDLER = 1,
  CREG_CAUSE = 2
};


#endif