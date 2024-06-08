#ifndef RELOCATION_TABLE_HPP
#define RELOCATION_TABLE_HPP

#include "file_rw.hpp"
#include <vector>
#include <iostream>
#include <iomanip>
using namespace std;

enum RelocationType{ //MOZDA MI I NE TREBA TIP
  EQU_DISP,
  ABSOLUTE
};

struct RelocationTableEntry{
  int32_t offset;
  uint16_t symbol;
  RelocationType type;
  int32_t addend;
};

class RelocationTable{

private:
  vector<RelocationTableEntry> table;

public:
  
    // RelocationTable(){}
    // RelocationTable(const RelocationTable&) = delete;
    // RelocationTable& operator=(const RelocationTable&) = delete;

    void addEntry(int32_t offset, uint16_t symbol, RelocationType type, int32_t addend = 0);

    void printTable() const;

    bool checkEmpty() const{
      return table.empty();
    }

    vector<RelocationTableEntry>& getTable(){
      return table;
    }

    void writeTableToOutput(ostream& output, string name) const;

    void readTableFromInput(istream& input);

    void mergeTwoTables(RelocationTable& table, int32_t offset);

    void updateOffsets(int32_t offset);
  
    ~RelocationTable(){}

};

#endif