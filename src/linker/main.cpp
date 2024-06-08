#include <iostream>
#include <vector>
#include <unordered_map>
#include "../../inc/linker/linker.hpp"
#include <regex>
using namespace std;

int main(int argc, char* argv[]){

  Linker& linker = Linker::getInstance();

  vector<string> inputFiles;
  string outFilename = "output.o";
  bool hex = false;
  bool relocatable = false;
  unordered_map<string, uint32_t> sectionToAddress;

  for(int i = 1 ; i < argc ; i++){
    string arg = argv[i];

    if(arg == "-hex"){
      hex = true;
    }
    else if(arg == "-o"){
      if(i + 1 < argc){
        outFilename = argv[++i];
      }
      else{
        cout << "ERROR: Output filename not provided!" << endl;
        return 1;
      }
    }
    else if(arg == "-relocatable"){
      relocatable = true;
    }
    else if (regex_match(arg, regex("^-place=([a-zA-Z._][a-zA-Z._0-9]*)@(0x[0-9a-fA-F]+)$"))) {
        smatch match;
        regex_search(arg, match, regex("^-place=([a-zA-Z._][a-zA-Z._0-9]*)@(0x[0-9a-fA-F]+)$"));
        string section = match[1];
        string address = match[2];
        sectionToAddress[section] = stoul(address, nullptr, 16);
    }
    else{
      inputFiles.push_back(arg);
    }

  }

  if(!relocatable && !hex){
    cout << "ERROR: You must specify exactly one of -hex or -relocatable flags!" << endl;
    return 1;
  }

  if(relocatable && hex){
    cout << "ERROR: You must specify exactly one of -hex or -relocatable flags!" << endl;
    return 1;
  }

  int ret = linker.link(inputFiles, hex, relocatable, outFilename, sectionToAddress);

  if(ret){
    cout << "Error in linking: " << ret << "!!!" << endl;
  }

}