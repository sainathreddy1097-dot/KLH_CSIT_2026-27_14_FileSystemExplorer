# Linux File System Explorer Using System Calls

## Section / Team Details

* **Section No.:** 12
* **Team No.:** 14
* **Project Title:** Linux File System Explorer Using System Calls

## Team Members

| Roll Number | Student Name     | Signature |
| ----------- | ---------------- | --------- |
| 2520090135  | M. Shiva         |           |
| 2520090115  | N. Sainath Reddy |           |

## Current Phase

**Phase 1 — Project Setup and Planning**

## 1. Abstract

Linux File System Explorer Using System Calls is a Linux-based system programming project that provides a command-line interface for exploring and performing basic file-system operations using low-level Linux/POSIX system calls. The project addresses the need to understand how user applications interact with the Linux kernel instead of relying only on graphical file managers.

The system will demonstrate important Operating Systems concepts such as file descriptors, file and directory management, file permissions, metadata, and kernel-level resource access. It will use C programming on Ubuntu/Linux with APIs and system calls such as `open()`, `read()`, `write()`, `close()`, `lseek()`, `stat()`, `mkdir()`, `opendir()`, and `readdir()`. Users will be able to navigate directories, display file information, create files/directories, read and write files, and perform basic file-management operations. The expected outcome is a working command-line file-system explorer that strengthens practical understanding of Linux system calls, file handling, directory structures, and Operating Systems concepts.

## 2. Problem Statement

Many file-management applications hide the low-level operations performed by the operating system, making it difficult for students to understand how files, directories, metadata, permissions, and file descriptors are handled by Linux. This project addresses the problem by developing a simple command-line File System Explorer that directly uses Linux/POSIX system calls.

It will allow users to perform common file and directory operations while demonstrating how a user program communicates with the Linux kernel. The project is relevant to Operating Systems and Systems Programming because it provides practical implementation of file-system interfaces, system calls, file descriptors, directory traversal, and resource management.

## 3. Objectives

1. Implement a Linux command-line file-system explorer using C and POSIX/Linux system calls.
2. Provide basic operations such as directory navigation, file creation, reading, writing, and listing.
3. Display file metadata such as size, type, permissions, and timestamps using appropriate APIs.
4. Demonstrate practical concepts of file descriptors, system calls, directory handling, and kernel interaction.

## 4. Proposed Methodology

The project will be implemented in C on Ubuntu/Linux as a menu-driven command-line application. The program will accept user commands and map them to suitable Linux/POSIX APIs. Directory-related operations will use directory APIs such as `opendir()` and `readdir()`, while file operations will use system calls such as `open()`, `read()`, `write()`, `close()`, and `lseek()`. File information will be obtained using `stat()` and related metadata structures.

The application flow will be: start the program → display the current directory/menu → accept a user operation → validate the input/path → invoke the required Linux API or system call → display the result or error message → return to the menu until the user exits. Error handling will be included using return values and appropriate error-reporting mechanisms. The program will be compiled using GCC and tested through the Linux terminal.

## 5. Operating Systems Concepts / Linux APIs Used

| OS Concept / Linux API / System Call | Purpose in the Project                                                  |
| ------------------------------------ | ----------------------------------------------------------------------- |
| `open(), close()`                    | Open a file and release its file descriptor.                            |
| `read(), write()`                    | Read data from and write data to files.                                 |
| `lseek()`                            | Move the file offset for random file access.                            |
| `stat()`                             | Retrieve file metadata such as size, type, permissions, and timestamps. |
| `opendir(), readdir()`               | Open a directory and traverse its entries.                              |
| `mkdir(), rmdir()`                   | Create and remove directories.                                          |

## 6. Individual Contribution

| Roll Number | Student Name     | Individual Responsibility                                                                                                                                                                                    |
| ----------- | ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 2520090135  | M. Shiva         | Design the command-line interface and overall program flow. Implement file metadata, permissions, error handling, and testing. Prepare documentation, GitHub repository, test cases, and final presentation. |
| 2520090115  | N. Sainath Reddy | Implement file operations using `open()`, `read()`, `write()`, `close()`, and `lseek()`. Implement directory traversal, creation, and removal using directory/POSIX APIs.                                    |

## 7. Tools / Platforms / Software Used

| Tool / Platform / Software | Purpose                                                      |
| -------------------------- | ------------------------------------------------------------ |
| Linux / Ubuntu             | Development and execution environment.                       |
| C                          | Primary programming language for system-call implementation. |
| GCC                        | Compile and build the C source code.                         |
| VS Code                    | Source-code editing and project development.                 |
| Linux Terminal             | Run, test, and demonstrate the application.                  |

## 8. Expected Outcome

A working Linux-based File System Explorer that provides command-line operations for listing directory contents, navigating directories, creating files and directories, reading and writing file contents, inspecting file metadata, and performing selected basic file-management operations.

The project will demonstrate practical understanding of Linux system calls, POSIX APIs, file descriptors, directory structures, permissions, file metadata, and interaction between C programs and Linux kernel services.
