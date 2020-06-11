.data
filename: .asciiz "Collatz.asm"
filename2: .asciiz "BinarySearch.asm"
filename3: .asciiz "LinearSearch.asm"
prompt: .asciiz "\nExitting Kernel...\n"
.text
.globl main
main:
li $t0,-1
li $t1,1
li $t2,2

li $a0, 3  #Max bound.
li $v0, 21  #Random Number Syscall.
syscall
la $a0,($v0)

#Get in one of those
beq $a0,$0,load_f1
beq $a0,$t1,load_f2
beq $a0,$t2,load_f3

load_f1_end:
load_f2_end:
load_f3_end:


li $t3,10
l2:
beq $t3,$0,end_l2
#fork
li $v0,18
syscall
#if it is the child
beq $v0,$0,if_child
if_child_end:
addi $t3, $t3,-1			# $t3 = $t3 -1
j l2
end_l2:

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

load_f1:
    la $a0,filename
    j load_f1_end
load_f2:
    la $a0,filename2
    j load_f2_end
load_f3:
    la $a0,filename3
    j load_f3_end

if_child:
    #execv
    li $v0,19
    syscall
    j if_child_end