/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.

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
#include <vector>
#include<string>
/* Exported functions. */



struct process{
  process(const char * name);
  void load_process();
  void store_process();
  
  int _pid;
  std::string _process_name;
  int _pstate;
  std::vector<process*> childs;
  process * _parent;
  

  reg_word _R[R_LENGTH];
  reg_word _HI, _LO;
  int _HI_present, _LO_present;
  mem_addr _PC, _nPC;
  double *_FPR;			/* Dynamically allocate so overlay */
  float *_FGR;			/* is possible */
  int *_FWR;			/* is possible */
  reg_word _CCR[4][32], _CPR[4][32];

  instruction **_text_seg;
  bool _text_modified;		/* => text segment was written */
  mem_addr _text_top;
  mem_word *_data_seg;
  bool _data_modified;		/* => a data segment was written */
  short *_data_seg_h;		/* Points to same vector as DATA_SEG */
  BYTE_TYPE *_data_seg_b;		/* Ditto */
  mem_addr _data_top;
  mem_addr _gp_midpoint;		/* Middle of $gp area */
  mem_word *_stack_seg;
  short *_stack_seg_h;		/* Points to same vector as STACK_SEG */
  BYTE_TYPE *_stack_seg_b;		/* Ditto */
  mem_addr _stack_bot;
  instruction **_k_text_seg;
  mem_addr _k_text_top;
  mem_word *_k_data_seg;
  short *_k_data_seg_h;
  BYTE_TYPE *_k_data_seg_b;
  mem_addr _k_data_top;
    
};


void SPIM_timerHandler();
int do_syscall ();
void handle_exception ();
void empty_curr();
void spim_fork();
void spim_execv(char * path);
bool schedule(bool in_syscall);
void print_process(process * p);
void free_process(process * p);
void spim_wait();
void print_table();

#define PRINT_ON_SWITCH 0

#define PRINT_INT_SYSCALL	1
#define PRINT_FLOAT_SYSCALL	2
#define PRINT_DOUBLE_SYSCALL	3
#define PRINT_STRING_SYSCALL	4

#define READ_INT_SYSCALL	5
#define READ_FLOAT_SYSCALL	6
#define READ_DOUBLE_SYSCALL	7
#define READ_STRING_SYSCALL	8

#define SBRK_SYSCALL		9

#define EXIT_SYSCALL		10

#define PRINT_CHARACTER_SYSCALL	11
#define READ_CHARACTER_SYSCALL	12

#define OPEN_SYSCALL		13
#define READ_SYSCALL		14
#define WRITE_SYSCALL		15
#define CLOSE_SYSCALL		16

#define EXIT2_SYSCALL		17
#define FORK_SYSCALL     18
#define EXECV_SYSCALL     19
#define WAITPID_SYSCALL     20
#define RANDOM_INT_SYSCALL     21
#define PROCESS_EXIT_SYSCALL     22

#define READY 40
#define RUNNING 41
#define BLOCKED 42
#define TERMINATED 43

#define BYTES_TO_INST(N) (((N) + BYTES_PER_WORD - 1) / BYTES_PER_WORD * sizeof(instruction*))