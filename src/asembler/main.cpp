#include <iostream>
#include <vector>
#include "../../inc/structures.hpp"
#include "../../parser.tab.hpp"
#include "../../inc/asembler/singlePassAssembler.hpp"
using namespace std;

extern FILE* yyin;
extern void yylex_destroy();
extern int lineNumber;

void yyerror(vector<Line> &code, char *sp){
  cout << "Error at line " << lineNumber << ": " << sp << endl;
}

void printCodeTesting(vector<Line>& code);

void freeCode(vector<Line> &code);

int main(int argc, char* argv[]){

  if(argc != 2 && argc != 4){
    cout << "Invalid number of arguments!!!" << endl;
    return 1;
  }

  string outFilename = "out.o";
  string inFilename = argv[argc - 1];

  if(argc == 4){
    if(string(argv[1]) == "-o"){
      outFilename = argv[2];
    }
    else{
      cout << "Invalid argument options!!!" << endl;
      return 1;
    }
  }

  FILE* f = fopen(inFilename.c_str(), "r");

  if(!f){
    cout << "NE POSTOJI FAJL!" << endl;
    return 2;
  }

  yyin = f;

  vector<Line> code;

  int a = yyparse(code);

  yylex_destroy();

  fclose(f);

  if(!a){
    
    // printCodeTesting(code);

    SinglePassAssembler& spa = SinglePassAssembler::getInstance();

    int ret = spa.assemble(code, (char*)outFilename.c_str());

    if(ret){
      cout << "Error in assembling: " << ret << "!!!" << endl;
    }
    
    freeCode(code);

  }
  else{
    cout << "NES NE VALJA: " << a << endl;
  }
}




void printCodeTesting(vector<Line>& code){
    Instruction ins;
    Directive dir;
  
    cout << endl << "***** PRINT CODE FOR TESTING PURPOSES:" << endl;

    for(auto &line : code){
      
      // cout << line.type << " ";

      if(line.label != nullptr)
        cout << line.label << ": ";

      switch(line.type){
        case LINE_INS:

          ins = line.content.ins;

          cout << ins.type << " " << ins.regSrc << " " << ins.regDst << " ";
          cout << ins.op.type << " ";
          switch(ins.op.type){
            case OP_REG:
              cout << ins.op.value.reg << endl;
              break;
            case OP_REG_IND:
              cout << "[" << ins.op.value.reg << "]" << endl;
              break;
            case OP_IMM_SYM:
              cout << ins.op.value.imm.value.symbol << endl;
              break;
            case OP_IMM_LIT:
              cout << ins.op.value.imm.value.literal << endl;
              break;
            case OP_MEM_SYM:
              cout << ins.op.value.mem.value.symbol << endl;
              break;
            case OP_MEM_LIT:
              cout << ins.op.value.mem.value.literal << endl;
              break;
            case OP_MEM_OFF_LIT:
              cout << ins.op.value.memOff.reg << " + " << ins.op.value.memOff.offset.value.literal << endl;
              break;
            case OP_MEM_OFF_SYM:
              cout << ins.op.value.memOff.reg << " + " << ins.op.value.memOff.offset.value.symbol << endl;
              break;
            case OP_PC_LIT:
              cout << " PC + " << ins.op.value.imm.value.literal << endl;
              break;
            case OP_PC_SYM:
              cout << " PC + " << ins.op.value.imm.value.symbol << endl;
              break;
            default:
              cout << endl;
              break;
          }
          break;
        case LINE_DIR:
          
          dir = line.content.dir;

          cout << dir.type << " ";
          if(dir.symbols_literals != nullptr){

            vector<SymbolLiteral> symbols_literals = *dir.symbols_literals;
            
            for(int i = 0; i < symbols_literals.size(); i++){
              bool isSymbol = symbols_literals[i].isSymbol;
              if(isSymbol)
                cout << symbols_literals[i].value.symbol << " ";
              else
                cout << symbols_literals[i].value.literal << " ";
            }
          }
          if(dir.type == ASCII){
            cout << dir.ascii_string;
          }
          if(dir.expression != nullptr){
            vector<ExpressionOperand> expression = *dir.expression;
            for(int i = 0; i < expression.size(); i++){
              char sign = (expression[i].sign == 1) ? '+' : '-';
              cout << sign;
              if(expression[i].symbol.isSymbol)
                cout << expression[i].symbol.value.symbol << " ";
              else
                cout << expression[i].symbol.value.literal << " ";
            }
          
          }
          cout << endl;
          break;
        default:
          cout << endl;
          break;
      }

    }

    cout<<"***** END" << endl << endl;
}


void freeCode(vector<Line> &code){
  for(auto &line : code){
    // if(line.label != nullptr)
      // free(line.label);
    if(line.type == LINE_DIR){
      if(line.content.dir.symbols_literals != nullptr)
        delete line.content.dir.symbols_literals;
      if(line.content.dir.expression != nullptr)
        delete line.content.dir.expression;
      // if(line.content.dir.ascii_string != nullptr)
        // free(line.content.dir.ascii_string);
    }
  }
}




