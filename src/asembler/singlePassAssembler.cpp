#include "../../inc/asembler/singlePassAssembler.hpp"
#include "../../inc/instructions.hpp"

#include <iomanip>
#include <string.h>
#include <fstream>
#include <algorithm>
#include <iterator>

int createInstruction(int opcode, int mod, int regA, int regB, int regC, int disp);

SinglePassAssembler::SinglePassAssembler(){

  symbolTable.addEntry("UND", 0, SYM_DEFINED | SYM_SECTION, "UND");
  symbolTable.addEntry("ABS", 0, SYM_DEFINED | SYM_SECTION, "ABS");
  symbolTable.addEntry(".text", 0, SYM_DEFINED | SYM_SECTION, ".text");

  sectionsContent.push_back(vector<uint8_t>()); //empty section
  sectionsNames.push_back(".text");

  relocationTables.push_back(RelocationTable()); //ako ne bude potrebna relok tabela za sekciju bice prazno, pa se nece upisivati u obj fajl

  finished = false;

  firstForPoolAt = -1;

}

int SinglePassAssembler::assemble(vector<Line> parsedCode, char* outputFileName){

  if(finished){
    cout << "ERROR: already finished" << endl;
    return 3; //ERROR: already finished
  }

  // cout << endl << endl << "Assembling..." << endl;

  int index = 0;

  for(Line& l : parsedCode){

    if(l.label != nullptr){
      if(symbolTable.checkSymbol(l.label)){
        if(symbolTable.checkDefined(l.label)){
          cout << "ERROR: multiple definitions of symbol: " << l.label << endl;
          cout << "ERROR: while processing line: " << index << endl;
          return 1; //ERROR: multiple definistions
        }
        else{
          symbolTable.setFlags(l.label, SYM_DEFINED);
          symbolTable.setSection(l.label, sectionsNames.back());
          symbolTable.setSymbolValue(l.label, sectionsContent.back().size());

          if(symbolTable.checkForwardReferences(l.label)){
            resolveForwardReferences(l.label);
          }
        }
      }
      else{
        symbolTable.addEntry(l.label, sectionsContent.back().size(), SYM_DEFINED, sectionsNames.back());
      }
    }

    int ret = 0;

    if(l.type == LINE_DIR){
      ret = (this->*SinglePassAssembler::directiveHandlers[l.content.dir.type])(l.content.dir);
    }
    else if(l.type == LINE_INS){
      ret = (this->*SinglePassAssembler::instructionHandlers[l.content.ins.type])(l.content.ins);
    }

    if(ret){
      cout << "ERROR: while processing line: " << index << endl;
      return ret; //ERROR while processing directive or instruction
    }

    index++;
    if(finished)break;

  }

  vector<string>& allSyms = symbolTable.getSymbolNames();
  for(string symbol: allSyms){

    if(!symbolTable.checkDefined(symbol) && !symbolTable.checkGlobal(symbol)){
      cout << "ERROR: Undefined, non extern/global symbols left!" << endl;
      return 7; //ERROR: undefined symbols left
    } 
    else if(!symbolTable.checkDefined(symbol)){//globalni a nedfenisian
      symbolTable.setFlags(symbol, SYM_GLOBAL | SYM_DEFINED);
      if(symbolTable.checkForwardReferences(symbol)){
        resolveForwardReferences(symbol);
      }
    }
  }

  // if(symbolTable.checkUndefinedSymbols()){
  //   return 7; //ERROR: undefined symbols left
  // }

  //pre relocatioin table finalizacije, treba equ neizracunljive simbole da se izracunaju
  // equTable.printTable();
  // cout<<endl;
  int ret = resolveEquTable();

  if(ret)
    return ret;


  ret = finalizeRelocationTables();

  if(ret)
    return ret;

  writeOutput(outputFileName);

  return 0;
  
}

SinglePassAssembler::~SinglePassAssembler(){

}





void SinglePassAssembler::write32bitToSection(int32_t value, int sectionIndex, int offset){
  if(offset == -1){
    sectionsContent[sectionIndex].push_back(value & 0xFF);
    sectionsContent[sectionIndex].push_back((value >> 8) & 0xFF);
    sectionsContent[sectionIndex].push_back((value >> 16) & 0xFF);
    sectionsContent[sectionIndex].push_back((value >> 24) & 0xFF);
  }
  else{
    sectionsContent[sectionIndex][offset] = value & 0xFF;
    sectionsContent[sectionIndex][offset + 1] = (value >> 8) & 0xFF;
    sectionsContent[sectionIndex][offset + 2] = (value >> 16) & 0xFF;
    sectionsContent[sectionIndex][offset + 3] = (value >> 24) & 0xFF;
  }
}

void SinglePassAssembler::writeInstructionToSection(int32_t value, int sectionIndex, int offset){
  //provera dal mora pool da se uglavi tu
  //ako ne mozemo da grantujemo da ce pool moci i nakon ovog upisa da se doda 
  // (+4 jer dodajemo novu instr, +4 jer nam treab skok preko poola i minus 4 jer cemo upisivati pcRel pomeraj na adresi firstForPoolAt)
  if(offset == -1 && checkPoolNeccessary(4)){ //samo za nove instrukcije koje se dodaju (offset == -1)

    //ugraditi hardCode jump isntrukciju za velicinu poola
    int disp = orderInPool.size() * 4;
    int instructionValue = createInstruction(OPC_JMP, JMP_REG, REG_PC, 0, 0, disp);
    sectionsContent[sectionIndex].push_back((instructionValue >> 24) & 0xFF);
    sectionsContent[sectionIndex].push_back((instructionValue >> 16) & 0xFF);
    sectionsContent[sectionIndex].push_back((instructionValue >> 8) & 0xFF);
    sectionsContent[sectionIndex].push_back(instructionValue  & 0xFF);

    resolveLiteralPool();
  }


  if(offset == -1){
    sectionsContent[sectionIndex].push_back((value >> 24) & 0xFF);
    sectionsContent[sectionIndex].push_back((value >> 16) & 0xFF);
    sectionsContent[sectionIndex].push_back((value >> 8) & 0xFF);
    sectionsContent[sectionIndex].push_back(value & 0xFF);
  }
  else{
    sectionsContent[sectionIndex][offset] = (value >> 24) & 0xFF;
    sectionsContent[sectionIndex][offset + 1] = (value >> 16) & 0xFF;
    sectionsContent[sectionIndex][offset + 2] = (value >> 8) & 0xFF;
    sectionsContent[sectionIndex][offset + 3] = value & 0xFF;
  }
}

int32_t SinglePassAssembler::readInstructionFromSection(int sectionIndex, int offset){
  int32_t value = 0;
  value |= sectionsContent[sectionIndex][offset + 3];
  value |= sectionsContent[sectionIndex][offset + 2] << 8;
  value |= sectionsContent[sectionIndex][offset + 1] << 16;
  value |= sectionsContent[sectionIndex][offset] << 24;
  return value;
}







void SinglePassAssembler::printAllSectionContent(){
  cout << "***** SECTIONS CONTENT *****" << endl;
  for(int i = 0; i < sectionsContent.size(); i++){
    if(sectionsContent[i].empty())continue;
    cout << "Section: " << sectionsNames[i] << endl;
    int j = 0;
    for(uint8_t byte : sectionsContent[i]){
      cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
      if(j++ == 7){
        j = 0;
        cout << endl;
      }
    }
    cout << endl;
  }
  cout << endl << setfill(' ') << dec ;
  // cout << "***** END" << endl;
}

void SinglePassAssembler::printAllRelocationTables(){
  cout << "***** RELOCATION TABLES *****" << endl;
  for(int i = 0; i < relocationTables.size(); i++){
    if(relocationTables[i].checkEmpty())continue;
    cout << "rela." << sectionsNames[i] << endl;
    relocationTables[i].printTable();
  }
  cout << endl;
  // cout << "***** END" << endl;
}

void SinglePassAssembler::writeOutput(const char* outputFileName) {
    string objdumpName = string(outputFileName).replace(string(outputFileName).find(".o"), 2, ".objdump");
    ofstream outputFile(objdumpName);

    // Redirect std::cout to the output file
    streambuf* coutbuf = std::cout.rdbuf();
    cout.rdbuf(outputFile.rdbuf());

    symbolTable.printTable();
    cout << endl;
    printAllSectionContent();
    cout << endl;
    printAllRelocationTables();

    // Reset std::cout to its original output
    cout.rdbuf(coutbuf);

    outputFile.close();

    
    ofstream output(outputFileName, ios::binary);

    uint32_t size = 1; //koliko razlicith stvari treba procitati, 1 je SYMTAB, pa + broj relok tabela + broj sekcija
    size += nonEmptyRelocationTables();
    size += sectionsContent.size();
    write(output, size);

    symbolTable.writeTableToOutput(output);
    for(RelocationTable& table : relocationTables){
      if(table.checkEmpty())continue;
      table.writeTableToOutput(output, sectionsNames[&table - &relocationTables[0]]);
    }

    for(int i = 0 ; i < sectionsContent.size(); i++){
      writeString(output, sectionsNames[i]);
      write(output, sectionsContent[i]);
    }

    output.close();

}







//HANDLERS
//Handleri za direktive
int SinglePassAssembler::handleGlobalDirective(Directive& directive) {
    // cout << "GLOBAL" << endl;
    
    for(SymbolLiteral& symbol : *directive.symbols_literals){
      if(symbolTable.checkSymbol(symbol.value.symbol)){
        if(symbolTable.checkSection(symbol.value.symbol)){
          cout << "ERROR: section cannot be global" << endl;
          return 2; //ERROR: section cannot be global
        }
        symbolTable.setFlags(symbol.value.symbol, SYM_GLOBAL);
      }
      else
        symbolTable.addEntry(symbol.value.symbol, 0, SYM_GLOBAL, "UND");//kad naidjemo na definiciju postavicemo pravu sekciju
    }

    return 0;
}

int SinglePassAssembler::handleExternDirective(Directive& directive) {
    // cout << "EXTERN" << endl;

    for(SymbolLiteral& symbol : *directive.symbols_literals){
      if(symbolTable.checkSymbol(symbol.value.symbol)){
        if(symbolTable.checkDefined(symbol.value.symbol)){
          cout << "ERROR: multiple definitions of symbol: " << symbol.value.symbol << endl;
          return 1; //Multiple definitons
        }
        symbolTable.setFlags(symbol.value.symbol, SYM_GLOBAL | SYM_DEFINED);
        if(symbolTable.checkForwardReferences(symbol.value.symbol)){
          resolveForwardReferences(symbol.value.symbol);
        }
      }
      else
        symbolTable.addEntry(symbol.value.symbol, 0, SYM_GLOBAL | SYM_DEFINED, "UND");
    }

    return 0;
}

int SinglePassAssembler::handleSectionDirective(Directive& directive) {
    // cout << "SECTION" << endl;

    resolveLiteralPool();

    SymbolLiteral symbol = (*directive.symbols_literals)[0];

    if(symbolTable.checkSymbol(symbol.value.symbol)){
      if(symbolTable.checkDefined(symbol.value.symbol)){
        cout << "ERROR: multiple definitions of symbol: " << symbol.value.symbol << endl;
        return 1; //Multiple definitons
      }
      else if(symbolTable.checkGlobal(symbol.value.symbol)){
        cout << "ERROR: section cannot be global" << endl;
        return 2; //ERROR: section cannot be global
      }
      symbolTable.setFlags(symbol.value.symbol, SYM_SECTION | SYM_DEFINED);
      if(symbolTable.checkForwardReferences(symbol.value.symbol)){
        resolveForwardReferences(symbol.value.symbol);
      }
    }
    else{
      symbolTable.addEntry(symbol.value.symbol, 0, SYM_SECTION | SYM_DEFINED, symbol.value.symbol);
    }

    this->sectionsContent.push_back(vector<uint8_t>());
    this->sectionsNames.push_back(symbol.value.symbol);

    this->relocationTables.push_back(RelocationTable());

    return 0;
}

int SinglePassAssembler::handleWordDirective(Directive& directive) {
    // cout << "WORD" << endl;

    //da li svi simboli mogu da stanu odjednom pre poola
    if(checkPoolNeccessary((*directive.symbols_literals).size() * 4)){ 

      //ugraditi hardCode jump isntrukciju za velicinu poola
      int disp = orderInPool.size() * 4;
      int instructionValue = createInstruction(OPC_JMP, JMP_REG, REG_PC, 0, 0, disp);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 24) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 16) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 8) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back(instructionValue  & 0xFF);

      resolveLiteralPool();
    }

    for(SymbolLiteral& symbol : *directive.symbols_literals){

      if(symbol.isSymbol){

        if(!symbolTable.checkSymbol(symbol.value.symbol)){
          symbolTable.addEntry(symbol.value.symbol, 0, 0, "UND");//kad bude definisian postavice se sekcija
          symbolTable.addForwardReference(symbol.value.symbol, this->sectionsContent.back().size(), this->sectionsNames.back());
          write32bitToSection(0, sectionsNames.size() - 1);//za sad vrednost 0 pa ce relok zapis to srediti
          continue;
        }
        else{
          if(!symbolTable.checkDefined(symbol.value.symbol)){
            symbolTable.addForwardReference(symbol.value.symbol, this->sectionsContent.back().size(), this->sectionsNames.back());//dodati forward referencu
            write32bitToSection(0, sectionsNames.size() - 1);//za sad vrednost 0 pa ce relok zapis to srediti
            continue;
          }
          else if(symbolTable.checkAbsolute(symbol.value.symbol) && !symbolTable.checkCalculated(symbol.value.symbol)){
            symbolTable.addForwardReference(symbol.value.symbol, this->sectionsContent.back().size(), this->sectionsNames.back());//dodati forward referencu
            write32bitToSection(0, sectionsNames.size() - 1);//za sad vrednost 0 pa ce relok zapis to srediti
            continue;
          }
        }  

        if(symbolTable.checkCalculated(symbol.value.symbol)){
          int value = symbolTable.getSymbolValue(symbol.value.symbol); 
          write32bitToSection(value, sectionsNames.size() - 1);//upisati vrednost simbola u content sekcije (4 bajta)
        }
        else{
          addRelocation(symbol.value.symbol, RelocationType::ABSOLUTE);

          write32bitToSection(0, sectionsNames.size() - 1);//ako nije apsolutan izracunat simbol upisati 0 pa ce relok zapis to srediti
        }

      }
      else{
        write32bitToSection(symbol.value.literal, sectionsNames.size() - 1);//literal upisati u content sekcije
      }

    }

    return 0;
}

int SinglePassAssembler::handleSkipDirective(Directive& directive) {
    // cout << "SKIP" << endl;

    SymbolLiteral literal = (*directive.symbols_literals)[0];
    int skipSize = literal.value.literal;

    //da li sve nule mogu da stanu odjednom pre poola
    if(checkPoolNeccessary(skipSize)){ 

      //ugraditi hardCode jump isntrukciju za velicinu poola
      int disp = orderInPool.size() * 4;
      int instructionValue = createInstruction(OPC_JMP, JMP_REG, REG_PC, 0, 0, disp);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 24) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 16) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 8) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back(instructionValue  & 0xFF);

      resolveLiteralPool();
    }

    
    //upisati skipSize bajtova jednakih 0 u content sekciju
    for(int i = 0; i < skipSize; i++){
      sectionsContent.back().push_back(0);
    }

    return 0;
}

int SinglePassAssembler::handleAsciiDirective(Directive& directive) {
    // cout << "ASCII" << endl;

    string asciiString = directive.ascii_string;

    //da li ceo string moze da stanu odjednom pre poola
    if(checkPoolNeccessary(asciiString.size())){ 

      //ugraditi hardCode jump isntrukciju za velicinu poola
      int disp = orderInPool.size() * 4;
      int instructionValue = createInstruction(OPC_JMP, JMP_REG, REG_PC, 0, 0, disp);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 24) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 16) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back((instructionValue >> 8) & 0xFF);
      sectionsContent[sectionsNames.size()-1].push_back(instructionValue  & 0xFF);

      resolveLiteralPool();
    }

    //upisati asciiString u content sekciju
    for(char c : asciiString){
      sectionsContent.back().push_back(c);
    }

    return 0;
}

int SinglePassAssembler::handleEquDirective(Directive& directive) {
    // cout << "EQU" << endl;
    // za sadda podrazumevano da equ moze samo sa vec def simbolima i literalima

    SymbolLiteral symbol = (*directive.symbols_literals)[0];
    vector<ExpressionOperand> expression = *directive.expression;

    int value = 0;
    bool valueKnown = true;

    for(ExpressionOperand& exp : expression){
      if(exp.symbol.isSymbol){
        if(symbolTable.checkSymbol(exp.symbol.value.symbol)){
          if(symbolTable.checkCalculated(exp.symbol.value.symbol)){//ako je vec izracunat abs simbol
            value += exp.sign * symbolTable.getSymbolValue(exp.symbol.value.symbol);
          }
          // else if(symbolTable.checkDefined(exp.symbol.value.symbol) && !symbolTable.checkAbsolute(exp.symbol.value.symbol)){
          //   return 8; //ERROR: not absolute symbol in EQU
          // }
          else{
            //mozda kasnije bude definisan pa je moguce za sad ili je vec definisan kao abs al nije bio izracunat odmah jer nije moglo
            //dodamo u equ tabelu
            equTable.addEntry(symbol.value.symbol, exp.symbol.value.symbol, exp.sign);

            valueKnown = false;
          }
        }
        else{
          //mozda kasnije bude definisan pa je moguce za sad
          //dodamo u equ tabelu
          equTable.addEntry(symbol.value.symbol, exp.symbol.value.symbol, exp.sign);

          valueKnown = false;
        }
      }
      else{
        value += exp.sign * exp.symbol.value.literal;
      }
    }

    int flags = SYM_DEFINED | SYM_ABSOLUTE;
    if(valueKnown){
      flags |= SYM_CALCULATED;
    }

    if(symbolTable.checkSymbol(symbol.value.symbol)){
      if(symbolTable.checkDefined(symbol.value.symbol)){
        cout << "ERROR: multiple definitions of symbol: " << symbol.value.symbol << endl;
        return 1; //ERROR: multiple definitons
      }
      symbolTable.setFlags(symbol.value.symbol, flags);
      symbolTable.setSection(symbol.value.symbol, "ABS");
      symbolTable.setSymbolValue(symbol.value.symbol, value);
      if(valueKnown && symbolTable.checkForwardReferences(symbol.value.symbol)){
        resolveForwardReferences(symbol.value.symbol);
      }
    }
    else{
      symbolTable.addEntry(symbol.value.symbol, value, flags, "ABS");
    }

    return 0;
}

int SinglePassAssembler::handleEndDirective(Directive& directive) {
    // cout << "END" << endl;
    finished = true;
    resolveLiteralPool();
    return 0;
}




int createInstruction(int opcode, int mod, int regA, int regB, int regC, int disp){
  int val = 0;
  SET_OPCODE(val, opcode);
  SET_MOD(val, mod);
  SET_REGA(val, regA);
  SET_REGB(val, regB);
  SET_REGC(val, regC);
  SET_DISP(val, disp);
  return val;
}


//Handleri za instrkucije
int SinglePassAssembler::handleHaltInstruction(Instruction& instruction) {
    // cout << "HALT" << endl;

    int32_t instructionValue = createInstruction(OPC_HALT, 0, 0, 0, 0, 0);

    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleIntInstruction(Instruction& instruction) {
    // cout << "INT" << endl;

    int32_t instructionValue = createInstruction(OPC_INT, 0, 0, 0, 0, 0);

    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleIretInstruction(Instruction& instruction) {
    // cout << "IRET" << endl;

    //zapravo pop pc, pop status, ne sme prvo PC da se menja tako da SP + 8, LOAD STATUS, LOAD PSW
    int32_t instructionValue = createInstruction(OPC_LOAD, LOAD_REG, REG_SP, REG_SP, 0, 8);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
    instructionValue = createInstruction(OPC_LOAD, LOAD_CREGIND, CREG_STATUS, REG_SP, 0, -4);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
    instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, REG_PC, REG_SP, 0, -8);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
    

    return 0;
}

int SinglePassAssembler::handleCallInstruction(Instruction& instruction) {
    // cout << "CALL" << endl;

    //push pc prvo
    // int32_t instructionValue = createInstruction(OPC_STORE, STORE_PUSH, REG_SP, 0, REG_PC, -4);
    // writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

  
    //zatim skok
    Operand operand = instruction.op;

    switch(operand.type){
      case OP_PC_LIT:
        PCLiteralInstruction(operand.value.imm.value.literal, OPC_CALL, CALL_REGS, CALL_MEM, 0, 0, 0);
        break;

      case OP_PC_SYM:
        PCSymbolInstruction(operand.value.imm.value.symbol, OPC_CALL, CALL_REGS, CALL_MEM, 0, 0, 0);
        break;
    }

    return 0;
}

int SinglePassAssembler::handleRetInstruction(Instruction& instruction) {
    // cout << "RET" << endl;

    //zapravo pop pc
    int32_t instructionValue = createInstruction(OPC_LOAD, LOAD_POP, REG_PC, REG_SP, 0, 4);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleJmpInstruction(Instruction& instruction) {
    // cout << "JMP" << endl;

    Operand operand = instruction.op;

    switch(operand.type){
      case OP_PC_LIT:
        PCLiteralInstruction(operand.value.imm.value.literal, OPC_JMP, JMP_REG, JMP_MEM, 0, 0, 0);
        break;
      case OP_PC_SYM:
        PCSymbolInstruction(operand.value.imm.value.symbol, OPC_JMP, JMP_REG, JMP_MEM, 0, 0, 0);
        break;
    }

    return 0;
}

int SinglePassAssembler::handleBeqInstruction(Instruction& instruction) {
    // cout << "BEQ" << endl;

    Operand operand = instruction.op;
    int32_t regB = instruction.regSrc;
    int32_t regC = instruction.regDst;

    switch(operand.type){
      case OP_PC_LIT:
        PCLiteralInstruction(operand.value.imm.value.literal, OPC_JMP, JMP_REG_EQ, JMP_MEM_EQ, 0, regB, regC);
        break;
      case OP_PC_SYM:
        PCSymbolInstruction(operand.value.imm.value.symbol, OPC_JMP, JMP_REG_EQ, JMP_MEM_EQ, 0, regB, regC);
        break;
    }

    return 0;
}

int SinglePassAssembler::handleBneInstruction(Instruction& instruction) {
    // cout << "BNE" << endl;

    Operand operand = instruction.op;
    int32_t regB = instruction.regSrc;
    int32_t regC = instruction.regDst;

    switch(operand.type){
      case OP_PC_LIT:
        PCLiteralInstruction(operand.value.imm.value.literal, OPC_JMP, JMP_REG_NE, JMP_MEM_NE, 0, regB, regC);
        break;
      case OP_PC_SYM:
        PCSymbolInstruction(operand.value.imm.value.symbol, OPC_JMP, JMP_REG_NE, JMP_MEM_NE, 0, regB, regC);
        break;
    }

    return 0;
}

int SinglePassAssembler::handleBgtInstruction(Instruction& instruction) {
    // cout << "BGT" << endl;

    Operand operand = instruction.op;
    int32_t regB = instruction.regSrc;
    int32_t regC = instruction.regDst;

    switch(operand.type){
      case OP_PC_LIT:
        PCLiteralInstruction(operand.value.imm.value.literal, OPC_JMP, JMP_REG_GT, JMP_MEM_GT, 0, regB, regC);
        break;
      case OP_PC_SYM:
        PCSymbolInstruction(operand.value.imm.value.symbol, OPC_JMP, JMP_REG_GT, JMP_MEM_GT, 0, regB, regC);
        break;
    }

    return 0;
}

int SinglePassAssembler::handlePushInstruction(Instruction& instruction) {
    // cout << "PUSH" << endl;

    int regC = instruction.regSrc;

    int32_t instructionValue = createInstruction(OPC_STORE, STORE_PUSH, REG_SP, 0, regC, -4);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handlePopInstruction(Instruction& instruction) {
    // cout << "POP" << endl;

    int regA = instruction.regDst;
    int32_t instructionValue = createInstruction(OPC_LOAD, LOAD_POP, regA, REG_SP, 0, 4);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleXchgInstruction(Instruction& instruction) {
    // cout << "XCHG" << endl;

    int regB = instruction.regSrc;
    int regC = instruction.regDst;
    int32_t instructionValue = createInstruction(OPC_XCHG, 0, 0, regB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleAddInstruction(Instruction& instruction) {
    // cout << "ADD" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_ARITH, ARITH_ADD, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleSubInstruction(Instruction& instruction) {
    // cout << "SUB" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_ARITH, ARITH_SUB, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleMulInstruction(Instruction& instruction) {
    // cout << "MUL" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_ARITH, ARITH_MUL, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleDivInstruction(Instruction& instruction) {
    // cout << "DIV" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_ARITH, ARITH_DIV, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleNotInstruction(Instruction& instruction) {
    // cout << "NOT" << endl;

    int regAB = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_LOGIC, LOGIC_NOT, regAB, regAB, 0, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleAndInstruction(Instruction& instruction) {
    // cout << "AND" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_LOGIC, LOGIC_AND, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleOrInstruction(Instruction& instruction) {
    // cout << "OR" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_LOGIC, LOGIC_OR, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleXorInstruction(Instruction& instruction) {
    // cout << "XOR" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_LOGIC, LOGIC_XOR, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleShlInstruction(Instruction& instruction) {
    // cout << "SHL" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_SHIFT, SHIFT_L, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);


    return 0;
}

int SinglePassAssembler::handleShrInstruction(Instruction& instruction) {
    // cout << "SHR" << endl;

    int regAB = instruction.regDst;
    int regC = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_SHIFT, SHIFT_R, regAB, regAB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleLdInstruction(Instruction& instruction) {
    // cout << "LD" << endl;

    Operand operand = instruction.op;
    int regA = instruction.regDst;
    int regB;
    int regC;
    int disp;
    int32_t instructionValue;
    int32_t value;
    string symbol;
    char* cstr;

    switch (operand.type)
    {
    case OP_REG:
      regB = operand.value.reg;
      instructionValue = createInstruction(OPC_LOAD, LOAD_REG, regA, regB, 0, 0);
      writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      break;
    case OP_REG_IND:
      regB = operand.value.reg;
      instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, regB, 0, 0);
      writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      break;
    case OP_IMM_LIT:
      value = operand.value.imm.value.literal;
      if(checkIn12bitsRange(value)){
        instructionValue = createInstruction(OPC_LOAD, LOAD_REG, regA, 0, 0, value);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else{
        //pool literala
        addLiteralToPool(value);

        instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, REG_PC, 0, 0);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      break;
    case OP_IMM_SYM:
      symbol = operand.value.imm.value.symbol;
      if(symbolTable.checkDefined(symbol) && symbolTable.checkSymbolInSection(symbol, sectionsNames.back())){
        value = symbolTable.getSymbolValue(symbol);
        disp = value - sectionsContent.back().size() - 4;
        if(checkIn12bitsRange(disp)){
          instructionValue = createInstruction(OPC_LOAD, LOAD_REG, regA, REG_PC, 0, disp);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
        }
        else{
          //pool simbola
          addSymbolToPool(symbol);

          instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, REG_PC, 0, 0);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
        }
      }
      // i za abs (equ) simbole moze bez pool mozda (samo ako odma znamo vrednost i garantujemo da je manja od 12bit)
      else if(symbolTable.checkCalculated(symbol)){
        value = symbolTable.getSymbolValue(symbol);
        if(checkIn12bitsRange(value)){
          instructionValue = createInstruction(OPC_LOAD, LOAD_REG, regA, 0, 0, value);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
        }
        else{
          //pool literala
          addLiteralToPool(value);

          instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, REG_PC, 0, 0);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
        }
      }
      else{
        //pool simbola
        addSymbolToPool(symbol);

        instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, REG_PC, 0, 0);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      break;
    case OP_MEM_LIT:
      
      value = operand.value.mem.value.literal;
      
      if(checkIn12bitsRange(value)){
        instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, 0, 0, value);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else{
        //isto kao OP_IMM_LIT pa onda OP_REG_IND
        operand.type = OP_IMM_LIT;
        operand.value.imm.value.literal = value;
        instruction.op = operand;
        handleLdInstruction(instruction);

        operand.type = OP_REG_IND;
        operand.value.reg = regA;
        instruction.op = operand;
        handleLdInstruction(instruction);
      }

      break;
    case OP_MEM_SYM:
      symbol = operand.value.mem.value.symbol;

      if(symbolTable.checkDefined(symbol) && symbolTable.checkSymbolInSection(symbol, sectionsNames.back())){
        value = symbolTable.getSymbolValue(symbol);
        disp = value - sectionsContent.back().size() - 4;
        if(checkIn12bitsRange(disp)){
          instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, REG_PC, 0, disp);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
          return 0;
        }
      }
      //mora u pool simbola, ponasa se kao OP_IMM_SYM pa OP_REG_IND
      operand.type = OP_IMM_SYM;
      cstr = new char[symbol.length() + 1];
      strcpy(cstr, symbol.c_str());
      operand.value.imm.value.symbol = cstr;
      instruction.op = operand;
      handleLdInstruction(instruction);
      free(cstr);

      operand.type = OP_REG_IND;
      operand.value.reg = regA;
      instruction.op = operand;
      handleLdInstruction(instruction);

      break;
    case OP_MEM_OFF_LIT:
      value = operand.value.memOff.offset.value.literal;
      regB = operand.value.memOff.reg;

      if(checkIn12bitsRange(value)){
        instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, regB, 0, value);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else{
        //greska, preveliki offset
        cout << "ERROR: offset too big" << endl;
        return 4;
      }

      break;
    case OP_MEM_OFF_SYM:
      symbol = operand.value.memOff.offset.value.symbol;
      regB = operand.value.memOff.reg;

      // za equ symbol jedino mozda nije greska, ali vodi racuna da mozda equ jos nije def
      if(symbolTable.checkCalculated(symbol)){
        value = symbolTable.getSymbolValue(symbol);
        if(checkIn12bitsRange(value)){
          instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, regB, 0, value);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
          return 0;
        }
        cout << "ERROR: symbol:" << symbol <<" used for offset is too big or unknown!" << endl;
        return 5;
      }
      if(symbolTable.checkAbsolute(symbol)){
        addRelocation(symbol, RelocationType::EQU_DISP);

        instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, regB, 0, 0);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else if(!symbolTable.checkDefined(symbol)){
        addRelocation(symbol, RelocationType::EQU_DISP);

        instructionValue = createInstruction(OPC_LOAD, LOAD_REGIND, regA, regB, 0, 0);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else{
        cout << "ERROR: symbol:" << symbol <<" used for offset is too big or unknown!" << endl;
        return 5; //greska, symbol nije poznat u trenutku asembliranja ili ne staje u 12bit
      }

      break;
    }

    return 0;
}

int SinglePassAssembler::handleStInstruction(Instruction& instruction) {
    // cout << "ST" << endl;

    Operand operand = instruction.op;
    int regA;
    int regB;
    int regC = instruction.regSrc;
    int disp;
    int32_t instructionValue;
    int32_t value;
    string symbol;

    switch (operand.type)
    {
    case OP_REG:
      cout << "ERROR: invalid operand for ST" << endl;
      return 6; //greska, nevalidan operand za STORE
      break;
    case OP_REG_IND:
      regA = operand.value.reg;
      instructionValue = createInstruction(OPC_STORE, STORE_MEMDIR, regA, 0, regC, 0);
      writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      break;
    case OP_IMM_LIT:
      cout << "ERROR: invalid operand for ST" << endl;
      return 6; //greska, nevalidan operand za STORE
      break;
    case OP_IMM_SYM:
      cout << "ERROR: invalid operand for ST" << endl;
      return 6; //greska, nevalidan operand za STORE
      break;
    case OP_MEM_LIT:
      value = operand.value.mem.value.literal;
      if(checkIn12bitsRange(value)){
        instructionValue = createInstruction(OPC_STORE, STORE_MEMDIR, 0, 0, regC, value);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else{
        //morace u bazen literal
        addLiteralToPool(value);

        instructionValue = createInstruction(OPC_STORE, STORE_MEMIND, REG_PC, 0, regC, 0);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      break;
    case OP_MEM_SYM:
      symbol = operand.value.mem.value.symbol;
      if(symbolTable.checkDefined(symbol) && symbolTable.checkSymbolInSection(symbol, sectionsNames.back())){
        value = symbolTable.getSymbolValue(symbol);
        disp = value - sectionsContent.back().size() - 4;
        if(checkIn12bitsRange(disp)){
          instructionValue = createInstruction(OPC_STORE, STORE_MEMDIR, regA, 0, regC, disp);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
          return 0;
        }
      }
      //inace, morace u bazen simbola, TODO: ne mora za equ mozda
      addSymbolToPool(symbol);

      instructionValue = createInstruction(OPC_STORE, STORE_MEMIND, REG_PC, 0, regC, 0);
      writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      break;
    case OP_MEM_OFF_LIT:
      value = operand.value.memOff.offset.value.literal;
      regA = operand.value.memOff.reg;

      if(checkIn12bitsRange(value)){
        instructionValue = createInstruction(OPC_STORE, STORE_MEMDIR, regA, 0, regC, value);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else{
        //greska, preveliki offset
        cout << "ERROR: offset too big" << endl;
        return 4;
      }

      break;
    case OP_MEM_OFF_SYM:
      symbol = operand.value.memOff.offset.value.symbol;
      regA = operand.value.memOff.reg;

      // za equ symbol jedino mozda nije greska, ali vodi racuna da mozda equ jos nije def
      if(symbolTable.checkCalculated(symbol)){
        value = symbolTable.getSymbolValue(symbol);
        if(checkIn12bitsRange(value)){
          instructionValue = createInstruction(OPC_STORE, STORE_MEMDIR, regA, 0, regC, value);
          writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
          return 0;
        }
        cout << "ERROR: symbol:" << symbol <<" used for offset is too big or unknown!" << endl;
        return 5;
      }
      if(symbolTable.checkAbsolute(symbol)){
        addRelocation(symbol, RelocationType::EQU_DISP);

        instructionValue = createInstruction(OPC_STORE, STORE_MEMDIR, regA, 0, regC, 0);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else if(!symbolTable.checkDefined(symbol)){
        addRelocation(symbol, RelocationType::EQU_DISP);

        instructionValue = createInstruction(OPC_STORE, STORE_MEMDIR, regA, 0, regC, 0);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
      }
      else{
        cout << "ERROR: symbol:" << symbol <<" used for offset is too big or unknown!" << endl;
        return 5; //greska, symbol nije poznat u trenutku asembliranja ili ne staje u 12bit
      }

      break;
    }

    return 0;
}

int SinglePassAssembler::handleCsrrdInstruction(Instruction& instruction) {
    // cout << "CSRRD" << endl;

    int regA = instruction.regDst;
    int regB = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_LOAD, LOAD_CSRRD, regA, regB, 0, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}

int SinglePassAssembler::handleCsrwrInstruction(Instruction& instruction) {
    // cout << "CSRWR" << endl;

    int regA = instruction.regDst;
    int regB = instruction.regSrc;
    int32_t instructionValue = createInstruction(OPC_LOAD, LOAD_CSRWR, regA, regB, 0, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);

    return 0;
}






void SinglePassAssembler::PCLiteralInstruction(int32_t value, int opcode, int mode1, int mode2, int regA, int regB, int regC){

    int disp;
    int instructionValue;

    if(checkIn12bitsRange(value)){
      disp = value;
      instructionValue = createInstruction(opcode, mode1, regA, regB, regC, disp);
      writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
    }
    else{
      //pool literala
      addLiteralToPool(value);

      instructionValue = createInstruction(opcode, mode2, REG_PC, regB, regC, 0);
      writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
    }

}

void SinglePassAssembler::PCSymbolInstruction(string symbol, int opcode, int mode1, int mode2, int regA, int regB, int regC){

    int disp;
    int instructionValue;

    if(symbolTable.checkDefined(symbol) && symbolTable.checkSymbolInSection(symbol, sectionsNames.back())){

      int32_t value = symbolTable.getSymbolValue(symbol);

      int32_t pcRel = value - sectionsContent.back().size() - 4;

      if(checkIn12bitsRange(pcRel)){

        instructionValue = createInstruction(opcode, mode1, REG_PC, regB, regC, pcRel);
        writeInstructionToSection(instructionValue, sectionsNames.size() - 1);
        return;

      }
    }

    //pool symbola
    addSymbolToPool(symbol);

    instructionValue = createInstruction(opcode, mode2, REG_PC, regB, regC, 0);
    writeInstructionToSection(instructionValue, sectionsNames.size() - 1);


}







bool SinglePassAssembler::checkIn12bitsRange(int32_t value) const{
  return value >= -2048 && value <= 2047;
}

void SinglePassAssembler::addLiteralToPool(int32_t value){

  if (literalPool.find(value) != literalPool.end()) {
    literalPool[value].push_back(sectionsContent.back().size());
    return;
  } else {
    literalPool[value] = {(int32_t)(sectionsContent.back().size())};
  }

  if(firstForPoolAt == -1)firstForPoolAt = sectionsContent.back().size();
  SymbolLiteral lit;
  lit.isSymbol = false;
  lit.value.literal = value;
  orderInPool.push_back(lit);

}

void SinglePassAssembler::addSymbolToPool(string symbol){

  if (symbolPool.find(symbol) != symbolPool.end()) {
    symbolPool[symbol].push_back(sectionsContent.back().size());
    return;
  } else {
    symbolPool[symbol] = {(int32_t)(sectionsContent.back().size())};
  }

  if(firstForPoolAt == -1)firstForPoolAt = sectionsContent.back().size();

  SymbolLiteral sym;
  sym.isSymbol = true;
  char* cstr = new char[symbol.length() + 1];
  strcpy(cstr, symbol.c_str());
  sym.value.symbol = cstr;
  orderInPool.push_back(sym);

}

void SinglePassAssembler::resolveLiteralPool(){

  int i = 0;

  while(i < orderInPool.size()){

    SymbolLiteral symlit = orderInPool[i];

    if(!symlit.isSymbol){

      int32_t value = symlit.value.literal;

      for(int32_t offset : literalPool[value]){ //za svako mesto odakle se referencirao ovaj literal upisujemo pomeraj do njega u DISP te instrukcije
        int32_t instruction = readInstructionFromSection(sectionsNames.size() - 1, offset);

        SET_DISP(instruction, sectionsContent.back().size() - offset - 4); //pomeraj do literala u odnosu na adresu instrukcije

        writeInstructionToSection(instruction, sectionsNames.size() - 1, offset);
      }

      write32bitToSection(value, sectionsNames.size() - 1); //upisemo vrednost na kraj sekcije u bazen
    }


    else{
      string symbol = symlit.value.symbol;

      free(symlit.value.symbol);

      //treba dodati mzd u tabelu sym i dodati forward reference
      int32_t value = 0;

      if(!symbolTable.checkSymbol(symbol)){
        symbolTable.addEntry(symbol, 0, 0, "UND");//kad bude definisian postavice se sekcija
        symbolTable.addForwardReference(symbol, this->sectionsContent.back().size(), this->sectionsNames.back());
      }
      else{
        if(!symbolTable.checkDefined(symbol)){
          symbolTable.addForwardReference(symbol, this->sectionsContent.back().size(), this->sectionsNames.back());//dodati forward referencu
        }
        else if(symbolTable.checkAbsolute(symbol) && !symbolTable.checkCalculated(symbol)){
          symbolTable.addForwardReference(symbol, this->sectionsContent.back().size(), this->sectionsNames.back());//dodati forward referencu
        }
        //TODO: ako je definisan u istoj sekciji kasnije mozda ne treba u pool, AL TO PREVISE MOZE DA MENJA (mozda nam vise ne treba 2 instrukcije nego 1 za nesto od ranije)
      }  

      if(symbolTable.checkDefined(symbol) && symbolTable.checkCalculated(symbol)){ //tad ne treba relok zapis
        value = symbolTable.getSymbolValue(symbol); 
      }
      else if(symbolTable.checkDefined(symbol)){//inace smo dodali forward ref pa ce tab biti dodat reloaction
        addRelocation(symbol, RelocationType::ABSOLUTE);
      }

      for(int32_t offset : symbolPool[symbol]){ //za svako mesto odakle se referencirao ovaj simbol upisujemo pomeraj do njega u DISP te instrukcije
        int32_t instruction = readInstructionFromSection(sectionsNames.size() - 1, offset);

        int32_t pcRel = sectionsContent.back().size() - offset - 4;

        SET_DISP(instruction, pcRel); //pomeraj do simbola u odnosu na adresu instrukcije

        writeInstructionToSection(instruction, sectionsNames.size() - 1, offset);
      }

      write32bitToSection(value, sectionsNames.size() - 1); //upisemo vrednost na kraj sekcije u bazen
    }

    i++;

  }


  literalPool.clear();
  symbolPool.clear();
  orderInPool.clear();
  firstForPoolAt = -1;

}



bool SinglePassAssembler::checkPoolNeccessary(int32_t value) {

  return firstForPoolAt != -1 && !checkIn12bitsRange(sectionsContent.back().size() + value + 4 - firstForPoolAt - 4);
}



void SinglePassAssembler::resolveForwardReferences(string symbol){
  //  ova funkcija treba da resi sve forward reference za dati simbol
  ST_forwardReference* current = symbolTable.getForwardReferences(symbol);

  while(current != nullptr){
    int32_t value = 0;
                                    //ovde moze da ostane i absolute (a ne calculated) jer znamo da ce se za absolute simbole resolve pozivati samo kad su calculated
    if(symbolTable.checkDefined(symbol) && symbolTable.checkAbsolute(symbol)){ //tad ne treba relok zapis
      value = symbolTable.getSymbolValue(symbol); 
      //upis vrednosti simbola u content sekcije (4 bajta)
      write32bitToSection(value, getSectionIndex(symbolTable.getSymbolName(current->section)), current->address);
    }
    else{
      string section = symbolTable.getSymbolName(current->section);
      addRelocation(symbol, RelocationType::ABSOLUTE, section, current->address);
    }

    current = current->next;
  }

  symbolTable.clearForwardReferences(symbol);
}


void SinglePassAssembler::addRelocation(string symbol, RelocationType type, string section, int index){ //inicijalno se dodaje relokacija kao da je symbol globalan
  uint16_t symIndex = symbolTable.getSymbolIndex(symbol);
  if(index == -1)
    relocationTables[sectionsNames.size() - 1].addEntry(sectionsContent.back().size(), symIndex, type);
  else{
    int sectionIndex = getSectionIndex(section);
    relocationTables[sectionIndex].addEntry(index, symIndex, type);
  }
}

int SinglePassAssembler::getSectionIndex(string section){
  for(int i = 0; i < sectionsNames.size(); i++){
    if(sectionsNames[i] == section){
      return i;
    }
  }
  return -1;
}



int SinglePassAssembler::finalizeRelocationTables(){
  // Mozda neka relkacija ne treba ako je kasnije definisana vrednost apsolutnog simbola
  // takodje posebne relokacije EQU_DISP za [reg + symbol] adresiranje koristim
  //Takodje mozda neke relokacije nisu u odnosu na global simbol nego u odnsou na sekciju pa to menjamo

  for(int i = 0 ; i < relocationTables.size() ; i++){

    RelocationTable& relocationTable = relocationTables[i];

    if (relocationTable.checkEmpty()) continue;

    vector<RelocationTableEntry>& entries = relocationTable.getTable();

    for (auto it = entries.begin(); it != entries.end(); ) {

      RelocationTableEntry& entry = *it;
      string symbol = symbolTable.getSymbolName(entry.symbol);

      if(symbolTable.checkCalculated(symbol)){//apsolutan sumbl za koji ipak ne treba relok (prvobitno je bio relok jer smo tek kasnije saznali da je abs)
        
        if(entry.type == RelocationType::EQU_DISP){
          
          int32_t value = symbolTable.getSymbolValue(symbol);
          if(checkIn12bitsRange(value)){
            int32_t instruction = readInstructionFromSection(i, entry.offset);
            SET_DISP(instruction, value);
            writeInstructionToSection(instruction, i, entry.offset);
          }
          else{
            cout << "ERROR: symbol:" << symbol <<" used for offset is too big or unknown!" << endl;
            return 5;
          }
          
        }
        
        it = entries.erase(it); 
      }
      else{
        if(!symbolTable.checkGlobal(symbol)){
          uint16_t section = symbolTable.getSymbolSection(symbol);
          entry.symbol = section;
          entry.addend = symbolTable.getSymbolValue(symbol);
        }
        it++;
      }
    }
  }

  return 0;

}

int SinglePassAssembler::nonEmptyRelocationTables(){
  int cnt = 0;
  for(int i = 0 ; i < relocationTables.size() ; i++){
    if(!relocationTables[i].checkEmpty()) cnt++;
  }
  return cnt;
}


int SinglePassAssembler::resolveEquTable(){

  unordered_map<string, vector<equTableEntry>>& uncalculatedSymbols = equTable.getTable();
  vector<string>& names = equTable.getSymbolNames();

  while(!equTable.allResolved()){

    bool atLeastOneResolved = false;

    for(int i = 0 ; i < names.size() ; ){

      vector<equTableEntry>& entries = uncalculatedSymbols[names[i]];

      uint16_t section = 0;
      uint16_t plusMinus = 0; //has to be all same section and equal plus and minus to be absolute

      for (auto it = entries.begin(); it != entries.end(); ) {
          equTableEntry& entry = *it;
          if (symbolTable.checkSymbol(entry.symbol)) {
              if (symbolTable.checkCalculated(entry.symbol)) {
                  int32_t oldValue = symbolTable.getSymbolValue(names[i]);
                  int32_t newValue = oldValue + symbolTable.getSymbolValue(entry.symbol) * entry.sign;
                  symbolTable.setSymbolValue(names[i], newValue);
                  it = entries.erase(it);  // erase returns the iterator following the last removed element
              } else if(symbolTable.checkAbsolute(entry.symbol)){
                  ++it;  // only increment here
              }
              else{
                uint16_t sec = symbolTable.getSymbolSection(entry.symbol);

                if(symbolTable.getSymbolName(sec) == "UND"){
                  cout << "ERROR: Undefined symbol used for equ directive!" << endl;
                  return 7; //ERROR: symbol not defined
                }

                if(section == 0){//u section pamtmimo iz koje sekcije su neaposlutni simboli
                  section = sec;
                }

                if(sec != section){
                  cout << "ERROR: Not aboslute value of equ symbol!" << endl;
                  return 8; //ERROR: not absolute symbol in equ
                }
                plusMinus += entry.sign;
                int32_t oldValue = symbolTable.getSymbolValue(names[i]);
                int32_t newValue = oldValue + symbolTable.getSymbolValue(entry.symbol) * entry.sign;
                symbolTable.setSymbolValue(names[i], newValue);
                it = entries.erase(it);  // erase returns the iterator following the last removed element
              }
          }
          else{
            cout << "ERROR: Undefined symbol used for equ directive!" << endl;
            return 7; //ERROR: symbol not defined
          }
      }

      if(section != 0 && plusMinus == 0){
        symbolTable.setFlags(names[i], SYM_ABSOLUTE);
      }
      else if(section != 0){
        cout << "ERROR: Not aboslute value of equ symbol!" << endl;
        return 8;
      }

      if(entries.size() == 0){
        atLeastOneResolved = true;
        uncalculatedSymbols.erase(names[i]);

        symbolTable.setFlags(names[i], SYM_CALCULATED);
        resolveForwardReferences(names[i]);

        names.erase(names.begin() + i);
      }
      else{
        i++;
      }

    }

    if(!atLeastOneResolved){
      cout << "ERROR: Circular dependency, couldnt resolve all equ symbols!" << endl;
      return 9; //ERROR: circular dependency, couldnt resolve all equ symbols
    }

  }

  return 0;

}