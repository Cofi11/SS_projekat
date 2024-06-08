#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP

#include <iostream>
#include <vector>
using namespace std;

enum InstructionType{
  HALT,
  INT,
  IRET,
  CALL,
  RET,
  JMP,
  BEQ,
  BNE,
  BGT,
  PUSH,
  POP,
  XCHG,
  ADD,
  SUB,
  MUL,
  DIV,
  NOT,
  AND,
  OR,
  XOR,
  SHL,
  SHR,
  LD,
  ST,
  CSRRD,
  CSRWR
};

enum DirectiveType{
  GLOBAL,
  EXTERN,
  SECTION,
  WORD,
  SKIP,
  ASCII,
  EQU,
  END
};

struct SymbolLiteral{
    bool isSymbol;

    union{
      char* symbol; //problem neki ako je string zbog destruktora
      int32_t literal;
    }value;
};

enum OperandType{
  OP_REG,
  OP_REG_IND,
  OP_IMM_LIT,
  OP_IMM_SYM,
  OP_MEM_LIT,
  OP_MEM_SYM,
  OP_MEM_OFF_LIT,
  OP_MEM_OFF_SYM,
  OP_PC_LIT,
  OP_PC_SYM,
  OP_NONE
};

struct Operand{
    OperandType type;

    union{
      int reg;
      SymbolLiteral imm;
      SymbolLiteral mem;
      struct{
        int reg;
        SymbolLiteral offset;
      }memOff;
    }value;

    // bool pcRelative; //kad je pc relative tad moze samo opcija IMM (literal il simbol)

};

struct ExpressionOperand{
    SymbolLiteral symbol;
    int sign;
};

struct Instruction{
    InstructionType type;
  
    int regSrc;
    int regDst;

    Operand op;

};

struct Directive {
    DirectiveType type;

    // union{
      vector<SymbolLiteral>* symbols_literals;//problem ako je vektor zbog destrukotra
      
      vector<ExpressionOperand>* expression;

      char* ascii_string;
    // }

    // Directive() : symbols_literals(new vector<SymbolLiteral>), signs(new vector<int>) {} 
    // ~Directive() {
    //     delete symbols_literals;
    //     delete signs;
    // }
};

enum LineType{
  LINE_INS,
  LINE_DIR,
  LINE_LABEL
};

struct Line{

    char* label; 

    LineType type;

    union{
      Instruction ins;
      Directive dir;
    }content;

    Line() : label(nullptr) {
      // content.ins.op.type = OP_NONE; 
    }

};

#endif