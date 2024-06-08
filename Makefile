#Builds

 .PHONY: all
 all: clean asembler linker emulator

parser.tab.cpp parser.tab.hpp: misc/parser.ypp
		bison -d misc/parser.ypp

lex.yy.c: misc/lexer.l parser.tab.hpp
		flex misc/lexer.l

parser: lex.yy.c parser.tab.hpp parser.tab.cpp
		gcc -o parser lex.yy.c parser.tab.cpp

asembler: lex.yy.c parser.tab.hpp parser.tab.cpp ./src/asembler/*.cpp ./src/common/*.cpp
		g++ -g -o asembler lex.yy.c parser.tab.cpp ./src/asembler/*.cpp ./src/common/*.cpp

linker: ./src/linker/*.cpp ./src/common/*.cpp
		g++ -g -o linker ./src/linker/*.cpp ./src/common/*.cpp

emulator: ./src/emulator/*.cpp # ./src/common/*.cpp
		g++ -g -o emulator ./src/emulator/*.cpp

clean:
		rm -f lex.yy.c
		rm -f parser.tab.hpp
		rm -f parser.tab.cpp
		rm -f parser
		rm -f asembler
		rm -f linker
		rm -f emulator

clean_obj:
		rm -f *.o
		rm -f *.objdump
		rm -f *.hex