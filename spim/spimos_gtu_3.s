.data
filename1: .asciiz "Collatz.asm"
filename2: .asciiz "BinarySearch.asm"
filename3: .asciiz "LinearSearch.asm"
prompt: .asciiz "\nExitting Kernel...\n"
.text
.globl main
main:
    li $a0, 3  #Max bound.
    li $v0, 21  #Random Number Syscall.
    syscall
    #t0 and t1 will be filled accordingly
    beq $v0,0,load_1_2
    beq $v0,1,load_0_2
    beq $v0,2,load_0_1

load_end:

    li $t6,3
l2:
    beq $t6,$0,end_l2


    li $v0,18
    syscall
    #$t5 = child
    la $t5,($v0)

    #if a == 0 and child
    #then execv
    seq $t3,$t0,0
    seq $t4,$t5,0
    and $t3,$t3,$t4
    jal load_f1

    #if a == 1 and child -> execv
    seq $t3,$t0,1
    seq $t4,$t5,0
    and $t3,$t3,$t4
    jal load_f2

    #if a == 2 and child -> execv

    seq $t3,$t0,2
    seq $t4,$t5,0
    and $t3,$t3,$t4
    jal load_f3

    #if b == 0 and child
    #then execv
    li $v0,18
    syscall
    #$t5 = child
    la $t5,($v0)

    seq $t3,$t1,0
    seq $t4,$t5,0
    and $t3,$t3,$t4
    jal load_f1

    #if b == 1 and child -> execv

    seq $t3,$t1,1
    seq $t4,$t5,0
    and $t3,$t3,$t4
    jal load_f2

    #if b == 2 and child -> execv

    seq $t3,$t1,2
    seq $t4,$t5,0
    and $t3,$t3,$t4
    jal load_f3

    addi $t6, $t6,-1			# $t6 = $t6 -1

    j l2
end_l2:

l1:
    #wait
    li $v0,20
    syscall
    beq $v0,-1,finished
    j l1

finished:
    li $v0,4
    la $a0,prompt
    syscall
    #Exit
    li $v0,22
    syscall

load_f1:
    beq $t3,1,if_ok_1
    if_ok_1_end:
    jr $ra
load_f2:
    beq $t3,1,if_ok_2
    if_ok_2_end:
    jr $ra
load_f3:
    
    beq $t3,1,if_ok_3
    if_ok_3_end:
    jr $ra


if_ok_1:
    la $a0,filename1
    li $v0,19
    syscall
    j if_ok_1_end
if_ok_2:
    la $a0,filename2
    li $v0,19
    syscall
    j if_ok_2_end
if_ok_3:
    la $a0,filename3
    li $v0,19
    syscall
    j if_ok_3_end

load_0_1:
    li $t0,0
    li $t1,1
    j load_end

load_1_2:
    li $t0,1
    li $t1,2
    j load_end

load_0_2:
    li $t0,0
    li $t1,2
    j load_end

