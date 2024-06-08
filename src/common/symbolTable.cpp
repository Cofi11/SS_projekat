#include "../../inc/common/symbolTable.hpp"
#include <iostream>
#include <iomanip>
using namespace std;

void SymbolTable::addEntry(string symbol, int32_t value, int flags, string section){
  SymbolTableEntry entry;
  entry.symbol = symbol;
  entry.value = value;
  entry.flags = flags;

  symbolIndex[symbol] = table.size();

  entry.section = symbolIndex[section];

  // if(flags & SYM_DEFINED)
    entry.forwardReferences = nullptr;
  // else{
  //   entry.forwardReferences = new ST_forwardReference(value, entry.section);
  // }

  table.push_back(entry);
  symbolNames.push_back(symbol);
}

void SymbolTable::addForwardReference(string symbol, int32_t address, string section){
  ST_forwardReference* oldReferences = table[symbolIndex[symbol]].forwardReferences;
  ST_forwardReference* newReference = new ST_forwardReference(address, symbolIndex[section], oldReferences);
  table[symbolIndex[symbol]].forwardReferences = newReference;
}

void SymbolTable::clearForwardReferences(string symbol){
  ST_forwardReference* current = table[symbolIndex.at(symbol)].forwardReferences;
  ST_forwardReference* next;
  while(current != nullptr){
    next = current->next;
    delete current;
    current = next;
  }
  table[symbolIndex.at(symbol)].forwardReferences = nullptr;
}

SymbolTable::~SymbolTable(){
  for(SymbolTableEntry& entry : table){
    ST_forwardReference* current = entry.forwardReferences;
    ST_forwardReference* next;
    while(current != nullptr){
      // cout<<"Deleting forward reference...";
      next = current->next;
      delete current;
      current = next;
    }
  }
}


void SymbolTable::printTable() const{
  cout << "***** SYMBOL TABLE *****" << endl;
  int i = 0;
  for(const SymbolTableEntry& entry : table){
    cout << setw(3) << i++ << ": ";
    cout << setw(8) << setfill('0') << hex << entry.value << " " << setfill(' ') << dec ;
    if(entry.flags & SYM_GLOBAL) cout << " G ";
    else cout << " L ";
    cout << setw(3) << entry.section; 
    cout << " ("<< table[entry.section].symbol << ") ";
    cout << entry.symbol << endl;
    // ST_forwardReference* current = entry.forwardReferences;
    // while(current != nullptr){
    //   cout << "|  " << current->address << " " << current->section << "  |  ";
    //   current = current->next;
    // }
  }
  cout << endl;
}




void SymbolTable::writeTableToOutput(ostream& output) const{

  string header = "__SYMTAB__";
  writeString(output, header);
  write(output, (uint32_t)table.size());
  for(const SymbolTableEntry& entry : table){
    writeString(output, entry.symbol);
    write(output, entry.value);
    write(output, entry.flags);
    write(output, entry.section);
  }

}

void SymbolTable::readTableFromInput(istream& input){
  // string header;
  // readString(input, header);
  // if(header != "__SYMTAB__"){
  //   cout << "Error: Invalid symbol table header" << endl;
  //   return;
  // }
  uint32_t size;
  read(input, size);
  table.clear();
  for(int i = 0; i < size; i++){
    SymbolTableEntry entry;
    readString(input, entry.symbol);
    read(input, entry.value);
    read(input, entry.flags);
    read(input, entry.section);
    entry.forwardReferences = nullptr;
    symbolIndex[entry.symbol] = table.size();
    table.push_back(entry);
    symbolNames.push_back(entry.symbol);
  }
}




int SymbolTable::mergeTwoTables(SymbolTable& otherTable, unordered_map<string, int32_t>& sectionsSizes){

  unordered_map<uint16_t, uint16_t> sectionToSection; //sekcije mogu dobiti novi indeks
  vector<string> unfinishedSymbols;

  for(SymbolTableEntry& sym : otherTable.table){

      //mozda lokalne simbole koji nisu sekcije treab preimonvati jer moze u vise fajlvoa isto
      if(!(sym.flags & SYM_GLOBAL) && !(sym.flags & SYM_SECTION)){
        string newSymbol = sym.symbol + "_" + to_string(table.size());
        sym.symbol = newSymbol;
      }

      if(symbolIndex.find(sym.symbol) == symbolIndex.end()){//ne postoji isti simbol vec
        addEntry(sym.symbol, sym.value, sym.flags, "UND");//ne mozemo odmah znati u kojoj ce sekciji biti jer je mozda njegova sekcija tek kasnije bila definisana
        setSectionIndex(sym.symbol, sym.section);

        if(sym.flags & SYM_SECTION){
          sectionToSection[otherTable.symbolIndex[sym.symbol]] = table.size() - 1;
        }

        string secName = otherTable.symbolNames[sym.section];
        if(secName == "ABS" || secName == "UND") continue;

        unfinishedSymbols.push_back(sym.symbol);

      }
      else{//ovde ce moci samo globalni jer lokalnima sam gore promenio ime da bude jedinstveno
        SymbolTableEntry& current = table[symbolIndex[sym.symbol]];
        
        if(!(sym.flags & SYM_SECTION) && !(current.flags & SYM_SECTION)){//dva obicna simbola, nisu sekcije
          if(symbolNames[current.section] != "UND" && otherTable.symbolNames[sym.section] != "UND"){
            cout << "ERROR: multiple definitions of symbol " << sym.symbol << endl;
            return 1; //LINKER ERROR: multiple definitions of symbol
          }

          if(symbolNames[current.section] == "UND" && otherTable.symbolNames[sym.section] != "UND"){
            current.flags = sym.flags;
            current.value = sym.value;
            current.section = sym.section; //svakako se sekcija mora updateovati mozda

            if(!(current.flags & SYM_ABSOLUTE)){
              unfinishedSymbols.push_back(sym.symbol);
            }

          }

        }
        else if((sym.flags & SYM_SECTION) && (current.flags & SYM_SECTION)){//dve sekcije sa istim imenom
          // if(sym.symbol == "ABS" || sym.symbol == "UND") continue;

          sectionToSection[sym.section] = current.section;

        }
        else if(sym.flags & SYM_SECTION){//simbol je sekcija, a drugi nije
          if(symbolNames[current.section] != "UND"){
            cout << "ERROR: symbol " << sym.symbol << " is section in file, but not in the other" << endl;
            return 2; //LINKER ERROR: one symbol is section, other is not
          }

          current.flags = sym.flags;
          current.value = sym.value; //svakako 0!
          current.section = sym.section;

          if(!(current.flags & SYM_ABSOLUTE)){
            unfinishedSymbols.push_back(sym.symbol);
          }

        }
        else{//simbol nije sekcija, a drugi jeste
          if(otherTable.symbolNames[sym.section] != "UND"){
            cout << "ERROR: symbol " << sym.symbol << " is section in file, but not in the other" << endl;
            return 2; //LINKER ERROR: one symbol is section, other is not
          }

          // if(!(current.flags & SYM_ABSOLUTE)){
          //   unfinishedSymbols.push_back(sym.symbol);
          // }

        }
      }
  }

  for(string& sym : unfinishedSymbols){
    SymbolTableEntry& current = table[symbolIndex[sym]];
    string secName = otherTable.symbolNames[current.section];
    current.section = sectionToSection[current.section];
    current.value += sectionsSizes[secName];
  }

  return 0;

}


int SymbolTable::updateGlobalSymbols(){
  for(SymbolTableEntry& sym : table){
    if(sym.flags & SYM_GLOBAL){
      if(symbolNames[sym.section] == "UND"){
        cout << "ERROR: global symbol is not defined" << endl;
        return 5; //LINKER ERROR: global symbol is not defined
      }
      else if(symbolNames[sym.section] != "ABS"){//ako nije ABS menja mu se vrednost (nije relative u odnosu na sekciju vise, nego je VA)
        sym.value += table[sym.section].value;
      }
    }
  }
  return 0;
}