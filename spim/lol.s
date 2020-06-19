.data 
	arr1: .word 7 1 3 24 5 6
	arr1_len: .word 6
	target: .word 24
	prompt: .asciiz "{7,1,3,24,5,6} The target:  24 is at position : "
.text
.globl main
main:
	# setting the values
	# a0 -> arr1
	# a1 -> arr1_len
	# a2 -> target
	la $a0,arr1
	lw $a1,arr1_len
	lw $a2,target
	# set the values for calling the function now call it
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
	beq $a2,$t1,if_found # branch if found
	addi $t0,$t0,1 # increment index
	addi $a0,$a0,4 # arr_ptr++
	#if index == len it is over if not found branch
	beq $a1,$t0,not_found
	j l1
if_found:
	# print the prompt
	la $a0,prompt
	li $v0,27
	# process_print(prompt)
	syscall
	# print the result
	li $v0,26	
	la $a0,($t0)
	# process_print_int(result)
	syscall
	# branch to end
	j end
not_found:
	j end # if not found branch to end and exit	
end:
	# process_terminate()
	li $v0,22
	syscall
