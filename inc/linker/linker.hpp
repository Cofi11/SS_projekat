#ifndef LINKER_HPP
#define LINKER_HPP

#include <iostream>
#include <vector>
#include <fstream>
#include "../../inc/structures.hpp"
#include "../../inc/common/file_rw.hpp"
#include "../../inc/common/symbolTable.hpp"
#include "../../inc/common/relocationTable.hpp"
using namespace std;

class Linker{

  private:
      
      SymbolTable symbolTable;
      vector<RelocationTable> relocationTables;
      vector<string> relocationSections; //sekcije za koje su nadjene relocation tabele

      vector<vector<uint8_t>> sectionsContent;
      vector<string> sectionsNames;
      
      Linker() {}
      Linker(const Linker&) = delete;
      Linker& operator=(const Linker&) = delete;

      int mergeEverything(SymbolTable& symTab, vector<RelocationTable>& relTabs, vector<string>& relSecs, vector<vector<uint8_t>>& secContent, vector<string>& secNames);

      void writeOutput(string outFilename);
      void printAllSectionContent();
      void printAllRelocationTables();

      int createExecutable(string outFilename, unordered_map<string, uint32_t>& sectionToAddress);

      void write32bitToSection(int32_t value, int sectionIndex, int offset);

  public:

      static Linker& getInstance(){
        static Linker instance;
        return instance;
      }

      int link(vector<string> inputFiles, bool isHex, bool isRelocatable, string outFilename, unordered_map<string, uint32_t>& sectionToAddress);

      ~Linker() {}

};


#endif