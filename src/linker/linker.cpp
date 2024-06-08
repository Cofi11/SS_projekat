#include "../../inc/linker/linker.hpp"
#include <algorithm>

int Linker::link(vector<string> inputFiles, bool isHex, bool isRelocatable, string outFilename, unordered_map<string, uint32_t>& sectionToAddress){
  
  for(int i = 0 ; i < inputFiles.size(); i++){
    ifstream input(inputFiles[i], ios::binary);

    if(!input.is_open()){
      cout << "ERROR: Failed to open file " << inputFiles[i] << " for reading!" << endl;
      return 10; //neuspesno otvaranje fajla
    }
    
    uint32_t size;
    SymbolTable symbolTableCurrent;
    vector<RelocationTable> relocationTablesCurrent;
    vector<string> relocationSectionsCurrent; //sekcije za koje su nadjene relocation tabele
    vector<vector<uint8_t>> sectionsContentCurrent;
    vector<string> sectionsNamesCurrent;
    
    read(input, size);
    
    for(int i = 0 ; i < size; i++){
      string name;
      readString(input, name);
      if(name == "__SYMTAB__"){
        symbolTableCurrent.readTableFromInput(input);
      }
      else if(name.find(".rela.") != string::npos){
        RelocationTable table;
        table.readTableFromInput(input);
        relocationTablesCurrent.push_back(table);
        std::string prefix = ".rela.";
        name = name.substr(prefix.size());
        relocationSectionsCurrent.push_back(name);
      }
      else{
        sectionsNamesCurrent.push_back(name);
        vector<uint8_t> content;
        read(input, content);
        sectionsContentCurrent.push_back(content);
      }
    }

    input.close();

    int ret = mergeEverything(symbolTableCurrent, relocationTablesCurrent, relocationSectionsCurrent, sectionsContentCurrent, sectionsNamesCurrent);
    
    if(ret){
      return ret;
    }
    
  }

  if(isRelocatable){
    writeOutput(outFilename);
  }
  else{//znamo da je onda hex
    int ret = createExecutable(outFilename, sectionToAddress);
    if(ret){
      return ret;
    }
  }
  
  return 0;
  
}


int Linker::mergeEverything(SymbolTable& symTab, vector<RelocationTable>& relTabs, vector<string>& relSecs, vector<vector<uint8_t>>& secContent, vector<string>& secNames){

  unordered_map<string, int32_t> sectionsSizes; //prethodne velicine sekcija, sluzi za dodavanje offste za istoimene nove sekcije
  unordered_map<string, int> sectionToIndex;

  for(int i = 0 ; i < sectionsNames.size(); i++){
    sectionsSizes[sectionsNames[i]] = sectionsContent[i].size();
    sectionToIndex[sectionsNames[i]] = i;
  }

  unordered_map<string, int> relocationSectionsToIndex;

  for(int i = 0 ; i < relocationSections.size(); i++){
    relocationSectionsToIndex[relocationSections[i]] = i;
  }

  int ret = symbolTable.mergeTwoTables(symTab, sectionsSizes);

  if(ret){
    return ret;
  }

  for(int i = 0 ; i < secNames.size(); i++){
    if(sectionToIndex.find(secNames[i]) != sectionToIndex.end()){
      sectionsContent[sectionToIndex[secNames[i]]].insert(sectionsContent[sectionToIndex[secNames[i]]].end(), secContent[i].begin(), secContent[i].end());
    }
    else{
      sectionsNames.push_back(secNames[i]);
      sectionsContent.push_back(secContent[i]);
    }
  }


  for(int i = 0 ; i < relTabs.size() ; i++){
    RelocationTable& table = relTabs[i];
    string relSect = relSecs[i];

    vector<RelocationTableEntry>& entries = table.getTable();
    for(RelocationTableEntry& entry: entries){
      entry.symbol = symbolTable.getSymbolIndex(symTab.getSymbolName(entry.symbol));

      if(symbolTable.checkSection(symbolTable.getSymbolName(entry.symbol))){
        entry.addend += sectionsSizes[symbolTable.getSymbolName(entry.symbol)];
      }

    }

    if(relocationSectionsToIndex.find(relSect) != relocationSectionsToIndex.end()){
      relocationTables[relocationSectionsToIndex[relSect]].mergeTwoTables(table, sectionsSizes[relSect]);
    }
    else{
      if(sectionToIndex.find(relSect) != sectionToIndex.end()){
        table.updateOffsets(sectionsSizes[relSect]);
      }
      
      relocationSections.push_back(relSect);
      relocationTables.push_back(table);
      relocationSectionsToIndex[relSect] = relocationTables.size() - 1;
    }

  }

  return 0;


}




void Linker::writeOutput(string outFilename){
    string objdumpName = string(outFilename).replace(string(outFilename).find(".o"), 2, ".objdump");
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

    
    ofstream output(outFilename, ios::binary);

    uint32_t size = 1; //koliko razlicith stvari treba procitati, 1 je SYMTAB, pa + broj relok tabela + broj sekcija
    size += relocationTables.size();
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


void Linker::printAllSectionContent(){
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
}


void Linker::printAllRelocationTables(){
  cout << "***** RELOCATION TABLES *****" << endl;
  for(int i = 0; i < relocationTables.size(); i++){
    // if(relocationTables[i].checkEmpty())continue;
    cout << "rela." << relocationSections[i] << endl;
    relocationTables[i].printTable();
  }
  cout << endl;
}








int Linker::createExecutable(string outFilename, unordered_map<string, uint32_t>& sectionToAddress){

  unordered_map<string, int32_t> sectionToIndex;
  for(int i = 0 ; i < sectionsNames.size(); i++){
    sectionToIndex[sectionsNames[i]] = i;
  }
  
  vector<pair<string,uint32_t>> sectionAdresses;

  for(auto& it : sectionToAddress){
    sectionAdresses.push_back(it);
  }

  sort(sectionAdresses.begin(), sectionAdresses.end(), [](pair<string,uint32_t>& a, pair<string,uint32_t>& b){
    return a.second < b.second;
  });

  uint32_t currentAddress = 0;
  for(auto& it : sectionAdresses){
    if(it.second < currentAddress){
      cout << "ERROR: Sections overlap!" << endl;
      return 3; //preklapanje sekcija
    }
    if(!symbolTable.checkSection(it.first)){
      cout << "ERROR: Place is not a section or it does not exist!" << endl;
      return 4; //place nije sekcija ili ne postoji
    }
    symbolTable.setSymbolValue(it.first, it.second);
    currentAddress = it.second + sectionsContent[sectionToIndex[it.first]].size();
  }

  //preostale sekcije mapirati na adrese
  for(int i = 0 ; i < sectionsNames.size(); i++){
    if(sectionToAddress.find(sectionsNames[i]) == sectionToAddress.end()){//sekcija koja nije obelezena
      
      symbolTable.setSymbolValue(sectionsNames[i], currentAddress);
      sectionAdresses.push_back({sectionsNames[i], currentAddress});
      currentAddress += sectionsContent[i].size();

    }
  }
  //sada treba svim globalnim simbolima preostalim dodeliti vrednosti
  //za lokalne nije bitno jer ce svakako relokacije biti u odnosu na sekciju
  //a sekcijama su vec dodeljene nove vrednosti
  int ret = symbolTable.updateGlobalSymbols();

  if(ret){
    return ret;
  }

  //prolazak kroz sve relokacije i ispravke u sekcijama
  for(int i = 0 ; i < relocationTables.size(); i++){
    RelocationTable& table = relocationTables[i];
    string relSect = relocationSections[i];
    vector<RelocationTableEntry> entries = table.getTable();

    for(RelocationTableEntry& entry: entries){
      int32_t value = symbolTable.getSymbolValue(symbolTable.getSymbolName(entry.symbol)) + entry.addend;

      write32bitToSection(value, sectionToIndex[relSect], entry.offset);

    }
  }

  // upis u hex file
  ofstream outputFile(outFilename);

  for(auto& it : sectionAdresses){

    vector<uint8_t> content = sectionsContent[sectionToIndex[it.first]];
    uint32_t adr = it.second;

    for (int i = 0; i < content.size(); i++) {
        if (i % 8 == 0) {
            outputFile << std::hex << std::setw(8) << std::setfill('0') << (adr + i) << ": ";
        }

        outputFile << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(content[i]) << " ";

        if (i % 8 == 7 || i == content.size() - 1) {
            outputFile << "\n";
        }
    }

  }

  outputFile.close();


  return 0;


}



void Linker::write32bitToSection(int32_t value, int sectionIndex, int offset){
  sectionsContent[sectionIndex][offset] = value & 0xFF;
  sectionsContent[sectionIndex][offset + 1] = (value >> 8) & 0xFF;
  sectionsContent[sectionIndex][offset + 2] = (value >> 16) & 0xFF;
  sectionsContent[sectionIndex][offset + 3] = (value >> 24) & 0xFF;
}