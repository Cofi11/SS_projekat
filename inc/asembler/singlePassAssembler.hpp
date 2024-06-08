#ifndef SINGLE_PASS_ASSEMBLER_HPP
#define SINGLE_PASS_ASSEMBLER_HPP

#include <vector>
#include <iostream>
#include <unordered_map>
#include "../structures.hpp"
#include "../common/symbolTable.hpp"
#include "../common/relocationTable.hpp"
#include "equTable.hpp"
using namespace std;

class SinglePassAssembler;

typedef int (SinglePassAssembler::*InstructionHandler)(Instruction& instruction);
typedef int (SinglePassAssembler::*DirectiveHandler)(Directive& directive);

class SinglePassAssembler{

private:

  SymbolTable symbolTable;
  vector<RelocationTable> relocationTables;
  EquTable equTable;

  vector<vector<uint8_t>> sectionsContent;
  vector<string> sectionsNames;
  unordered_map<int32_t, vector<int32_t>> literalPool; //bazen literala za trenutnu sekciju
  unordered_map<string, vector<int32_t>> symbolPool; //bazen simbola za trenutnu sekciju
  vector<SymbolLiteral> orderInPool;
  int firstForPoolAt;


  bool finished; //postavlja se na true kad je .end direktiva nadjena

  const static InstructionHandler instructionHandlers[];
  const static DirectiveHandler directiveHandlers[];

  SinglePassAssembler();
  SinglePassAssembler(const SinglePassAssembler&) = delete;
  SinglePassAssembler& operator=(const SinglePassAssembler&) = delete;

  //Handleri za direktive
  int handleGlobalDirective(Directive& directive);
  int handleExternDirective(Directive& directive);
  int handleSectionDirective(Directive& directive);
  int handleWordDirective(Directive& directive);
  int handleSkipDirective(Directive& directive);
  int handleAsciiDirective(Directive& directive);
  int handleEquDirective(Directive& directive);
  int handleEndDirective(Directive& directive);


  //Handleri za instrkucije
  int handleHaltInstruction(Instruction& instruction);
  int handleIntInstruction(Instruction& instruction);
  int handleIretInstruction(Instruction& instruction);
  int handleCallInstruction(Instruction& instruction);
  int handleRetInstruction(Instruction& instruction);           
  int handleJmpInstruction(Instruction& instruction);
  int handleBeqInstruction(Instruction& instruction);
  int handleBneInstruction(Instruction& instruction);
  int handleBgtInstruction(Instruction& instruction);
  int handlePushInstruction(Instruction& instruction);
  int handlePopInstruction(Instruction& instruction);
  int handleXchgInstruction(Instruction& instruction);
  int handleAddInstruction(Instruction& instruction);
  int handleSubInstruction(Instruction& instruction);
  int handleMulInstruction(Instruction& instruction);
  int handleDivInstruction(Instruction& instruction);
  int handleNotInstruction(Instruction& instruction);
  int handleAndInstruction(Instruction& instruction);
  int handleOrInstruction(Instruction& instruction);
  int handleXorInstruction(Instruction& instruction);
  int handleShlInstruction(Instruction& instruction);
  int handleShrInstruction(Instruction& instruction);
  int handleLdInstruction(Instruction& instruction);
  int handleStInstruction(Instruction& instruction);
  int handleCsrrdInstruction(Instruction& instruction);
  int handleCsrwrInstruction(Instruction& instruction);


  void write32bitToSection(int32_t value, int sectionIndex, int offset = -1);
  void writeInstructionToSection(int32_t value, int sectionIndex, int offset = -1);
  int32_t readInstructionFromSection(int sectionIndex, int offset);
  void printAllSectionContent();
  bool checkIn12bitsRange(int32_t value) const;

  void PCLiteralInstruction(int32_t value, int opcode, int mode1, int mode2, int regA, int regB, int regC);
  void PCSymbolInstruction(string value, int opcode, int mode1, int mode2, int regA, int regB, int regC);

  void addLiteralToPool(int32_t value);
  void addSymbolToPool(string value);
  void resolveLiteralPool();
  bool checkPoolNeccessary(int32_t value);

  void resolveForwardReferences(string symbol);
  void addRelocation(string symbol, RelocationType type, string section = "", int index = -1);
  int getSectionIndex(string section);
  int finalizeRelocationTables();
  void printAllRelocationTables();
  int nonEmptyRelocationTables();
  
  int resolveEquTable();

  void writeOutput(const char* outputFileName);

public:
  
  static SinglePassAssembler& getInstance(){
    static SinglePassAssembler instance;
    return instance;
  }

  int assemble(vector<Line> parsedCode, char* outputFileName);

  ~SinglePassAssembler();
};

#endif