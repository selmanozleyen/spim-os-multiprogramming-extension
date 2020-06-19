.data
# Words defined here
w0: .asciiz "wheel"
w1: .asciiz "coach"
w2: .asciiz "chop"
w3: .asciiz "meadow"
w4: .asciiz "agony"
w5: .asciiz "burn"
w6: .asciiz "protection"
w7: .asciiz "distort"
w8: .asciiz "margin"
w9: .asciiz "haunt"
w10: .asciiz "pleasant"
w11: .asciiz "shame"
w12: .asciiz "consciousness"
w13: .asciiz "wage"
w14: .asciiz "affinity"
w15: .asciiz "bulletin"
w16: .asciiz "depart"
w17: .asciiz "glare"
w18: .asciiz "housing"
w19: .asciiz "breeze"
w20: .asciiz "constitution"
w21: .asciiz "as"
w22: .asciiz "productive"
w23: .asciiz "ignore"
w24: .asciiz "screw"
w25: .asciiz "cane"
w26: .asciiz "disco"
w27: .asciiz "looting"
w28: .asciiz "snatch"
w29: .asciiz "cheap"
w30: .asciiz "continuation"
w31: .asciiz "agriculture"
w32: .asciiz "calorie"
w33: .asciiz "snow"
w34: .asciiz "wilderness"
w35: .asciiz "map"
w36: .asciiz "differ"
w37: .asciiz "supplementary"
w38: .asciiz "gossip"
w39: .asciiz "radio"
w40: .asciiz "influence"
w41: .asciiz "personal"
w42: .asciiz "index"
w43: .asciiz "restrict"
w44: .asciiz "jail"
w45: .asciiz "disability"
w46: .asciiz "display"
w47: .asciiz "aviation"
w48: .asciiz "cook"
w49: .asciiz "dash"
w50: .asciiz "reflection"
w51: .asciiz "writer"
w52: .asciiz "publish"
w53: .asciiz "responsible"
w54: .asciiz "variable"
w55: .asciiz "underline"
w56: .asciiz "secretary"
w57: .asciiz "bleed"
w58: .asciiz "charm"
w59: .asciiz "constant"
w60: .asciiz "arena"
w61: .asciiz "ward"
w62: .asciiz "animal"
w63: .asciiz "oral"
w64: .asciiz "incident"
w65: .asciiz "smell"
w66: .asciiz "month"
w67: .asciiz "year"
w68: .asciiz "reign"
w69: .asciiz "taxi"
w70: .asciiz "store"
w71: .asciiz "fly"
w72: .asciiz "stall"
w73: .asciiz "impact"
w74: .asciiz "bowel"
w75: .asciiz "cultivate"
w76: .asciiz "network"
w77: .asciiz "week"
w78: .asciiz "blonde"
w79: .asciiz "prefer"
w80: .asciiz "identity"
w81: .asciiz "adventure"
w82: .asciiz "robot"
w83: .asciiz "social"
w84: .asciiz "bag"
w85: .asciiz "wave"
w86: .asciiz "exact"
w87: .asciiz "daughter"
w88: .asciiz "lazy"
w89: .asciiz "force"
w90: .asciiz "Anna"
w91: .asciiz "Civic"
w92: .asciiz "Kayak"
w93: .asciiz "Level"
w94: .asciiz "Madam"
w95: .asciiz "Mom"
w96: .asciiz "Noon"
w97: .asciiz "Racecar"
w98: .asciiz "Radar"
w99: .asciiz "redder"
# Dictionary defined here
dict: .word  w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12,w13,w14,w15,w16,w17,w18,w19,w20,w21,w22,w23,w24,w25,w26,
.word w27,w28,w29,w30,w31,w32,w33,w34,w35,w36,w37,w38,w39,w40,w41,w42,w43,w44,w45,w46,w47,w48,w49,w50,w51,w52,
.word w53,w54,w55,w56,w57,w58,w59,w60,w61,w62,w63,w64,w65,w66,w67,w68,w69,w70,w71,w72,w73,w74,w75,w76,w77,w78,
.word w79,w80,w81,w82,w83,w84,w85,w86,w87,w88,w89,w90,w91,w92,w93,w94,w95,w96,w97,w98,w99
# Informing for input error
error_mes: .asciiz "Error Occured."
# Prompting for input
# Giving the result
is_palindrom_mes: .asciiz "Palindrome"
is_not_palindrom_mes: .asciiz "Is Not Palindrome"
newline: .asciiz "\n"
#text segment
.text
.globl main
main:
    # load the dict
	la $s0,dict
	li $s2,100 #numbers of words loaded
    li $s3,1 # i = 0 
l1:
	lw $s1,0($s0) #load the pointer of the word
	# load word pointer as arg
	la $a0,0($s1)	
    # call the next function is_palindrome
	jal is_palindrome
	
	addi $s0,$s0,4 # go to the next dict word
	addi $s2,$s2,-1 # decrement s2
	addi $s3,$s3,1 # increment s3 (global i)
	beq $s2,$0,exit #exit if s2 = 0
    # if not exited continue
	j l1
exit:
    # program is over
	li $v0,22 # process_exit();
	syscall

############################################################################
# int strlen_n_lower(str)
# returns the size of the elements while making it lowercase
# a0 strarr -> given null terminated char pointer
# v0 length of the char array
strlen_n_lower:
    addi $sp,$sp,-32 # allocate for saving the registers
    sw $ra,0($sp) #sp[0] = $ra
    sw $t0,4($sp) #sp[1] = $t0
    sw $t1,8($sp) #sp[2] = $t1
    sw $t2,12($sp) #sp[3] = $t2
    sw $t3,16($sp) #sp[4] = $t3
    sw $t4,20($sp) #sp[5] = $t4
    sw $t5,24($sp) #sp[6] = $t5
    sw $t6,28($sp) #sp[7] = $t6

	li $v0,0 # i = 0 (v0 = i)
strlen_loop:
	lb $t1,($a0) # t1 = arr[0]

    # before checking if null lowercase it if it is uppercase
    jal to_lower
	# if t1 == 0 then length is $v0
	beq $t1,$0,strlen_end #if null occured go to end branch
	addi $v0,$v0,1 #increment the result
	addi $a0,$a0,1 # go to the next byte
	j strlen_loop
	
strlen_end:

    lw $ra,0($sp) # $ra = sp[0]
    lw $t0,4($sp) # $t0 = sp[1]
    lw $t1,8($sp) # $t1 = sp[2]
    lw $t2,12($sp) # $t2 = sp[3]
    lw $t3,16($sp) # $t3 = sp[4]
    lw $t4,20($sp) # $t4 = sp[5]
    lw $t5,24($sp) # $t5 = sp[6]
    lw $t6,28($sp) # $t6 = sp[7]
    addi $sp,$sp,32 # allocate for saving the registers
	# return to the caller
	jr $ra

################ end strlen ################################################
	
############################################################################
# void to_lower(str)
# a0 strarr -> null terminated array to lower
# t1 the byte being pointed
to_lower:

    li $t2,64 # the lower limit ( uppercase A )
    li $t3,91 # the upper limit ( uppercase Z )
    li $t4,32 # number to add to get lowercase
    
    slt $t5,$t2,$t1 # lower_limit < char
    slt $t6,$t1,$t3 # char upper_limit
    and $t5,$t5,$t6 # lower_limit < char && char < upper_limit == $t5
    beq $t5,$0,not_lower # branch if not lower

    #is lower decrement thr value of t1
    add $t1,$t4,$t1
    # store the changed char
    sb $t1,($a0)

not_lower:
    # return to caller
    jr $ra

################ end to_lower ##############################################

############################################################################
# is_palindrome(str)
# a0 the array to check if it is palindrome or not
is_palindrome:
    addi $sp,$sp,-4 # allocate 1 word
    sw $ra,0($sp) # store 1 word

    la $t0,($a0) # save the argument

    jal strlen_n_lower # strlen and lower and get the length

    beq $v0,$0,is_invalid # branch for error

    la $a0,($t0) # restore the argument
    la $t1,($v0) # store the length of the array = i
    la $t2,($a0) # store the head of array
    add $t3,$t2,$t1 # t3->pointer to end of the array + 1 (NULL terminator)
    addi $t3,$t3,-1 # t3->pointer to end of the array
    
    sra $t1,$t1,1 # divide -> t1/2

palindrome_loop:
    lb $t4,0($t3) # load from the end iterator
    lb $t5,0($t2) # load from the start iterator
    bne $t4,$t5,is_not_palindrome_end # if it is not a palindrom print and return
    # if(i == 0) branch palindrome
    beq $0,$t1,is_palindrome_end # if (i == zero) it is a palindrome

    addi $t1,$t1,-1 # decrement i
    addi $t3,$t3,-1 # decrement end iterator
    addi $t2,$t2,1 # increment start iterator
    
    j palindrome_loop

# branch if it is decided to be a palindrome
is_not_palindrome_end:
    lw $ra,0($sp) # restore 1 word
    addi $sp,$sp,4 # allocate 1 word
    # print and return
    la $a1,($a0) # load the word
    la $a0,($s3) # put the global index here
    la $a2,is_not_palindrom_mes
    # print_palindrome set
    li $v0,28
    # process_print_palindrome(int,str,str)
    syscall
    # return to caller
    jr $ra
# branch if it is not a palindrome
is_palindrome_end:
    lw $ra,0($sp) # restore 1 word
    addi $sp,$sp,4 # allocate 1 word
    # print and return
    la $a1,($a0) # load the word
    la $a0,($s3) # put the global index here
    la $a2,is_palindrom_mes
    # print_palindrome set
    li $v0,28
    # process_print_palindrome(int,str,str)
    syscall
    # return to caller
    jr $ra

# if invalid
is_invalid:
    lw $ra,0($sp) # restore 1 word
    addi $sp,$sp,4 # allocate 1 word
    # print and return
    
    la $a0,error_mes
    li $v0,27
    # process_print_string(str)
    syscall

    # return
    jr $ra

################################is_palindrom end############################