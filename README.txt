Demostrated working knowledge and functionality of Operating System components like linker, scheduler, virtual memory manager and IO manager. Implemented 5 scheduling, 6 paging and 5 IO handling algorithms. (like FIFO, RoubndRobin, Shortest Job, Clock, Aging, CLOOK, FLOOK, etc.)

The entire project has been coded in C++. The goal of this project is to demonstrate working knowledge of different components of an Operating System. This project is divided into 4 parts:

Part 1 - Linker:
    In real life, a linker takes individually compiled code or object modules and creates a single executable file, resolving symbol references across the various modules produced. It performs the task of assigning global addresses to all the symbols used across all modules.  
  In my program linker.cpp, I have implemented a 2-pass linker. In the first pass, it verifies the correct syntax of the input, calculates the base address for each module and prints the symbol table (which is a list of all symbols/variables used across the given n modules along with their global address after resolving them). In the second pass, it parses the input again. Using the base addresses and the symbol table entries created in pass one, the actual output is generated by relocating relative addresses and resolving external references.
  
Part 2 - Scheduler:
    The scheduler in real life acts like a process manager that handles the removal of the running process from the CPU and the selection of another process on the basis of a particular strategy. 
    In my program sched.cpp, I explore the implementation and effects of a few scheduling policies on a set of processes/threads executing on a system. I have implemented the system using Discrete Event Simulation (DES), i.e. the system is represented as a chronological sequence of events. Each event occurs at an instant in time and marks a change of state in the system.
    
Part 3 - Virtual Memory Management:
  Virtual Memory is a storage allocation scheme in which secondary memory can be addressed as though it were part of the main memory.
  In mmu.cpp, I simulated the operation of an Operating System???s Virtual Memory Manager and mapped the virtual address spaces of multiple processes onto physical frames using page table translation. I have assumed multiple processes, each with its own virtual address space of 64 virtual pages. I have implemented paging as the sum of all virtual pages in all virtual address spaces may exceed the number of physical frames of my simulated system.
  
  
Run with the following gcc: gcc-11.2
