#include <iostream>
#include <fstream>
#include "../../inc/emulator/emulator.hpp"
using namespace std;

int main(int argc, char* argv[]){

  if(argc != 2){
    cout << "Invalid number of arguments!" << endl;
    return 1;
  }

  string arg = argv[1];
  
  ifstream inputFile(arg);
  if (!inputFile) {
    cout << "Failed to open file: " << arg << endl;
    return 1;
  }

  Emulator& emulator = Emulator::getInstance();

  int ret = emulator.emulate(inputFile);

  if(ret){
    cout << "Error while emulating: " << ret << endl;
    return 1;
  }
  

  return 0;

}