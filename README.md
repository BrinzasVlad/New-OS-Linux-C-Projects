# OS Linux C Projects
This repository contains solutions for the laboratory assignments for the Operating Systems course of 2023.
This parallels my [older repository](https://github.com/BrinzasVlad/OS-Linux-C-Projects) on the topic, which featured my solutions to the tasks as received and written in 2016.

Unlike that older repository, this one contains the complete task requirements, courtesy of a good sir (whose name I have blanked for internet anonymity reasons) whom I met at an event. I thank him for allowing me a second run at these problems in a more complete and modern format and would like to highlight that it was my initiative to request these task requirements from him and that any issues or errors are undoubtedly my doing and not his.

Finally, unlike the older repository, this one contains the Python tester programs given by the course, as well as Dockerfiles to compile the solutions and test them using the tester, so that my claim of receiving 100/100 points on all submissions can actually be verified.

## Assignment 1 - Files and Directories (File System Module)
An assignment centred around traversing the file system, listing directory contents, and parsing files.
A special type of file called an SF file is defined, and the student must scan its specific structure and efficiently extract data from it.

## Assignment 2 - Threads and Processes (Processes, Threads and Synchronization Module)
An assignment centred around coordinating processes and threads so they execute in specific orders.
Implemented with `fork()`, pthread, and semaphores.

## Assignment 3 - Pipes and Shared Memory (Inter-Process Communication)
An assignment centred around communication between processes using named pipes and shared memory.
Unlike previous assignments, this assignment features a back-and-forth communication between program and tester, such that the tester can request a sequence of consecutive instructions.