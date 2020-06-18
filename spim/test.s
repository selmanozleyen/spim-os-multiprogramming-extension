.data
filename: .asciiz "Collatz.asm"
filename2: .asciiz "BinarySearch.asm"
filename3: .asciiz "LinearSearch.asm"
prompt: .asciiz "\nExitting Kernel...\n"
debug1: .asciiz "\here before sw\n"
debug2: .asciiz "In Parent\n"
debug3: .asciiz "In Child\n"
debug4: .asciiz "In CS part\n"
debug5: .asciiz "IN child 1\n"
debug6: .asciiz "IN child 2\n"
debug7: .asciiz "IN child 3\n"
debug8: .asciiz "IN child 4\n"
.text
.globl main

main:
############################################################################
# void init()
# init
# calls the syscalls to initialize the processes.
############################################################################
init:    
    # Even though k0 is initially 0 it is set again
    # Disabling interrupts
    ori $k0,$0,0
    # Critical section with no interrupts
    jal _init_process_structure
    # Enabling interrupts
    ori $k0,$0,1

    # Critical section blocking interrupts
    ori $k0,$0,0 # Blocked Interrupts
    # Fork syscall
    li $v0,18
    syscall
    beq $v0,$0,child_process
    # Unblock interrupts
    ori $k0,$0,1 # Unblocked Interrupts

    #child 2
    # Critical section blocking interrupts
    ori $k0,$0,0 # Blocked Interrupts
    # Fork syscall
    li $v0,18
    syscall
    beq $v0,$0,child_process2
    # Unblock interrupts
    ori $k0,$0,1 # Unblocked Interrupts

    #child 3
    # Critical section blocking interrupts
    ori $k0,$0,0 # Blocked Interrupts
    # Fork syscall
    li $v0,18
    syscall
    beq $v0,$0,child_process3
    # Unblock interrupts
    ori $k0,$0,1 # Unblocked Interrupts

    #child 4
    # Critical section blocking interrupts
    ori $k0,$0,0 # Blocked Interrupts
    # Fork syscall
    li $v0,18
    syscall
    beq $v0,$0,child_process4
    # Unblock interrupts
    ori $k0,$0,1 # Unblocked Interrupts


loop:
    ori $k0,$0,0 # Blocked Interrupts
    la $a0,debug2
    li $v0,4
    syscall
    ori $k0,$0,1 # Unblocked Interrupts
    j loop

    # the loop for waiting children is over.
    # prompt the user that the kernel is exitting
    # print("Exiting kernel...\n")
    li $v0,4
    la $a0,prompt
    syscall
    

    # Close the kernel
    # exit_process_syscall()
    li $v0,8
    syscall

child_process:
    la $a0,debug3
    li $v0,4
    syscall
    ori $k0,$0,1 # Unblocked Interrupts
    j child_process

child_process2:
    la $a0,debug5
    li $v0,4
    syscall
    ori $k0,$0,1 # Unblocked Interrupts
    j child_process2

child_process3:
    la $a0,debug6
    li $v0,4
    syscall
    ori $k0,$0,1 # Unblocked Interrupts
    j child_process3

child_process4:
    la $a0,debug7
    li $v0,4
    syscall
    ori $k0,$0,1 # Unblocked Interrupts
    j child_process4

child_process5:
    la $a0,debug8
    li $v0,4
    syscall
    ori $k0,$0,1 # Unblocked Interrupts
    j child_process5
############################# HANDLER PART ##################################
.data
# There can be at most process 100 process created in this OS
# These will be  arrays
.globl ready_processes
ready_processes: .word 0:100
.globl blocked_processes
blocked_processes: .word 0:100
# List of process process_states[i] will give state of process with id i
.globl process_states
process_states: .word 0:100
# kernel registers and the
# k0 for interrupt enable
# k1 for user PC
.globl kernel_regs
kernel_regs: .word 0:16
user_regs: .word 0:17
# current process => $k0
# limit of the ready and blocked processes list is 100
p_lim: .word 100
interrupt_prompt: .asciiz "Interrupt happened, in kernel...\n"
############################################################################
# void handle_interrupt()
# _handle_interrupt
# When an interrupt occurs, the os will execute this function.
# STATE_NO's : RUNNING = 1, BLOCKED = 2, READY = 3, TERMINATED = 0
# k0 -> interrupt enable bit
# k1 -> current pid
# s0 -> size of blocked process list
# s1 -> size of ready process list
# s2 -> limit of active processes
# s3 -> user PC
############################################################################
.text
.globl _handle_interrupt
_handle_interrupt:

    # Load the registers for kernel and PC
    
    # SAVE USER REGISTERS
    # first the arguments
    sw $a0,user_regs # a0,a1,a2,a3 being stored
    sw $a1,user_regs+4
    sw $a2,user_regs+8
    sw $a3,user_regs+12
    # Then the return registers
    sw $v0,user_regs+16
    sw $v1,user_regs+20
    # Temporary registers
    sw $t0,user_regs+24 # user_regs[6]=$t0
    sw $t1,user_regs+28 # user_regs[7]=$t1
    sw $t2,user_regs+32 # user_regs[8]=$t2
    sw $t3,user_regs+36 # user_regs[9]=$t3
    sw $t4,user_regs+40 # user_regs[10]=$t4
    sw $t5,user_regs+44 # user_regs[11]=$t5
    sw $t6,user_regs+48 # user_regs[12]=$t6
    sw $t7,user_regs+52 # user_regs[13]=$t7
    sw $t8,user_regs+56 # user_regs[14]=$t8
    sw $t9,user_regs+60 # user_regs[15]=$t9
    # RESTORE THE KERNEL REGISTERS
    # first the arguments
    lw $a0,kernel_regs # a0,a1,a2,a3 being restored
    lw $a1,kernel_regs+4
    lw $a2,kernel_regs+8
    lw $a3,kernel_regs+12
    # Then the return registers
    lw $v0,kernel_regs+16
    lw $v1,kernel_regs+20
    # Temporary registers
    lw $t0,kernel_regs+24 # $t0 = kernel_regs[6]
    lw $t1,kernel_regs+28 # $t1 = kernel_regs[7]
    lw $t2,kernel_regs+32 # $t2 = kernel_regs[8]
    lw $t3,kernel_regs+36 # $t3 = kernel_regs[9]
    lw $t4,kernel_regs+40 # $t4 = kernel_regs[10]
    lw $t5,kernel_regs+44 # $t5 = kernel_regs[11]
    lw $t6,kernel_regs+48 # $t6 = kernel_regs[12]
    lw $t7,kernel_regs+52 # $t7 = kernel_regs[13]
    lw $t8,kernel_regs+56 # $t8 = kernel_regs[14]
    lw $t9,kernel_regs+60 # $t9 = kernel_regs[15]
    #lw $ra,kernel_regs+64 # $ra = kernel_regs[16]


    # print to inform 
    # print(interrupt_prompt)
    li $v0,4
    la $a0,interrupt_prompt
    syscall
    # Do a context switch if the ready queue is not empty.
    # if(0 != ready_queue_size) branch;
    bne $0,$s1,ready_queue_not_empty
    # Label to return after branch
ready_queue_not_empty_return:
    
    # SAVE KERNEL REGISTERS
    # first the arguments
    sw $a0,kernel_regs # a0,a1,a2,a3 being stored
    sw $a1,kernel_regs+4
    sw $a2,kernel_regs+8
    sw $a3,kernel_regs+12
    # Then the return registers
    sw $v0,kernel_regs+16
    sw $v1,kernel_regs+20
    # Temporary registers
    sw $t0,kernel_regs+24 # kernel_regs[6]=$t0
    sw $t1,kernel_regs+28 # kernel_regs[7]=$t1
    sw $t2,kernel_regs+32 # kernel_regs[8]=$t2
    sw $t3,kernel_regs+36 # kernel_regs[9]=$t3
    sw $t4,kernel_regs+40 # kernel_regs[10]=$t4
    sw $t5,kernel_regs+44 # kernel_regs[11]=$t5
    sw $t6,kernel_regs+48 # kernel_regs[12]=$t6
    sw $t7,kernel_regs+52 # kernel_regs[13]=$t7
    sw $t8,kernel_regs+56 # kernel_regs[14]=$t8
    sw $t9,kernel_regs+60 # kernel_regs[15]=$t9
    #sw $ra,kernel_regs+64 # kernel_regs[16]=$ra

    # RESTORE THE USER REGISTERS
    # first the arguments
    lw $a0,user_regs # a0,a1,a2,a3 being restored
    lw $a1,user_regs+4
    lw $a2,user_regs+8
    lw $a3,user_regs+12
    # Then the return registers
    lw $v0,user_regs+16
    lw $v1,user_regs+20
    # Temporary registers
    lw $t0,user_regs+24 # $t0 = user_regs[6]
    lw $t1,user_regs+28 # $t1 = user_regs[7]
    lw $t2,user_regs+32 # $t2 = user_regs[8]
    lw $t3,user_regs+36 # $t3 = user_regs[9]
    lw $t4,user_regs+40 # $t4 = user_regs[10]
    lw $t5,user_regs+44 # $t5 = user_regs[11]
    lw $t6,user_regs+48 # $t6 = user_regs[12]
    lw $t7,user_regs+52 # $t7 = user_regs[13]
    lw $t8,user_regs+56 # $t8 = user_regs[14]
    lw $t9,user_regs+60 # $t9 = user_regs[15]
    
    # Load the process id in $k0
    la $v0,25
    la $a0,0($k1)
    # load_process($k0) syscall
    # will also enable interrupts
    syscall
    # end of execution

# There will be a context switch
ready_queue_not_empty:
    # save the $tx registers
    jal save_routine

    #here do a debug
    la $a0,debug4
    li $v0,4
    syscall

    # pop the front of the queue and add the current process to the end of the queue
    # put the ready process list as argument
    la $a0,ready_processes
    # put the size of the queue
    la $a1,0($s1)
    jal pop_front
    # decrement the size of the queue after calling pop_front
    # $t0 = new current process
    la $t0,0($v0)

    # decrementing the size of the queue
    addi $s1,$s1,-1

    # the pointer remains the same
    # put the size of the queue
    la $a1,0($s1)
    # put the current process as argument
    la $a2,0($k1)
    # push the old process
    jal push_end


    # incrementing the size of the queue
    addi $s1,$s1,1

    # save the old process
    la $t3,0($k1)

    # set the new process
    # k1 = t0
    la $k1,0($t0)

    # update the status list
    # t1 = 4*k1
    sll $t1,$k1,2
    # t1 += status_arr
    # set $t4 as process_states pointer
    la $t4,process_states
    add $t1,$t1,$t4
    # t2 = RUNNING
    li $t2,1
    # status_arr[k0] = RUNNING
    lw $t2,0($t1)

    #t2 = READY
    li $t2,3
    #t1 = 4*t3 (old process)
    sll $t1,$t3,2    
    #t1 += status_arr
    add $t1,$t1,$t4
    # status_arr[t3] = READY
    lw $t2,0($t1)

    # restore the t registers
    jal restore_routine

    # store the init process except the PC and $ra
    li $a0,0
    li $a1,2
    # Mode to save init without PC
    li $v0,24
    # store_process(0,2) syscall
    syscall

    

    #here do a debug
    la $a0,debug4
    li $v0,4
    syscall

    # return to the branch end
    j ready_queue_not_empty_return

#########################_handle_interrupt end##############################


###########################################################################
# int pop_front(int * arr, int size)
# pop_front
# $a0 the array to be popped
# $a1 the size of the array
# $v0 the popped value 
# A simple utility function to manipulate the process array
# after calling this the caller should decrement the size of its array
###########################################################################
pop_front:
    # save $ra to the stack
    addi $sp, $sp, -4
    sw $ra, 0($sp)
    # Do the save routine
    jal save_routine

    # $v0 = arr[0]
    # The value to return is assigned
    lw $v0,0($a0)

    # Do a loop to shift the elements of the array
    # i = 0
    # while(size - 1 > i){
    #     arr[i] = arr[i+1]
    #     i++
    # }
    # $t2 = temp = size - 1
    addi $t2,$a1,-1
    # $t1 = arr + offset
    # $a1 = size
    # $t0 = i = 0
    li $t0,0
pop_loop:
    # $t3 = res =   i $t0 < size - 1 ($t2)
    slt $t3,$t0,$t2
    # if $t3 == 0 end the loop
    beq $0,$t3,pop_loop_end
    
    # set $t1 to arr + 4*i
    sll $t4,$t0,2
    add $t1,$a0,$t4

    # get the value of arr[i+1] and save it to $t4
    lw $t4,4($t1)
    # set arr[i] to $t4
    sw $t4,0($t1)

    # increment i ($ t0)
    addi $t0,$t0,1
    # i++
    # restart the loop
    j pop_loop

pop_loop_end:

    # Do the restore routine
    jal restore_routine
    # Restore routine end

    # restore $ra from the stack
    lw $ra, 0($sp)
    addi $sp, $sp,4
    # return to the caller
    j $ra
######################## pop_front end ####################################    


###########################################################################
# void push_end(int * arr, int size, int item)
# push_end
# $a0 the array to be popped
# $a1 the size of the array
# $a2 the item to push 
# A simple utility function to manipulate the process array
# after calling this the caller should decrement the size of its array
###########################################################################
push_end:
    # save $t0
    addi $sp, $sp, -4   
    sw $t0, 0($sp)

    # arr[size] = item
    # $t0 = 4*size
    sll $t0,$a1,2
    # $t0 += arr
    add $t0,$t0,$a0
    # *(arr + size) = item
    sw $a2,0($t0)

    # restore $t0
    lw $t0, 0($sp)
    addi $sp, $sp,4   
    
    # return to caller
    jr $ra

########################## push_end end ##################################

# This is a routine for saving the
# current registers to the stack
save_routine:
    # with this routine $t0-$t7 are saved
    # save routine
    addi $sp, $sp, -32
    # push the stack pointer for 8 elements
    # sp[0] = t0
    sw $t0, 0($sp)
    # sp[1] = t1
    sw $t1, 4($sp)
    # sp[2] = t2
    sw $t2, 8($sp) 
    # sp[3] = t3
    sw $t3, 12($sp) 
    # sp[4] = t4
    sw $t4, 16($sp) 
    # sp[5] = t5
    sw $t5, 20($sp) 
    # sp[6] = t6
    sw $t6, 24($sp) 
    # sp[7] = t7
    sw $t7, 28($sp)
        
    jal $ra

# This is a routine for loading the
# current registers with the stack
restore_routine:
    # with this routine $t0-$t7 are restored
    # load routine 
    # t0 = sp[0]
    lw $t0, 0($sp)
    #  t1 = sp[1]
    lw $t1, 4($sp)
    #  t2 = sp[2]
    lw $t2, 8($sp) 
    #  t3 = sp[3]
    lw $t3, 12($sp) 
    #  t4 = sp[4]
    lw $t4, 16($sp) 
    #  t5 = sp[5]
    lw $t5, 20($sp) 
    #  t6 = sp[6]
    lw $t6, 24($sp) 
    #  t7 = sp[7]
    lw $t7, 28($sp)

    addi $sp, $sp, 32
    # pop the stack pointer for 8 elements
    # return to the caller 
    jr $ra

############################################################################
# void init_process_structure()
# _init_process_structure
# The process lists will be initialized here. 
# $ra will not be saved
# STATE_NO's : RUNNING = 1, BLOCKED = 2, READY = 3, TERMINATED = 0
# k0 -> interrupt enable bit
# k1 -> current pid
# s0 -> size of blocked process list
# s1 -> size of ready process list
# s2 -> limit of active processes
# s3 -> user PC
############################################################################
_init_process_structure:
    # Disabling interrupts
    # k0 will be used for interrupt handling mask
    
    # This is the init process for the kernel.
    # First the process structures should be initialized.
    # init_process_structure() syscall
    
    # switch to kernel mode
    # SAVE USER REGISTERS
    # first the arguments
    sw $a0,user_regs # a0,a1,a2,a3 being stored
    sw $a1,user_regs+4
    sw $a2,user_regs+8
    sw $a3,user_regs+12
    # Then the return registers
    sw $v0,user_regs+16
    sw $v1,user_regs+20
    # Temporary registers
    sw $t0,user_regs+24 # user_regs[6]=$t0
    sw $t1,user_regs+28 # user_regs[7]=$t1
    sw $t2,user_regs+32 # user_regs[8]=$t2
    sw $t3,user_regs+36 # user_regs[9]=$t3
    sw $t4,user_regs+40 # user_regs[10]=$t4
    sw $t5,user_regs+44 # user_regs[11]=$t5
    sw $t6,user_regs+48 # user_regs[12]=$t6
    sw $t7,user_regs+52 # user_regs[13]=$t7
    sw $t8,user_regs+56 # user_regs[14]=$t8
    sw $t9,user_regs+60 # user_regs[15]=$t9

    li $v0,23
    syscall
    # will register this process as the init process.
    li $k0,0
    # set k1 as current process
    li $k1,0
    # setting the sizes of the process lists
    # $s0 = size of the ready process list
    li $s0,0
    # $s1 = size of the blocked process list
    li $s1,0
    # set limit of active processes
    lw $s2,p_lim
    # temp $t7 = RUNNING state
    li $t7,1
    # process_states[0] = $t7 (RUNNING STATE)
    sw $t7,process_states

    # Save these registers for the kernel mode
    # SAVE KERNEL REGISTERS
    # first the arguments
    sw $a0,kernel_regs # a0,a1,a2,a3 being stored
    sw $a1,kernel_regs+4
    sw $a2,kernel_regs+8
    sw $a3,kernel_regs+12
    # Then the return registers
    sw $v0,kernel_regs+16
    sw $v1,kernel_regs+20
    # Temporary registers
    sw $t0,kernel_regs+24 # kernel_regs[6]=$t0
    sw $t1,kernel_regs+28 # kernel_regs[7]=$t1
    sw $t2,kernel_regs+32 # kernel_regs[8]=$t2
    sw $t3,kernel_regs+36 # kernel_regs[9]=$t3
    sw $t4,kernel_regs+40 # kernel_regs[10]=$t4
    sw $t5,kernel_regs+44 # kernel_regs[11]=$t5
    sw $t6,kernel_regs+48 # kernel_regs[12]=$t6
    sw $t7,kernel_regs+52 # kernel_regs[13]=$t7
    sw $t8,kernel_regs+56 # kernel_regs[14]=$t8
    sw $t9,kernel_regs+60 # kernel_regs[15]=$t9
    #sw $ra,kernel_regs+64 # kernel_regs[16]=$ra

    # RESTORE THE USER REGISTERS
    # first the arguments
    lw $a0,user_regs # a0,a1,a2,a3 being restored
    lw $a1,user_regs+4
    lw $a2,user_regs+8
    lw $a3,user_regs+12
    # Then the return registers
    lw $v0,user_regs+16
    lw $v1,user_regs+20
    # Temporary registers
    lw $t0,user_regs+24 # $t0 = user_regs[6]
    lw $t1,user_regs+28 # $t1 = user_regs[7]
    lw $t2,user_regs+32 # $t2 = user_regs[8]
    lw $t3,user_regs+36 # $t3 = user_regs[9]
    lw $t4,user_regs+40 # $t4 = user_regs[10]
    lw $t5,user_regs+44 # $t5 = user_regs[11]
    lw $t6,user_regs+48 # $t6 = user_regs[12]
    lw $t7,user_regs+52 # $t7 = user_regs[13]
    lw $t8,user_regs+56 # $t8 = user_regs[14]
    lw $t9,user_regs+60 # $t9 = user_regs[15]
    # return to the caller
    jr $ra
########################## _init_process_structure end #####################
