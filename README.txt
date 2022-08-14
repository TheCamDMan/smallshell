# Small Shell (smallsh)

## by Cameron Blankenship (@TheCamDMan)

### San Diego, CA

---

## Description

The program is written in C, and is an emulation of a shell that handles a subset of commands commonly found in well-known shells such as bash.

Some commands are built into the shell and coded by me such as "exit", "cd", and "status". Other commands are implemented
by using the exec() family of functions.

---

### How to Use

Use this program like you would any other shell program. Type in the command, and if necessary, any command line arguments needed.
To exit program, type "exit" in the terminal. Kill signals such as "ctrl+C" will not work to exit the program.

Example 1:  ls
This command will print the directories and files that reside in the current working directory.

Example 2:  touch hello.txt
This command will create a new file called "hello.txt" in the current working directory.

---

## Installation

1.  If not already on your system, download compiler with this command in your terminal: "sudo apt install gcc"

This command works on Linux systems, or if on Windows, use WSL terminal.

2.  In terminal, compile code with this command: gcc -std=gnu99 -o smallsh smallsh.c

This will compile the code in file "smallsh.c" and create the runnable code as "smallsh"

3.  Run program from command line by entering "./smallsh" into the terminal

This will run the program. The terminal will wait for user entry.

---

## Background

This program was assigned to me during Operating Systems as a portfolio project during my time at Oregon State University while pursuing my Bachelor's of Science in Computer Science.

Throughout the class we primarily coded in C in order to understand and demonstrate how to use a lower-level language to manipulate the OS.
Some highlights of the class were learning how to communicate with the OS using system calls, create and manage multiple processes at once, create multiple threads, get processes and threads to synchronize their actions, and how processes can communicate with each other, when they are on the same machine, as well as over the network.

---

## Version

### v.1

This is the first public distribution of this program.

---

## Framework Used

The program was written completely in C using VIM.

---

## Credits

Portfolio Project for Oregon State University.

