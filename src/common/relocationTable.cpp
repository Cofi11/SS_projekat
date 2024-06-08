#include "../../inc/common/relocationTable.hpp"


void RelocationTable::addEntry(int32_t offset, uint16_t symbol, RelocationType type, int32_t addend){
  RelocationTableEntry entry;
  entry.offset = offset;
  entry.symbol = symbol;
  entry.type = type;
  entry.addend = addend;

  table.push_back(entry);
}


void RelocationTable::printTable() const{
  int i = 0;
  for(const RelocationTableEntry& entry : table){
    cout << setw(3) << i++ << ": ";
    cout << setw(8) << setfill('0') << hex << entry.offset << setfill(' ') << dec << " ";
    cout << setw(3) << entry.symbol << " ";
    cout << "ABS "; //only relocation type
    cout << setw(8) << setfill('0') << hex  << entry.addend << setfill(' ') << dec << endl;
  }
}


void RelocationTable::writeTableToOutput(ostream& output, string name) const{
  string header = ".rela." + name;
  writeString(output, header);
  write(output, table);
}

void RelocationTable::readTableFromInput(istream& input){
  read(input, table);
}



void RelocationTable::mergeTwoTables(RelocationTable& table, int32_t offset){
  for(RelocationTableEntry& entry : table.getTable()){
    // RelocationTableEntry newEntry;
    // newEntry.offset = entry.offset + offset;
    // newEntry.symbol = entry.symbol;
    // newEntry.type = entry.type;
    // newEntry.addend = entry.addend;
    // this->table.push_back(newEntry);
    addEntry(entry.offset + offset, entry.symbol, entry.type, entry.addend);
  }
}

void RelocationTable::updateOffsets(int32_t offset){
  for(RelocationTableEntry& entry : table){
    entry.offset += offset;
  }
}