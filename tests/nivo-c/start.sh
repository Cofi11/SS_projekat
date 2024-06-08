ASSEMBLER=./asembler
LINKER=./linker
EMULATOR=./emulator

${ASSEMBLER} -o main.o tests/nivo-c/main.s
${ASSEMBLER} -o handler.o tests/nivo-c/handler.s
${ASSEMBLER} -o isr_terminal.o tests/nivo-c/isr_terminal.s
${ASSEMBLER} -o isr_timer.o tests/nivo-c/isr_timer.s
${LINKER} -hex \
  -place=code@0x40000000 \
  -o program.hex \
  main.o isr_terminal.o isr_timer.o handler.o
${EMULATOR} program.hex