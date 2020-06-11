.data
filename: .asciiz "Collatz.asm"
filename2: .asciiz "BinarySearch.asm"
filename3: .asciiz "LinearSearch.asm"
prompt: .asciiz "\nExitting Kernel...\n"
.text
.globl main
main:
li $t0,-1
#fork
li $v0,18
syscall
beq $v0,$0,if_child
if_child_end:
li $v0,18
syscall
beq $v0,$0,if_child2
if_child_end2:
li $v0,18
syscall
beq $v0,$0,if_child3
if_child_end3:

l1:
li $v0,20
syscall
beq $t0,$v0,finished
j l1

finished:
li $v0,4
la $a0,prompt
syscall
#Exit
li $v0,22
syscall

if_child:
    la $v0,19
    la $a0,filename
    syscall
    j if_child_end

if_child2:
    la $v0,19
    la $a0,filename2
    syscall
    j if_child_end2

if_child3:
    la $v0,19
    la $a0,filename3
    syscall
    j if_child_end3