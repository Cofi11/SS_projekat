%option noyywrap

%{

#include "inc/structures.hpp"
#include <vector>  
#include "parser.tab.hpp"
int lineNumber = 1;


%}

%%

#.*                 {return COMMENT;}
[a-zA-Z_][a-zA-Z_0-9]*[ \t]*: {
                    yylval.strval = strdup(yytext);

                    int i = 0;
                    while(yylval.strval[i] != '\0' && yylval.strval[i] != ':' && yylval.strval[i] != ' ' && yylval.strval[i] != '\t')
                        i++;
                    yylval.strval[i] = '\0';

                    return LABEL;
}

","                 {return COMMA;}
"+"                 {return PLUS;}
"-"                 {return MINUS;}
"$"                 {return DOLLAR;}
"["                 {return OPEN_BRACKET;}
"]"                 {return CLOSED_BRACKET;}

".global"           {return DIR_GLOBAL;}
".extern"           {return DIR_EXTERN;}
".section"          {return DIR_SECTION;}
".word"             {return DIR_WORD;}
".skip"             {return DIR_SKIP;}
".ascii"            {return DIR_ASCII;}
".equ"              {return DIR_EQU;}
".end"              {return DIR_END;}

"halt"              {return INS_HALT;}
"int"               {return INS_INT;}
"iret"              {return INS_IRET;}
"call"              {return INS_CALL;}
"ret"               {return INS_RET;}
"jmp"               {return INS_JMP;}
"beq"               {return INS_BEQ;}
"bne"               {return INS_BNE;}
"bgt"               {return INS_BGT;}
"push"              {return INS_PUSH;}
"pop"               {return INS_POP;}
"xchg"              {return INS_XCHG;}
"add"               {return INS_ADD;}
"sub"               {return INS_SUB;}
"mul"               {return INS_MUL;}
"div"               {return INS_DIV;}
"not"               {return INS_NOT;}
"and"               {return INS_AND;}
"or"                {return INS_OR;}
"xor"               {return INS_XOR;}
"shl"               {return INS_SHL;}
"shr"               {return INS_SHR;}
"ld"                {return INS_LD;}
"st"                {return INS_ST;}
"csrrd"             {return INS_CSRRD;}
"csrwr"             {return INS_CSRWR;}

%r[0-9]             {yylval.numval = atoi(yytext + 2); return REG;}
%r1[0-5]            {yylval.numval = atoi(yytext + 2); return REG;}
sp                  {yylval.numval = 14; return REG;}
pc                  {yylval.numval = 15; return REG;}
%status             {yylval.numval = 16; return CREG;}
%handler            {yylval.numval = 17; return CREG;}
%cause              {yylval.numval = 18; return CREG;}

[0-9]               {yylval.numval = atoi(yytext); return NUM;}
[1-9][0-9]+         {yylval.numval = atoi(yytext); return NUM;}
0[0-7]+             {yylval.numval = strtol(yytext, NULL, 8); return NUM_OCT;}
0x[0-9a-fA-F]+      {yylval.numval = strtol(yytext, NULL, 16); return NUM_HEX;}

\"[^"]+\"           {yylval.strval = strdup(yytext + 1);
                     yylval.strval[strlen(yylval.strval) - 1] = '\0';
                     return STRING;
                    }
[a-zA-Z._][a-zA-Z._0-9]* {
                    yylval.strval = strdup(yytext);
                    return SYMBOL;
}
\n                  {lineNumber++; return ENDL;}
.                   ;


%%