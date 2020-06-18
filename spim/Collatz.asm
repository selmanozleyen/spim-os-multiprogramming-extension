.data
input_prompt: .asciiz "Input 7:"
delimeter: .asciiz " "
new_line: .asciiz "\n"
.text
.globl main
main:	
	li $v0,4
	la $a0,input_prompt
	syscall
	
	li $a0,7
	l1:
	li $v0,1
	syscall
	la $t0,($a0)
	li $v0,4
	la $a0,delimeter
	syscall
	la $a0,($t0)
	
	la $t0,($a0)
	andi $t0,$t0,1
	beq $t0,$0,even
	j odd
	
	check_end:
	
	bne $a0,1,l1
	
	li $a0,1
	li $v0,1
	syscall

	li $v0,4
	la $a0,new_line
	syscall
	li $v0,22
	syscall
	
even:
	sra $a0,$a0,1
	j check_end
odd:
	la $t0,($a0)
	add $a0,$a0,$t0
	add $a0,$a0,$t0
	addi $a0,$a0,1
	sra $a0,$a0,1
	j check_end