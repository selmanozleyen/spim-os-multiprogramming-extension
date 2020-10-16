# Introduction
This is a modification of SPIM. Original source code can be found [here](https://sourceforge.net/projects/spimsimulator/). Original spim is designed to execute only one process but by adding syscalls like execv, fork, wait and some other helper syscalls I was able to modify it to a multi-process OS with multitasking. There are also interrupts periodically to call the *interrupt function* which is written in assebly. Interrupt handler makes context switches with *round robin scheduling*.

# Running
To install spim enter the following;
```
$cd spim
$sudo make clean
$sudo make install
```
After that enter something like

```
$./spim -file SPIMOS_GTU_1.s
```
where the file given could be SPIMOS_GTU_1.s, SPIMOS_GTU_2.s or SPIMOS_GTU_3.s.

# Different init processes
SPIMOS_GTU_1.s, SPIMOS_GTU_2.s and SPIMOS_GTU_3.s indicates different init processes of the kernel (the process that runs initially on the system).
Here is how they behave;  

1.  In the SPIMOS_GTU_1.s init process will initialize Process Table, load 4 different programs (listed below) to the memory start
them and will enter an infinite loop until all the processes terminate.
2.  SPIMOS_GTU_2.s is randomly choosing one of the programs and loads it into memory 5 times (Same program 5 different
processes), start them and will enter an infinite loop until all the processes terminate.
3.  SPIMOS_GTU_2.s is choosing 3 out 4 programs randomly and loading each program 3 times start them and will enter an
infinite loop until all the processes terminate.

# Test programs
-BinarySearch.s   

-LinearSearch.s   

-Collatz.s: You are going to find collatz sequence for each number less than 25. You should do this starting from 25 to 1
iteratively (Not only for 1 number). You can find information about (Collatz conjecture on internet). For each number
you will show the number being interested in, and its collatz sequence and go to next number.   

-Palindrome.s: Create a dictionary that contains 100 words, where 90 of the words are not palindrome, 10 of them are
palindrome. You do not have to create the dictionary by taking from the user. Then, you will print out each word and
whether they are palindrome or not respectively. When you assign all the words in the dictionary whether palindrome or
not, then you ask for the user continue or not, if yes, take an input word and show whether a string given from keyboard
is a palindrome by printing the “string” semicolon: “Palindrome” or “Not Palindrome”. Otherwise terminate the program.

# Output Example
![out](https://github.com/SelmanOzleyen/spim-multiprogramming-mod/blob/master/media/1.PNG)
