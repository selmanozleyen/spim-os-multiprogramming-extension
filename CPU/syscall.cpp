/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"
#include "spim-utils.h"
#include "run.h"
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "data.h"
#include <string>

using namespace std;

#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>




void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
  if (function != NULL)
    {
      run_error ("Bad parameter to system call: %s\n", function);
    }
  else
    {
      run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
  static _invalid_parameter_handler oldHandler;
  static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

  if (flag == 0)
    {
      oldHandler = _set_invalid_parameter_handler(newHandler);
      _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
  else
    {
      newHandler = _set_invalid_parameter_handler(oldHandler);
      _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif


int next_pid = 0;
process * init_process = NULL;
/* 
  The process table is actually in data segment these parts
  contains the context about the procces.
  Relation with other processes is not contained here
  but it is stored in assembly.
 */
process *  processes[PROCESS_COUNT] = {NULL};

/* names of the labels stored here */
// name of the interrupt functions
char * timer_interrupt_handler_name = "_handle_interrupt";
char * ready_processes_lname = "ready_processes";
char * blocked_processes_lname = "blocked_processes";
char * process_states_lname = "process_states";
char * bl_size_lname = "bl_size";
char * rl_size_lname = "rl_size";
char * state_names[] = {"CORRUPTION","RUNNING","BLOCKED","READY","TERMINATED"};
/* pointers to the data segment of assembly file */
int ready_processes_ptr = 0;
int blocked_processes_ptr = 0;
int process_state_ptr = 0;
int handler_ptr = 0;
int bl_size_ptr = 0;
int rl_size_ptr = 0;

void SPIM_timerHandler()
{ 
  // interrupts are disabled when the current process running is init process
  int init_status = my_read_mem_word(process_state_ptr,init_process);

  if(R[K0_REG]==0 && init_status == BLOCKED && init_process != NULL){
    // Report user
    printf("Kernel: Interrupt happened executing handler.\n");
    // Get current process id
    int cur_pid = R[K1_REG];
    // Save the proccess with its current state before interrupt handling
    processes[cur_pid]->store_process();
    init_process->load_process();
    PC = handler_ptr;
  } 
}

//  SYSCALL FUNCION that loads the next selected pid by the assembly file
void kernel_load_next_process(int pid){
  // store the previous values
  R[K1_REG] = pid; 
  R[K0_REG] = 0;
  processes[pid]->load_process();
  processes[pid]->_PC -= BYTES_PER_WORD;
  // after syscall is over pc will be incremented to equalize this PC is decremented
  PC -= BYTES_PER_WORD;
  // after context switch print the table
  print_table();
}

/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */


int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif

  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  switch (R[REG_V0])
    {
    // Provides Atomic Formatted Printing For pretty and testable results
    case PROCESS_PALINDROME_PRINT:
      palindrome_print(R[REG_A0],
      (char *)mem_reference(R[REG_A1]),
      (char *)mem_reference(R[REG_A2]));
      return 1;
    case PROCESS_PRINT_INT:
      process_print_int(R[REG_A0]);
      return 1;
    case PROCESS_PRINT_STRING:
      process_print_string((char *)mem_reference(R[REG_A0]));
      return 1;
    // This syscall will store the current registers and data segments
    // to the given process
    case LOAD_NEXT_PROCESS_SYSCALL:
      kernel_load_next_process(R[REG_A0]);
      return(1);
    case PRINT_PT_SYSCALL:
      print_table();
      break;
    // This syscall will be called by the kernel only
    // It will initialize the process structure in the
    // Os emulator
    case INIT_PROCESS_STRUCTURE:
      init_kernel();
      break;
    case PROCESS_EXIT_SYSCALL:
      process_exit();
      return (1);
    case RANDOM_INT_SYSCALL:
      if(R[REG_A0]<0)
        perror("Invalid upper bound");
      R[REG_V0] = rand()%R[REG_A0];
      break;
    case WAITPID_SYSCALL:
      spim_wait();
      return(1);
      break;
    case EXECV_SYSCALL:
      spim_execv((char *) mem_reference (R[REG_A0]));
      return (1);
      break;
    case FORK_SYSCALL:
      spim_fork();
      return (1);
      break;
    case PRINT_INT_SYSCALL:

      write_output (console_out, "%d", R[REG_A0]);
      break;

    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (REG_FA0);

	write_output (console_out, "%.8f", val);
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      break;

    case PRINT_STRING_SYSCALL:
      write_output (console_out, "%s", mem_reference (R[REG_A0]));
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	R[REG_RES] = atol (str);
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR_S (REG_FRES) = (float) atof (str);
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR [REG_FRES] = atof (str);
	break;
      }

    case READ_STRING_SYSCALL:
      {
	read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
	data_modified = true;
	break;
      }

    case SBRK_SYSCALL:
      {
	mem_addr x = data_top;
	expand_data (R[REG_A0]);
	R[REG_RES] = x;
	data_modified = true;
	break;
      }

    case PRINT_CHARACTER_SYSCALL:
      write_output (console_out, "%c", R[REG_A0]);
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];

	read_input (str, 2);
	if (*str == '\0') *str = '\n';      /* makes xspim = spim */
	R[REG_RES] = (long) str[0];
	break;
      }

    case EXIT_SYSCALL:
      spim_return_value = 0;
      return (0);

    case EXIT2_SYSCALL:
      spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
      return (0);

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
	R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
	break;
      }

    case READ_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	data_modified = true;
	break;
      }

    case WRITE_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
	R[REG_RES] = _close(R[REG_A0]);
#else
	R[REG_RES] = close(R[REG_A0]);
#endif
	break;
      }

    default:
      run_error ("Unknown system call: %d\n", R[REG_V0]);
      break;
    }
#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
  return (1);
}


void
handle_exception ()
{
  if (!quiet && CP0_ExCode != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
	error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_AdES:
      if (!quiet)
	error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_IBE:
      if (!quiet)
	error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_DBE:
      if (!quiet)
	error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_Sys:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
	error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
	error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
	error ("  Floating point\n");
      break;

    default:
      if (!quiet)
	error ("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}

// empties currently allocated globals by the process
void empty_curr(){
  // frees them before emptying
  free(data_seg);
  free(text_seg);
  free(k_data_seg);
  free(stack_seg);
  data_seg = NULL;
  data_seg_h = NULL;
  data_seg_b = NULL;
  text_seg = NULL;
  k_data_seg = NULL;
  k_data_seg_h = NULL;
  k_data_seg_b = NULL;
  k_text_seg = NULL;
  stack_seg = NULL;
}

//Copies the current process
void spim_fork(){
  // copied the process
  // cur_pid = $k1
  int cur_pid = R[K1_REG];
  process * cur_process = processes[cur_pid];
  cur_process->store_process();
  process * newp = new process(cur_process->_process_name.data());
  processes[newp->_pid] = newp;
  // Set the return value for child process
  newp->_R[REG_V0] = 0;
  // Set the return value for parent process
  cur_process->_R[REG_V0] = newp->_pid;
  // New process PC will be incremented
  newp->_PC = PC + BYTES_PER_WORD;

  // set the parent
  newp->_parent = cur_process;
  //ready_processes.push_back(newp);
  // add to childs
  cur_process->childs.push_back(newp);
  int newpid = newp->_pid;

  // change size of the ready queue
  // size of the ready queue is in $s1 of the init process
  int size = my_read_mem_word(rl_size_ptr,init_process);
  my_set_mem_word(rl_size_ptr,size+1,init_process);
  
  // set the end of the list to the new pid
  my_set_mem_word(ready_processes_ptr+BYTES_PER_WORD*(size),
  newp->_pid,
  init_process);
  // set the status of the process to ready
  my_set_mem_word(process_state_ptr+BYTES_PER_WORD*newpid,
  READY,
  init_process);
  // if this is the kernel process then it must be loaded again to update values
  if(cur_process == init_process)
    init_process->load_process();
  // Report to the terminal that fork is over
  printf("Kernel: Fork syscall is done.\n");
}

// Will create a new memory segments and load the process there
void spim_execv(char * path){
  // Setting the name for process below
  string temp = string(path);
  int last_index1,last_index2;
  last_index1  = temp.find_last_of('\\');
  last_index2 = temp.find_last_of('/');
  last_index1 = MAX(last_index1,last_index2);
  if (last_index1 < 0)
    last_index1 = 0;
  // get the current process from $k1 register
  process * cur_process = processes[R[K1_REG]];
  cur_process->_process_name = string(path+last_index1);
  // empty  current globals but before save the kernel registers
  int k0 = R[K0_REG];
  int k1 = R[K1_REG];

  /* Series of functions to create new memory segment STARTS HERE*/
  empty_curr();
  make_memory (initial_text_size,
	       initial_data_size, initial_data_limit,
	       initial_stack_size, initial_stack_limit,
	       initial_k_text_size,
	       initial_k_data_size, initial_k_data_limit);
  initialize_registers ();
  initialize_symbol_table ();
  k_text_begins_at_point (K_TEXT_BOT);
  k_data_begins_at_point (K_DATA_BOT);
  data_begins_at_point (DATA_BOT);
    /* Series of functions to create new memory segment ENDS HERE*/
  // store the kernel registers back
  R[K0_REG] = k0;
  R[K1_REG] = k1;
  // save the name of the process
  char * p = strdup(temp.data());
  // read the process from the file
  read_assembly_file(p);
  // decrement the value because syscall will incement it
  PC = find_symbol_address("main")-BYTES_PER_WORD;
  // store it in the procces contents
  cur_process->store_process();
  free(p);
  // Report to the user
  printf("Kernel: Execv syscall is done.\n");
}

// Returns the child status -1 if there is none
void spim_wait(){
  // set the current proccess from assembly file
  int cur_pid = R[K1_REG];
  process * cur_process = processes[cur_pid];
  if(cur_process->childs.empty()){
    R[REG_V0] = -1;
    cur_process->_R[REG_V0] = -1;
  }
  else{
    // push this process to blocked procceses
    // get the new size of block list 
    // note that $s0 is the size of the blocked registers
    int size_bl = my_read_mem_word(bl_size_ptr,init_process) + 1;
    // set the end of the list to the new pid
    my_set_mem_word(blocked_processes_ptr+BYTES_PER_WORD*(size_bl-1),
    cur_process->_pid,
    init_process);
    // set the status of the process to blocked
    my_set_mem_word(process_state_ptr+BYTES_PER_WORD*cur_pid,
    BLOCKED,
    init_process);
    // save the state of the current process
    cur_process->store_process();
    // increment the blocked process values after the store functions
    my_set_mem_word(bl_size_ptr,size_bl,init_process);
    // context switch is going to happen here
    // get a process from the ready list
    // $s1 is the size register for ready queue
    int size_rl = my_read_mem_word(rl_size_ptr,init_process);
    // if there are none give an error
    if(size_rl <= 0){
      perror("There are no other processes");
      _exit(-1);
    }
    // get from the front of the queue
    int newpid = my_read_mem_word(ready_processes_ptr,init_process);
    int i = 0,next_val = 0;
    // shift the values from left to right
    while(i < size_rl -1){
      // readyq[i]
      next_val = my_read_mem_word(ready_processes_ptr+BYTES_PER_WORD*(i+1),
      init_process);
      // readyq[i] = readyq[i+1]
      my_set_mem_word(ready_processes_ptr+BYTES_PER_WORD*i,
      next_val,
      init_process);

      i++;
    }
    // shifted now decrement the s1 register = size of ready queue
    my_set_mem_word(rl_size_ptr,size_rl-1,init_process);
    
    R[K1_REG] = newpid;
    // set newpid as running
    my_set_mem_word(process_state_ptr + newpid*BYTES_PER_WORD,
    RUNNING,init_process);
    // Set new current process
    cur_process = processes[newpid];
    // Decrement the pc because when syscall ends it will incement it
    cur_process->_PC-= BYTES_PER_WORD;
    cur_process->load_process();
  }
  print_table();
}

// function to print the table of processes in assembly file
void print_table(){
  
  /* Getting proccess table from assembly part and storing it */
  int allocated_pids = next_pid;
  /* Allocate for lists */
  int * state_l = (int *) malloc(sizeof(int)*allocated_pids);
  /* Set the state list from assembly data segment */
  for (int i = 0; i < allocated_pids; i++)
  {
    state_l[i] = my_read_mem_word(
    process_state_ptr+i*BYTES_PER_WORD,
    init_process);
  }
  printf("Kernel: Context switch happened here is the process table.\n");
  printf("pid%*sstate%*sname%*sPC%*sstack_pointer\n",19,"",17,"",18,"",20,"");
  for (int i = 0; i < allocated_pids; i++)
  {
    if(state_l[i] == TERMINATED){
      printf("%-20d  %-20s  %-20s  %-20d  %-20d\n",i,
        state_names[TERMINATED],
        state_names[TERMINATED],
        -1,-1
      );
    }
    else{
      printf("%-20d  %-20s  %-20s  %-20d  %-20p\n",i,
        state_names[state_l[i]],
        processes[i]->_process_name.data(),
        processes[i]->_PC,(void *)processes[i]->_stack_seg
      );
    }
  }
  fflush(stdout);
  // free allocated array
  free(state_l);
}

// free the resources allocated by the process
void free_process(process * p){
  int instruction_count = (p->_text_top - TEXT_BOT )>> 2;
  for (int i = 0; i < instruction_count; i++){
    if(p->_text_seg[i])
      free_inst(p->_text_seg[i]);
  }
  free(p->_data_seg);
  free(p->_text_seg);
  free(p->_stack_seg);
  free(p);
}

// modified function to set the word in a desired process
void my_set_mem_word(mem_addr addr, reg_word value, process * p)
{
  if ((addr >= DATA_BOT) && (addr < data_top) && !(addr & 0x3))
    p->_data_seg [(addr - DATA_BOT) >> 2] = (mem_word) value;
  else if ((addr >= stack_bot) && (addr < STACK_TOP) && !(addr & 0x3))
    p->_stack_seg [(addr - stack_bot) >> 2] = (mem_word) value;
  else{
    perror("my_set_mem_word");
    _exit(-1);
  }
}

// modified function to get the word in a desired process
reg_word my_read_mem_word(mem_addr addr,process * p)
{
  if ((addr >= DATA_BOT) && (addr < data_top) && !(addr & 0x3))
    return p->_data_seg [(addr - DATA_BOT) >> 2];
  else if ((addr >= stack_bot) && (addr < STACK_TOP) && !(addr & 0x3))
    return p->_stack_seg [(addr - stack_bot) >> 2];
  else{
    perror("my_read_mem_word");
    _exit(-1);
  }
}

// exits from the process and switchs to another process if there is
// Also if the process is being waitied on waited process will be woken up
void process_exit() 
{
  int old_pid = R[K1_REG];
  process * oldp = processes[old_pid];
  oldp->store_process();
  // if it is the init process then close the kernel
  if(oldp->_pid == 0){
    _exit(0);
  }
  // get the size of the blocked queue
  int size_bl = my_read_mem_word(bl_size_ptr,init_process);
  // if the parent of oldp is blocked
  int parent_pid = oldp->_parent->_pid;
  process * parentp = processes[parent_pid];
  // get the status of parent from assembly
  int parent_status = my_read_mem_word(process_state_ptr+parent_pid*BYTES_PER_WORD,
  init_process); 
  
  // set the current pid status to terminated
  my_set_mem_word(process_state_ptr + old_pid*BYTES_PER_WORD,
  TERMINATED,init_process);
  free_process(oldp);

  if(parent_status == BLOCKED){
      // switch to parent
      int i = 0;
      int found_pos = 0;
      int iter_pid = 0;
      int found = 0;
      // find the process in the blocked queue
      for(; i < size_bl && !found; i++){
        // iter_pid = blocked_processes[i] (is the same as below)
        iter_pid = my_read_mem_word(blocked_processes_ptr + i*(BYTES_PER_WORD),
        init_process
        );
        // if found store the related infos
        if (iter_pid == parent_pid){
          found = 1;
          found_pos = i;
        }
      }
      // Give an error if there is no pid in the blocked list
      if(!found){
        perror("Blocked list corrupted");
        _exit(-1);
      }
      // Remove the pid from the blocked list    
      // set the position to be replaced
      i = found_pos;
      int next_val = 0;
      // shift the values from left to right
      while(i < size_bl - 1){
        // save blockedq[i+1]
        next_val = my_read_mem_word(
        blocked_processes_ptr+BYTES_PER_WORD*(i+1),
        init_process);
        // blockedq[i] = blockedq[i+1]
        my_set_mem_word(blocked_processes_ptr + BYTES_PER_WORD*i,
        next_val,
        init_process);

        i++;
      }
      // decrement the size of the blocked list
      my_set_mem_word(bl_size_ptr,size_bl-1,init_process);

      // remove from parents child
      auto iter = find(parentp->childs.begin(),parentp->childs.end(),oldp);
      if(*iter != oldp){
        perror("Corruption Error");
        _exit(-1);
      }
      parentp->childs.erase(iter);

      // set the new_pid $k1
      R[K1_REG] = parent_pid;
      // set parent to running
      my_set_mem_word(process_state_ptr + parent_pid*BYTES_PER_WORD,
      RUNNING,init_process);
      // load the parent process
      processes[parent_pid]->load_process();
  }
  else{
      // do round robin 
      int size_rl = my_read_mem_word(rl_size_ptr,init_process);
      // if there are none give an error
      if(size_rl <= 0){
        fprintf(stderr,"There are no other processes that can run.\n");
        _exit(1);
      }
      // get from the front of the queue
      int newpid = my_read_mem_word(ready_processes_ptr,init_process);
      int i = 0,next_val = 0;
      // shift the values from left to right
      while(i < size_rl -1){
        // readyq[i]
        next_val = my_read_mem_word(ready_processes_ptr+BYTES_PER_WORD*(i+1),
        init_process);
        // readyq[i] = readyq[i+1]
        my_set_mem_word(ready_processes_ptr,
        next_val,
        init_process);

        i++;
      }
      // shifted now decrement the s1 register = size of ready queue
      my_set_mem_word(rl_size_ptr,size_rl-1,init_process);
      R[K1_REG] = newpid;
      // set newpid as running
      my_set_mem_word(process_state_ptr + newpid*BYTES_PER_WORD,
      RUNNING,init_process);
      // Set new current process
      process * cur_process = processes[newpid];
      // Decrement the pc because when syscall ends it will incement it
      cur_process->_PC-= BYTES_PER_WORD;
      cur_process->load_process();
    }
    /* Set the table to NULL after freeing and comparing the address of oldp */
    processes[old_pid] = NULL;
    oldp = NULL;
    printf("Kernel: Exit process syscall is done process no %d exited\n",old_pid);
    print_table();
}

// Initializes some memory part for the kernel to work properly
// Will be called in the start of the kernel
void init_kernel(){
  srand(time(NULL)); // seed for the rand syscall
  // create the process struct
  init_process = new process("init");
  processes[0] = init_process;
  // Set parent to NULL since it has no parent
  init_process->_parent = NULL;
  // this will be the init process.
  // setting the address of the assembly file
  ready_processes_ptr = lookup_label(ready_processes_lname)->addr;
  blocked_processes_ptr = lookup_label(blocked_processes_lname)->addr;
  process_state_ptr = lookup_label(process_states_lname)->addr;
  handler_ptr = lookup_label(timer_interrupt_handler_name)->addr;
  bl_size_ptr = lookup_label(bl_size_lname)->addr;
  rl_size_ptr = lookup_label(rl_size_lname)->addr;
}

imm_expr * my_copy_imm_expr (imm_expr *old_expr)
{
  imm_expr *expr = (imm_expr *) xmalloc (sizeof (imm_expr));

  memcpy ((void*)expr, (void*)old_expr, sizeof (imm_expr));
  return (expr);
}

instruction * my_copy_inst (instruction *inst)
{
  instruction *new_inst = (instruction *) xmalloc (sizeof (instruction));
  if(inst)
    *new_inst = *inst;
  else
    return inst;
  /*memcpy ((void*)new_inst, (void*)inst , sizeof (instruction));*/
  if(EXPR(inst))
    SET_EXPR (new_inst, my_copy_imm_expr (EXPR (inst)));
  return (new_inst);
}

process::process(const char * process_name){
  
  _parent = processes[R[K1_REG]];
  _pid = next_pid++;
  _process_name = std::string(process_name);
  _pstate = READY;

  for (int i = 0; i < 26; i++)
    _R[i] = R[i];
  for (int i = 28; i < R_LENGTH; i++)
    _R[i] = R[i];
  _HI = HI;
  _LO = LO;
  _PC = PC;
  _nPC = nPC;

  _FPR = FPR;
  _FGR =  FGR; 
  _FWR = FWR; 

  for(int i = 0; i < 4 ; i++){
    for(int j = 0;j < 32; j++){
      _CCR[i][j] = CCR[i][j];
      _CPR[i][j] =CPR[i][j];
    }
  }
  int instruction_count = (text_top - TEXT_BOT )>> 2;
  _text_seg = (instruction **) xmalloc(sizeof(instruction*)*instruction_count);
  for (int i = 0; i < instruction_count; i++)
    _text_seg[i] = my_copy_inst(text_seg[i]);
  _text_top = text_top;
  
  _data_seg = (mem_word *) malloc (initial_data_size);
  memcpy(_data_seg,data_seg,initial_data_size);
  _data_seg_b = (BYTE_TYPE *) _data_seg;
  _data_seg_h = (short *) _data_seg;
  
  _data_top = data_top;
  
  _stack_seg = (mem_word *) malloc (initial_stack_size);
  memcpy(_stack_seg,stack_seg,initial_stack_size);
  _stack_seg_b = (BYTE_TYPE *) _stack_seg;
  _stack_seg_h = (short *) _stack_seg;
  _stack_bot  = stack_bot;

  _gp_midpoint = gp_midpoint;

  _text_modified = true;
  _data_modified = true;

}

void process::store_process(){
  for (int i = 0; i < 26; i++)
    _R[i] = R[i];
  for (int i = 28; i < R_LENGTH; i++)
    _R[i] = R[i];


  _HI = HI;
  _LO = LO;
  
  _PC = PC;
  _nPC = nPC;
  
  _FPR = FPR;
  _FGR = FGR; 
  _FWR = FWR; 


  _text_seg = text_seg;
  _text_top = text_top;
  _data_seg = data_seg;
  _data_seg_b = data_seg_b;
  _data_seg_h = data_seg_h;
  _data_top = data_top;
  _stack_seg = stack_seg;
  _stack_seg_b = stack_seg_b;
  _stack_seg_h = stack_seg_h;
  _stack_bot = stack_bot;
  _gp_midpoint = gp_midpoint;
  _text_modified = text_modified;
  _data_modified = data_modified;

}

void process::load_process(){
  for (int i = 0; i < 26; i++)
    R[i] = _R[i];
  for (int i = 28; i < R_LENGTH; i++)
    R[i] = _R[i];
  

  HI = _HI;
  LO = _LO;
  
  PC = _PC;
  nPC = _nPC;
  
  FPR = _FPR;
  FGR = _FGR; 
  FWR = _FWR; 

  text_seg = _text_seg;
  text_top = _text_top;
  data_seg = _data_seg;
  data_seg_b = _data_seg_b;
  data_seg_h = _data_seg_h;
  data_top = _data_top;
  stack_seg = _stack_seg;
  stack_seg_b = _stack_seg_b;
  stack_seg_h = _stack_seg_h;
  stack_bot = _stack_bot;
  gp_midpoint = _gp_midpoint;
  text_modified = _text_modified;
  data_modified = _data_modified;

}

// special syscall for the processes to print string this way they will be printing the pid too
void process_print_string(char * mes){
  if(mes == NULL){
    perror("Syscall input error");
    _exit(-1);
  }
  int cur_pid = R[K1_REG];
  int size = strlen(mes);
  // if there is newline at the end remove it because this function is going to print it
  if(mes[size-1] == '\n')
    mes[size-1] = 0;
  printf("Process[id = %d, name = %s]: %s\n",cur_pid,processes[cur_pid]->_process_name.data(),mes);
}

// special syscall for the processes to print int this way they will be printing the pid too
void process_print_int(int mes){
  int cur_pid = R[K1_REG];
  printf("Process[id = %d, name = %s]: %d\n",cur_pid,processes[cur_pid]->_process_name.data(),mes);
}

// Provides Atomic Printing For Palindrom File
void palindrome_print(int no, char * word, char * res){
  int cur_pid = R[K1_REG];
  if(word == NULL){
    perror("Syscall input error");
    _exit(-1);
  }
  int size = strlen(word);
  // if there is newline at the end remove it because this function is going to print it
  if(size > 1 && word[size-1] == '\n')
    word[size-1] = 0;
  printf("Process[id = %d, name = %s]: %d : %s : %s\n",cur_pid,processes[cur_pid]->_process_name.data(),
  no,word,res);
}