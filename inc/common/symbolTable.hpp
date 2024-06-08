#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <vector>
#include <unordered_map>
#include "file_rw.hpp"
using namespace std;

struct ST_forwardReference{
  int32_t address;
  uint16_t section;
  ST_forwardReference* next;

  ST_forwardReference(int32_t address, uint16_t section, ST_forwardReference* next = nullptr){
    this->address = address;
    this->section = section;
    this->next = next;
  }

};

const int SYM_GLOBAL = 1; //is global flag
const int SYM_DEFINED = 2; //is defined flag
const int SYM_SECTION = 4; //is a section flag
const int SYM_ABSOLUTE = 8; //is absolute flag
const int SYM_CALCULATED = 16; //is calculated flag (for equ absolute)

struct SymbolTableEntry{
  string symbol;
  int32_t value;
  int flags;
  uint16_t section;  //nece valjda biti vise od 2^16 sekcija
  // uint16_t size; //velicina u bajtovima, mozda i ne treba ako je sve 4B uvek

  ST_forwardReference* forwardReferences; //lista forward referenci
};

class SymbolTable{

private:
  vector<SymbolTableEntry> table;
  unordered_map<string, uint16_t> symbolIndex;
  vector<string> symbolNames;

public:
  
    SymbolTable(){}
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;
  
    void addEntry(string symbol, int32_t value, int flags, string section);
    void setFlags(string symbol, int flags){
      table[symbolIndex[symbol]].flags |= flags;
    }
    bool checkSymbol(string symbol) const{
      return symbolIndex.find(symbol) != symbolIndex.end();
    }
    bool checkDefined(string symbol) const{
      return symbolIndex.find(symbol) != symbolIndex.end() && (table[symbolIndex.at(symbol)].flags & SYM_DEFINED);
    }
    bool checkSection(string symbol) const{
      return symbolIndex.find(symbol) != symbolIndex.end() && (table[symbolIndex.at(symbol)].flags & SYM_SECTION);
    }
    bool checkSymbolInSection(string symbol, string section) const{
      return table[symbolIndex.at(symbol)].section == symbolIndex.at(section);
    }
    bool checkAbsolute(string symbol) const{
      return symbolIndex.find(symbol) != symbolIndex.end() && (table[symbolIndex.at(symbol)].flags & SYM_ABSOLUTE);
    }
    bool checkCalculated(string symbol) const{
      return symbolIndex.find(symbol) != symbolIndex.end() && (table[symbolIndex.at(symbol)].flags & SYM_ABSOLUTE) && (table[symbolIndex.at(symbol)].flags & SYM_CALCULATED);
    }
    bool checkGlobal(string symbol) const{
      return symbolIndex.find(symbol) != symbolIndex.end() && (table[symbolIndex.at(symbol)].flags & SYM_GLOBAL);
    }
    bool checkForwardReferences(string symbol) const{
      return table[symbolIndex.at(symbol)].forwardReferences != nullptr;
    }

    // bool checkUndefinedSymbols() {
    //   for(SymbolTableEntry& entry : table){
    //     if(!(entry.flags & SYM_DEFINED) && !(entry.flags & SYM_GLOBAL)){
    //       return true;
    //     }
    //     else if(!(entry.flags & SYM_DEFINED) && (entry.flags & SYM_GLOBAL)){
    //       entry.flags |= SYM_DEFINED;
    //       entry.section = symbolIndex["UND"];
    //     }
    //   }
    //   return false;
    // }
    vector<string>& getSymbolNames(){
      return symbolNames;
    }


    void addForwardReference(string symbol, int32_t address, string section);
    int32_t getSymbolValue(string symbol) const{
      return table[symbolIndex.at(symbol)].value;
    }
    void setSymbolValue(string symbol, int32_t value){
      table[symbolIndex.at(symbol)].value = value;
    }
    uint16_t getSymbolSection(string symbol) const{
      return table[symbolIndex.at(symbol)].section;
    }
    string getSymbolName(uint16_t index) const{
      return symbolNames[index];
    }
    uint16_t getSymbolIndex(string symbol) const{
      return symbolIndex.at(symbol);
    }
    void setSection(string symbol, string section){
      table[symbolIndex.at(symbol)].section = symbolIndex.at(section);
    }
    void setSectionIndex(string symbol, uint16_t section){
      table[symbolIndex.at(symbol)].section = section;
    }
    ST_forwardReference* getForwardReferences(string symbol) const{
      return table[symbolIndex.at(symbol)].forwardReferences;
    }
    void clearForwardReferences(string symbol);

    void printTable() const;

    void writeTableToOutput(ostream& output) const;

    void readTableFromInput(istream& input);

    int mergeTwoTables(SymbolTable& otherTable, unordered_map<string, int32_t>& sectionsSizes);

    int updateGlobalSymbols();

    // void freeTable();
  
    ~SymbolTable();

};

#endif