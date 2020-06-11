.data
	arr: .word 1 2 3 4 5 6 7 8 9 10 11
	arr_len: .word 11
	target: .word 6
	input_prompt: .asciiz "Input {1,2,3,4,5,6,7,8,9,10,11} x = 6\n Output = "
	NOT_FOUND_MSG: .asciiz "Not Found"
.text
.globl main
main:
	la $a0,arr
	lw $a1,arr_len
	lw $a2,target
	j func

func:
	#save a0,t0,t1,t2,t3
	addi $sp,$sp,-20
	sw $a0,0($sp)
	sw $t0,4($sp)
	sw $t1,8($sp)
	sw $t2,12($sp)
	sw $t3,16($sp)
	
	
	# l
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
	#print input prompt
	la $a0,input_prompt
	li $v0,4
	syscall
	#print the index and exit
	la $a0,($t2)
	li $v0,1
	syscall
	#print newline
	li $a0,10
	li $v0,11
	syscall
	j end		
on_left:
	la $t1,-1($t2)
	j start_w1
on_right:
	la $t0,1($t2)
	j start_w1

end:
	#reload a0,t0,t1,t2,t3

	lw $a0,0($sp)
	lw $t0,4($sp)
	lw $t1,8($sp)
	lw $t2,12($sp)
	lw $t3,16($sp)
	addi $sp,$sp,20
	
	li $v0,22
	syscall

