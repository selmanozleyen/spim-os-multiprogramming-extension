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


int switched = 0;
int init = 1;
int next_pid = 0;
std::vector<process*> blocked_processes;
std::vector<process*> ready_processes;
process * cur_process;



imm_expr *
my_copy_imm_expr (imm_expr *old_expr)
{
  imm_expr *expr = (imm_expr *) xmalloc (sizeof (imm_expr));

  memcpy ((void*)expr, (void*)old_expr, sizeof (imm_expr));
  return (expr);
}

instruction *
my_copy_inst (instruction *inst)
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
  _parent = cur_process;
  _pid = next_pid++;
  _process_name = std::string(process_name);
  _pstate = READY;

  for (int i = 0; i < R_LENGTH; i++)
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

  _k_text_seg = (instruction **) malloc (BYTES_TO_INST(initial_k_text_size));
  memcpy(_k_text_seg,k_text_seg,initial_k_text_size);
  

  instruction_count = (k_text_top - K_TEXT_BOT)>> 2;
  _k_text_seg = (instruction **) xmalloc(sizeof(instruction*)*instruction_count);
  for (int i = 0; i < instruction_count; i++)
    _k_text_seg[i] = my_copy_inst(k_text_seg[i]);

  _k_text_top = k_text_top;

  
  _k_data_seg = (mem_word *) malloc (initial_k_data_size);
  memcpy(_k_data_seg,k_data_seg,initial_k_data_size);

  _k_data_seg_b = (BYTE_TYPE *) _k_data_seg;
  _k_data_seg_h = (short *) _k_data_seg;
  k_data_top = k_data_top;

  _gp_midpoint = gp_midpoint;

  _text_modified = true;
  _data_modified = true;

}
  


void process::store_process(){
  for (int i = 0; i < R_LENGTH; i++)
    _R[i] = R[i];
  _HI = HI;
  _LO = LO;
  
  _PC = PC;
  _nPC = nPC;
  
  _FPR = FPR;
  _FGR = FGR; 
  _FWR = FWR; 

  for(int i = 0; i < 4 ; i++){
    for(int j = 0;j < 32; j++){
      _CCR[i][j] = CCR[i][j];
      _CPR[i][j] = CPR[i][j];
    }
  }

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
  _k_text_seg = k_text_seg;
  _k_text_top = k_text_top;
  _k_data_seg = k_data_seg;
  _k_data_seg_b = k_data_seg_b;
  _k_data_seg_h = k_data_seg_h;
  _k_data_top = k_data_top;
  _gp_midpoint = gp_midpoint;
  _text_modified = text_modified;
  _data_modified = data_modified;

}

void process::load_process(){
  for (int i = 0; i < R_LENGTH; i++)
    R[i] = _R[i];
  HI = _HI;
  LO = _LO;
  
  PC = _PC;
  nPC = _nPC;
  
  FPR = _FPR;
  FGR = _FGR; 
  FWR = _FWR; 

  for(int i = 0; i < 4 ; i++){
    for(int j = 0;j < 32; j++){
      CCR[i][j] = _CCR[i][j];
      CPR[i][j] = _CPR[i][j];
    }
  }

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
  k_text_seg = _k_text_seg;
  k_text_top = _k_text_top;
  k_data_seg = _k_data_seg;
  k_data_seg_b = _k_data_seg_b;
  k_data_seg_h = _k_data_seg_h;
  k_data_top = _k_data_top;
  gp_midpoint = _gp_midpoint;
  text_modified = _text_modified;
  data_modified = _data_modified;

}

/*You implement your handler here*/
void SPIM_timerHandler()
{
   schedule(false);
}

/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */


int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif
  
  //if(cur_process == NULL)
  if(!schedule(true))
    return (1);
  cur_process->store_process();
  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  switch (R[REG_V0])
    {
    case PROCESS_EXIT_SYSCALL:
      spim_return_value = 0;
      cur_process->_pstate = TERMINATED;
      schedule(true);
      return (1);
    case RANDOM_INT_SYSCALL:
      srand(time(NULL));
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
      cur_process->_pstate = TERMINATED;
      schedule(true);
      return (1);

    case EXIT2_SYSCALL:
      cur_process->_pstate = TERMINATED;
      spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
      schedule(true);
      return (1);

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

void empty_curr(){
  int instruction_count = (text_top - TEXT_BOT )>> 2;
  for (int i = 0; i < instruction_count; i++){
    if(text_seg[i])
      free_inst(text_seg[i]);
  }
  instruction_count = (k_text_top - K_TEXT_BOT)>> 2;
  for (int i = 0; i < instruction_count; i++)
    if(k_text_seg[i])
      free_inst(k_text_seg[i]);
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
  //copied the process
  process * newp = new process(cur_process->_process_name.data());
  newp->_R[REG_V0] = 0;
  R[REG_V0] = newp->_pid;
  newp->_PC = PC + 4;
  
  newp->_parent = cur_process;
  ready_processes.push_back(newp);
  cur_process->childs.push_back(newp);

  cur_process->store_process();
  if(PRINT_ON_SWITCH)
   print_table();
}

void spim_execv(char * path){
  string temp = string(path);
  int last_index1,last_index2;
  last_index1  = temp.find_last_of('\\');
  last_index2 = temp.find_last_of('/');
  last_index1 = MAX(last_index1,last_index2);
  if (last_index1 < 0)
    last_index1 = 0;
  cur_process->_process_name = string(path+last_index1);
  empty_curr();  
  initialize_world (getenv ("SPIM_EXCEPTION_HANDLER"), true);
  char * p = strdup(temp.data());
  read_assembly_file(p);
  PC = find_symbol_address("main")-4;
  cur_process->store_process();

}

// Returns the child status -1 if there is none
void spim_wait(){
  if(cur_process->childs.empty()){
    R[REG_V0] = -1;
    cur_process->store_process();

  }
  else{
    blocked_processes.push_back(cur_process);
    cur_process->_pstate = BLOCKED;
    PC = PC + 4;
    cur_process->store_process();
    cur_process = NULL;
    schedule(true);
  }
}


void print_table(){
  // fprintf(stderr,"\n________________TABLE___________________\n");
  // cur_process->store_process();
  // print_process(cur_process);
  // for (size_t i = 0; i < blocked_processes.size(); i++)
  //   print_process(blocked_processes[i]);
  // for (size_t i = 0; i < ready_processes.size(); i++)
  //   print_process(ready_processes[i]);
  // fprintf(stderr,"________________________________________\n");
}

bool schedule(bool in_syscall){
  bool continuable = true;
  bool cs = false;
  switched = (switched + 1) % 2;
  if(cur_process == NULL && init){
    process * firstp = new process("init");
    firstp->_pstate = RUNNING;
    cur_process = firstp;
    cur_process->_parent = NULL;
    init = 0;
    continuable = true;
    cs = true;
  }
  //If it is a blocked it is going to be running one of its ready proccesses
  else if(cur_process == NULL && !blocked_processes.empty()){
    process * newp = ready_processes.front();
    ready_processes.erase(ready_processes.begin());   
    cur_process = newp;
    cur_process->load_process();
    cur_process->_pstate = RUNNING;
    if(in_syscall)
      PC = PC - 4;
    continuable = false;
    cs = true;
  }
  else if(ready_processes.size() != 0 && cur_process->_pstate == RUNNING && switched){
    cur_process->store_process();
    process * oldp = cur_process;
    process * newp = cur_process = ready_processes.front();
    ready_processes.erase(ready_processes.begin());    
    ready_processes.push_back(oldp);
    oldp->_pstate = READY;
    newp->_pstate = RUNNING;
    oldp->store_process();
    newp->load_process();
    cur_process = newp;
    if(in_syscall)
      PC = PC - 4;
    continuable = false;
    cs = true;
  }
  else if(cur_process->_pstate == TERMINATED){
    process * oldp = cur_process;
    process * parentp = NULL;
    
    cur_process->store_process();
    if(cur_process->_pid == 0){
      exit(0);
    }
    
    for (size_t i = 0; i < blocked_processes.size(); i++){
      if(blocked_processes[i] == oldp->_parent){
        parentp = blocked_processes[i];
        blocked_processes.erase(blocked_processes.begin()+i);
        break;
      }
    }
    if(oldp->_parent){
      int old_index = -1;
      for (size_t i = 0; i < oldp->_parent->childs.size(); i++)
      {
        if(oldp->_parent->childs[i] == oldp){
          old_index =i;
          break;
        }
      }
      if(old_index != -1)
        oldp->_parent->childs.erase(oldp->_parent->childs.begin() + old_index);
      
      for (size_t i = 0; i < oldp->childs.size(); i++){
        oldp->childs[i]->_parent = oldp->_parent;
        oldp->_parent->childs.push_back(oldp->childs[i]);
      }
      
    }
    //If it has a waiting parent
    //Context switch to parentp
    if(parentp){
      cur_process = parentp;
      
      cur_process->load_process();
      R[REG_V0] = oldp->_pid;

      if(in_syscall)
        PC = PC - 4;
      continuable = false;
      cs = true;
    }
    //If there are proccesses remaining
    else if(ready_processes.size() != 0){
      cur_process = ready_processes.front();
      ready_processes.erase(ready_processes.begin());
      
      cur_process->load_process();
      if(in_syscall)
        PC = PC - 4;
      continuable = false;
      cs = true;
    }
    else{
      //NO remaining process
      exit(0);
    }

    
    cur_process->store_process();
    free_process(oldp);
    cur_process->_pstate = RUNNING;
  }
  

  if(cs && PRINT_ON_SWITCH)
      print_table();
  return continuable;
}

void free_process(process * p){
  int instruction_count = (p->_text_top - TEXT_BOT )>> 2;
  for (int i = 0; i < instruction_count; i++){
    if(p->_text_seg[i])
      free_inst(p->_text_seg[i]);
  }
  instruction_count = (p->_k_text_top - K_TEXT_BOT)>> 2;
  for (int i = 0; i < instruction_count; i++){
    if(p->_k_text_seg[i])
      free_inst(p->_k_text_seg[i]);
  }
  free(p->_data_seg);
  free(p->_text_seg);
  free(p->_k_data_seg);
  free(p->_stack_seg);
  free(p);
}


void print_process(process * p){
  
  fprintf(stderr,"\n");
  fprintf(stderr,"Pid:%d\nProcess Name:%s\nPC:%d\nStack Pointer Adrress:%p\n",
  p->_pid,p->_process_name.data(),p->_PC,(void *)p->_stack_seg);
  if(p->_pstate == RUNNING)
    fprintf(stderr,"State: Running\n");
  if(p->_pstate == READY)
    fprintf(stderr,"State: Ready\n");
  if(p->_pstate == BLOCKED)
    fprintf(stderr,"State: Waiting\n");
  if(p->_pstate == TERMINATED)
    fprintf(stderr,"State: Terminated\n");
    
}
