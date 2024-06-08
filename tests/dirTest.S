.equ d, 1
.equ b, 4 - 5 - d

.extern a, c
.global ALO


.section text_moj

.global _start
.skip 5

jmp a
labela: beq %r2, %r5, symbol
bne %r7, %r13, 020

ALO:.word 0x1234, a, ALO, 0123, 5000, proba
jmp 0x1234
jmp 10000

call 0x1234

ld [%r2 + 7], %r3

bne %r7, %r13, 10

.ascii "SRBIJA"
