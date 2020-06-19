# The prompts allocated
.data
input_prompt: .asciiz "For input X = "
input_prompt2: .asciiz "Collatz Numbers are: "
.text
.globl main
# Main function to execute
main:
	# the limit
	li $t2,26
	# init t1 = 0

	# l2 loop below
	# while(t1<26){
	# 	collatz(t1)
	# 	t1++;
	# } 
	# exit 
	# t1 = 1
	li $t1,1
l2:	
	# call the collatz function 25 times
	j collatz
collatz_end:
	
	addi $t1,$t1,1 #increment t1

	# if t1 == 25 exit
	bne $t2,$t1,l2


	# process_exit()
	li $v0,22
	syscall
##############################################################
# input -> $t1 the number to find the sequence of
# collatz(t1)
##############################################################
collatz:
	# process_print(a0)
	li $v0,27
	la $a0,input_prompt # print the prompt for input
	syscall

	# process_int(a0)
	li $v0,26
	la $a0,0($t1)  # print the input
	syscall
	
	# process_print(a0)
	li $v0,27
	la $a0,input_prompt2 # print the prompt for output 
	syscall

	# load the init vale
	la $a0,0($t1)
	
	li $t0,1
	beq $a0,$t0,check_end

	l1:
	# Call process_print($a0)
	li $v0,26
	syscall
	
	la $t0,($a0)
	andi $t0,$t0,1 # and i to see if even
	beq $t0,$0,even # if(i is even) branch
	j odd # if odd branch there
	
	check_end: # branch join points
	
	bne $a0,1,l1 # if $a0 became one don't branch
	
	# Call process_print(1)
	li $a0,1
	li $v0,26
	syscall # printed 1

	j collatz_end
	
even:
	# right shift a if it is even
	sra $a0,$a0,1
	j check_end
odd:
	# if it is odd do calculations
	la $t0,($a0) # restoration
	add $a0,$a0,$t0 # collatz formula being applied
	add $a0,$a0,$t0
	addi $a0,$a0,1 # (3*a+1) here
	#sra $a0,$a0,1 # right shift again and branch to check_end
	j check_end

##################### end collatz ##############################