.data
	arr: .word 1 2 3 4 5 6 7 8 9 10 11
	arr_len: .word 11
	target: .word 6
	input_prompt: .asciiz "Input {1,2,3,4,5,6,7,8,9,10,11} x = 6  Output = "
	NOT_FOUND_MSG: .asciiz "Not Found"
.text
.globl main
main:
	# loading data parts to registers
	# a0 = arr
	la $a0,arr
	# a1 = arr_len 
	lw $a1,arr_len
	# a2 = target
	lw $a2,target
	# bs(arr,arr_len,target)
	j func

func:
	#save registers to stack
	addi $sp,$sp,-20
	sw $a0,0($sp)   #sp[0] = a0
	sw $t0,4($sp)   #sp[1] = t0
	sw $t1,8($sp)   #sp[2] = t1
	sw $t2,12($sp)  #sp[3] = t2
	sw $t3,16($sp)  #sp[4] = t3
	
	
	# t0 = 0
	li $t0,0
	# r
	addi $t1,$a1,-1
	
	# while (l <= r)
start_w1:	
	ble $t0,$t1,while1
	
	
	#if didn't found anything print and exit
	la $a0,NOT_FOUND_MSG
	li $v0,4
	syscall
	j end
	
	#main while
while1:
	# m being calculated
	add $t2,$t0,$t1
	sra $t2,$t2,1
	# t2 <- m
	
	sll $t3,$t2,2
	# arr + 4m
	add $t3,$a0,$t3
	#arr[m]
	lw $t3,($t3)
	
	# if arr[m] == t
	beq $t3,$a2,found
	bgt $t3,$a2,on_left
	blt $t3,$a2,on_right

	j start_w1			
found:
	# print input prompt but with a special syscall
	# set the parameters
	la $a0,input_prompt
	li $v0,27
	# print_terminal_string(input_prompt)
	syscall
	# print_terminal_int(input_prompt)
	la $a0,($t2)
	li $v0,26
	syscall
	
	j end		
on_left:
	la $t1,-1($t2)
	j start_w1
on_right:
	la $t0,1($t2)
	j start_w1

end:

	lw $a0,0($sp)
	lw $t0,4($sp)
	lw $t1,8($sp)
	lw $t2,12($sp)
	lw $t3,16($sp)
	addi $sp,$sp,20
	
	li $v0,22
	syscall

