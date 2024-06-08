#ifndef EQU_TABLE_HPP
#define EQU_TABLE_HPP


struct equTableEntry{
  string symbol;
  int sign;
};

class EquTable{

private:
  unordered_map<string, vector<equTableEntry>> table;
  vector<string> symbolNames;

public:

    EquTable(){}
    EquTable(const EquTable&) = delete;
    EquTable& operator=(const EquTable&) = delete;
  
    void addEntry(string symbol, string equSymbol, int sign){
      equTableEntry entry;
      entry.symbol = equSymbol;
      entry.sign = sign;

      if(table.find(symbol) == table.end()){
        symbolNames.push_back(symbol);
        table[symbol] = vector<equTableEntry>();
      }

      table[symbol].push_back(entry);
    }

    void printTable() const{
      cout << "***** EQU TABLE *****" << endl;
      for(const auto& entry : table){
        cout << entry.first << ": ";
        for(const equTableEntry& equEntry : entry.second){
          cout << equEntry.symbol << " " << equEntry.sign << " | ";
        }
        cout << endl;
      }
    }

    bool allResolved(){
      return table.empty();
    }

    unordered_map<string, vector<equTableEntry>>& getTable(){
      return table;
    }

    vector<string>& getSymbolNames(){
      return symbolNames;
    }

    // bool checkSymbol(string symbol) const{
    //   return table.find(symbol) != table.end();
    // }

    // vector<equTableEntry>& getEntries(string symbol){
    //   return table[symbol];
    // }

    ~EquTable(){}

};


#endif