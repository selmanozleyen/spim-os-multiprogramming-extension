.data 
	arr1: .word 7 1 3 24 5 6
	arr1_len: .word 6
	target: .word 24
	prompt: .asciiz "{7,1,3,24,5,6} The target:  24 is at position : "
.text
.globl main
main:
	la $a0,arr1
	lw $a1,arr1_len
	lw $a2,target
	jal func
	j end
func:
	#storing previous values to stack
	addi $sp,$sp,-12
	sw $a0,0($sp)
	sw $t0,4($sp)
	sw $t1,8($sp)
	#set i to 0
	li $t0,0
	#set to print integer mode
	li $v0,1
l1:	
	#if found
	lw $t1,($a0)
	beq $a2,$t1,if_found
	addi $t0,$t0,1
	addi $a0,$a0,4
	#if index == len
	beq $a1,$t0,not_found
	j l1
if_found:
	
	la $a0,prompt
	li $v0,4
	syscall
	li $v0,1	
	la $a0,($t0)
	syscall
	li $a0,10
	li $v0,11
	syscall
	j end
not_found:
	j end 	
end:
	#process terminate
	li $v0,22
	syscall
