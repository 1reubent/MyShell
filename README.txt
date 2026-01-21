# MyShell

## Overview

**MyShell** is a Linux shell emulator.

To run it, `cd` into `/MyShell` and execute:

```bash
make clean
make
./mysh
````

### Not Supported

MyShell **does NOT** support:

* Tab completion
* Consecutive piping (only single piping is supported)
* Command history (arrow keys will not navigate previous commands)
* Directory prompt (current directory is not shown in the prompt)

### Supported

MyShell **DOES** support:

* Everything else 😄

---

## Implementation

We first set up argument arrays for both the current process and a possible piped process, each with respective counters.
We also maintain input and output variables to keep track of `stdin` and `stdout` for the current process.

---

## Parsing

The entire command is split into tokens using `strtok()`. Each token is processed as follows:

### 1. Conditionals

* Only for the first token.
* If the token is `"else"` or `"then"`, execution continues only if the previous status is correct.

### 2. Program Name Validity

* Again, only for the next token.
* We check whether it is a valid program name by searching appropriate locations using `name_builder()`.
* If valid, the proper pathname is built and parsing continues.

### 3. Pipes, Wildcards, Redirects, or Arguments

For every token thereafter, we check if it is one of the following:

* **Pipe (`|`)**

  * Stop parsing the current process and move to step 4.
* **Wildcard**

  * Run `expand_wildcard()` and add each expanded argument to the argument list.
* **Input or Output Redirect**

  * Open the next token as a file and update the input or output variable.
* **Argument**

  * If none of the above, the token is treated as an argument and added to the argument list.

### 4. If Piped

* Repeat steps 2 and 3 for the second process.

After parsing:

* The last argument in each argument list is set to `NULL`.
* If the command is piped, `execute_pipe_command()` is called.
* Otherwise, `execute_command()` is called.
* The resulting exit status is returned to the caller and passed back to the parser for the next command.

---

## Executing

### `execute_command`

The command can be either one of three built-in commands or an external command.

#### Built-in Commands

* **`cd`**

  * Calls `change_directory()`.

* **`pwd`**

  * If there is an output redirect:

    * Start a child process, set up `stdout`, and run `print_wd()`.
  * Otherwise:

    * Run `print_wd()` directly.

* **`which`**

  * Ensures there is exactly one argument.
  * If there is an output redirect:

    * Start a child process, set up `stdout`, and run `which_d()`.
  * Otherwise:

    * Run `which_d()` directly.

#### External Commands

* A child process is created.
* `stdin` and `stdout` are set up appropriately.
* `execv()` is executed.

After execution:

* Once the child process (if any) returns, the function returns `EXIT_SUCCESS` to the parsing function.

---

### `execute_pipe_command`

* A pipe is established.
* For each of the two processes:

  * If the program is `cd`, `change_directory()` is run.
  * Otherwise:

    * A child process is created.
    * Proper `stdin` and `stdout` are set up using the pipe.
    * If the program is `pwd` or `which`, their respective functions are called.
    * Otherwise, `execv()` is run.
* The parent process returns the status of the second process to the parsing function.

---

## Freeing

* Memory is freed at the end of `parse_and_execute()` and also within the execute functions.
* If a child process is created, dynamically allocated arguments are freed before exiting.
