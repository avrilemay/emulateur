# Imperative Programming Project - Emulator in C

This project involves the creation and testing of a simplified computer emulator written in C, with integrated debugging features. The project was developed as part of the coursework for a Bachelor’s degree in Computer Science.

## Overview

The emulator reads a program file, simulates its execution, and provides various debugging options such as step-by-step execution, memory tracking, and output logging. 

## Prerequisites

- **Operating System:** Linux (Ubuntu recommended)
- **Compiler:** GCC with support for GNU99 standard (`gcc -Wall -std=gnu99`)
- **Text Editor/IDE:** Any editor capable of handling C files
- **Terminal:** For executing the program and running makefiles

## Installation

# Compile the Program

Use the provided Makefile to compile the program:

```bash
make all
```
This will generate the executable cx25.1 in the project directory.

# Usage

The program is executed with the following syntax:

```bash
./cx25.1 [program_file].txt 0x[start_address] [options]
```

# Available Options

-`stepper`: Pauses the execution after each instruction, waiting for user input to continue.

-`ram:` Displays the initialization of the RAM.

-`journal`: Logs detailed information about each operation cycle to a file.

-`print`: Verbose mode that prints detailed operation cycles to the terminal.

-`breakpoint`: Sets a breakpoint at a specified memory address, pausing execution when the address is reached.


# Example Commands

To run the program with the stepper and RAM tracking enabled:

```bash
./cx25.1 prog_10_5.txt 0x30 -stepper -ram
```

To run the program with journal logging and breakpoints:

```bash
./cx25.1 prog_10_6.txt 0x36 -journal -print -breakpoint
```
