.section kod
halt

int
  iret # alo

ret


push %r3
pop %r11

PROBA:xchg %r4, %r12
# AAAA
call 0x1234

add %r1, %r2
sub %r1, %r7
mul %r1, %r2  # alo BBBB

    div %r1, %r2
  not %r5

and %r6,    %r8
or %r6, %r14

.section nova_sekcija
LABELA:xor %r6, %r15

.global PROBA


shl %r2, %r1
shr %r3, %r13

call 0xFFFFFF

csrrd %handler, %r10
csrwr %r7, %status


.section jumps_sekcija

ISTA: bne %r1, %r2, 0xAABBCC

bgt %r3, %r4, 0x123

beq %r5, %r6, LABELA

call 1000

bgt %r10, %r11, ISTA

call KASNIJE

jmp cc

.section poslednja
 KASNIJE: .word 0x1234, simbol, dd # , nedefinisan
 .equ dd, cc - ee
 .equ ee, bb + 5
 .equ cc, bb
  .equ bb, 4


# .section proba_pool

# call 0x123456

# .skip 0x7f0
# .word 10,20,30
# .ascii "proba proba1"
# .skip 12

# push %r2
# push %r2
# call 0xaabb
# pop %r2
# .word 0x12345678

.section loadstore

.equ simbol2, simbol +10 + dd
.equ simbol, 0x5678
ld $0x12, %r1
A:ld $0x1234, %r2
ld 0x12, %r1
ld 0x1234, %r2
ld %r1, %r2
ld [%r1], %r2
ld $A, %r2
ld A, %r2
ld KASNIJE, %r2
ld [%r1 + 0x12], %r2

st %r1, [%r2]
st %r1, 0x12
st %r1, 0xaabbcc
st %r1, KASNIJE
st %r1, B
B: st %r1, [%r2 + 0x12]
# st %r1, [%r2 + B]

.section loadstore2

ld simbol2, %r1
ld [%r3 + bb], %r4
ld [%r3 + ee], %r4
st %r7, [%r9 + bb]
st %r7, [%r9 + dd]

# .global KASNIJE


.end
