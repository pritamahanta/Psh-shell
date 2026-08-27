# Psh — A Unix Shell in C

A fully functional Unix shell built from scratch in C, implementing process
management, job control, pipelines, signal handling, and persistent command history.

## TL;DR — Why This Project Matters

- Built a POSIX-compliant Unix shell from scratch in C
- Full foreground & background job control (Ctrl-C, Ctrl-Z, `&`)
- Supports pipes, I/O redirection, background execution, and sequential commands
- Dynamic prompt showing `user@hostname:path$` with live `~` home directory substitution
- Uses process groups and terminal control (`setpgid`, `tcsetpgrp`)
- Signal-safe design using `sigaction` without `SA_RESTART`
- Environment variable expansion (`$VAR`) before execution
- Persistent command history across sessions (`~/.Psh_history`)
- Recursive descent parser validates syntax before any process is forked

---

## How to Build and Run

### Prerequisites

- GCC compiler
- Make
- Linux (Ubuntu recommended)

### Build

```bash
git clone https://github.com/perxeusss/Psh-shell.git
cd Psh-shell
make
```

### Benchmarks

Build the benchmark-enabled shell and run the execution or job-control benchmark:

```bash
make bench
make bench-exec
make bench-jobcontrol
```

Run the pipeline throughput benchmark with:

```bash
make bench-pipeline
```

Recorded validation results on CPU core 2, using five independent benchmark runs:

- External `true` execution: **536.82 us median**, **774.38 us p95** for 10,000 commands
- Foreground job-control bookkeeping: **7.28 us median**, **14.68 us p95** for 10,000 commands
- Pipeline throughput for a 1 GiB `/dev/zero` stream to `/dev/null`:
    - 2 processes: **2.778 GiB/s** median
    - 3 processes: **1.342 GiB/s** median
    - 5 processes: **0.704 GiB/s** median
    - 9 processes: **0.333 GiB/s** median

Results and validation details are in [`bench/results/`](bench/results/), including
the workload, repetition count, and aggregation method.

### Run

```bash
./psh
```

### Exit

Type `exit` or press `Ctrl-D`. The shell prints `logout` and exits cleanly,
killing all background jobs before exiting.

### Example

```bash
make && ./psh

# Dynamic prompt shows user, hostname, and current path
perxeuss@hostname:~$

# ~ substitution — home directory shown as ~
perxeuss@hostname:~$ cd /tmp
perxeuss@hostname:/tmp$ cd ~
perxeuss@hostname:~$

# Try a command
perxeuss@hostname:~$ echo Hello World
Hello World

# Run a pipeline
perxeuss@hostname:~$ ls -la | grep ".c" | wc -l
9

# Exit
perxeuss@hostname:~$ exit
```

---

## Dynamic Prompt

The prompt is generated live on every command using real system calls:

```
perxeuss@hostname:~$
perxeuss@hostname:~/projects$
perxeuss@hostname:/tmp/build$
```

**Format:** `user@hostname:path$`

- `user` — read from `$USER` environment variable
- `hostname` — fetched via `gethostname()`
- `path` — current working directory from `getcwd()`, with home directory replaced by `~`

The `~` substitution works by comparing the current working directory against the
shell's recorded home at startup. If the path starts with the home prefix, it is
replaced with `~` — matching real shell behavior.

---

## Commands Reference

### External Commands

Psh can run any binary available on your system `PATH`:

```bash
perxeuss@hostname:~$ ls -la
perxeuss@hostname:~$ gcc main.c -o main
perxeuss@hostname:~$ python3 script.py
```

**Error handling:** If a command doesn't exist, the shell prints `Command not found!`

---

### Built-in Commands

#### `cd` — Change Directory

**Syntax:** `cd [~ | - | path]`

**Arguments:**

- No argument or `~`: Change to home directory
- `-`: Go back to previous directory
- `path`: Change to specified relative or absolute path

**Examples:**

```bash
perxeuss@hostname:~$ cd /tmp
perxeuss@hostname:/tmp$ cd ~
perxeuss@hostname:~$ cd -
perxeuss@hostname:/tmp$ cd ../home
perxeuss@hostname:/home$
```

**Error:** Prints `cd: No such file or directory` if path doesn't exist.

---

#### `pwd` — Print Working Directory

**Syntax:** `pwd`

```bash
perxeuss@hostname:~/projects$ pwd
/home/perxeuss/projects
```

---

#### `echo` — Print Arguments

**Syntax:** `echo [-n] [args...]`

**Flags:**

- `-n`: Suppress trailing newline

```bash
perxeuss@hostname:~$ echo Hello World
Hello World

perxeuss@hostname:~$ echo -n no newline
no newline perxeuss@hostname:~$

perxeuss@hostname:~$ echo $HOME
/home/perxeuss
```

---

#### `env` — Print Environment

**Syntax:** `env`

```bash
perxeuss@hostname:~$ env
PATH=/usr/local/sbin:/usr/local/bin:...
HOME=/home/perxeuss
USER=perxeuss
...
```

---

#### `setenv` — Set Environment Variable

**Syntax:** `setenv VAR=value` or `setenv VAR value`

```bash
perxeuss@hostname:~$ setenv FOO=bar
perxeuss@hostname:~$ echo $FOO
bar
```

---

#### `unsetenv` — Unset Environment Variable

**Syntax:** `unsetenv VAR`

```bash
perxeuss@hostname:~$ unsetenv FOO
perxeuss@hostname:~$ echo $FOO
           # empty — variable removed
```

---

#### `which` — Locate a Command

**Syntax:** `which command`

```bash
perxeuss@hostname:~$ which gcc
/usr/bin/gcc

perxeuss@hostname:~$ which cd
cd: shell built-in command

perxeuss@hostname:~$ which fakecommand
fakecommand not found
```

---

#### `exit` — Exit the Shell

**Syntax:** `exit`

Kills all active background jobs and exits cleanly.

---

### Command Operators

#### Pipes (`|`)

**Syntax:** `command1 | command2 | ... | commandN`

Each command runs in its own process. `stdout` of one is wired to `stdin` of the
next. Ctrl-C and Ctrl-Z affect the **entire pipeline group**, not just the last process.

```bash
perxeuss@hostname:~$ cat /etc/passwd | grep root | wc -l
2

perxeuss@hostname:~$ ls -la | sort | head -5

perxeuss@hostname:~$ cat file.txt | grep "error" | sort | uniq > errors.txt
```

---

#### Input Redirection (`<`)

**Syntax:** `command < filename`

```bash
perxeuss@hostname:~$ sort < data.txt
perxeuss@hostname:~$ wc -l < file.txt
```

---

#### Output Redirection (`>` and `>>`)

**Syntax:** `command > filename` or `command >> filename`

```bash
perxeuss@hostname:~$ echo "Hello" > output.txt
perxeuss@hostname:~$ echo "World" >> output.txt
perxeuss@hostname:~$ ls -la > listing.txt
```

- `>` creates or overwrites the file
- `>>` appends to the file

---

#### Combined Redirection

```bash
perxeuss@hostname:~$ cat < input.txt > output.txt
perxeuss@hostname:~$ cat < input.txt | grep "pattern" > results.txt
perxeuss@hostname:~$ ls | sort > sorted_list.txt
```

---

#### Sequential Execution (`;`)

**Syntax:** `command1 ; command2 ; ... ; commandN`

```bash
perxeuss@hostname:~$ echo "First" ; echo "Second" ; echo "Third"
First
Second
Third

perxeuss@hostname:/tmp$ cd ~ ; ls ; pwd
```

---

#### Background Execution (`&`)

**Syntax:** `command &`

```bash
perxeuss@hostname:~$ sleep 10 &
perxeuss@hostname:~$            # prompt returns immediately

perxeuss@hostname:~$ make build &
```

Completion messages appear before the next prompt:

```
sleep with pid 12345 exited normally
gcc with pid 12346 exited abnormally
```

---

### Environment Variable Expansion

`$VAR` is expanded before execution across all commands, not just builtins:

```bash
perxeuss@hostname:~$ echo $HOME
/home/perxeuss

perxeuss@hostname:~$ ls $HOME

perxeuss@hostname:~$ cd $HOME/projects
perxeuss@hostname:~/projects$
```

---

### Command History

Psh saves every command to `~/.Psh_history` and loads it on startup.

**Features:**

- Stores up to 15 commands
- Persists across shell sessions
- No duplicate consecutive commands stored
- Commands containing `log` as a token are not stored

```bash
perxeuss@hostname:~$ echo hello
perxeuss@hostname:~$ ls -la
perxeuss@hostname:~$ pwd

# View saved history
perxeuss@hostname:~$ cat ~/.Psh_history
echo hello
ls -la
pwd
```

---

### Keyboard Shortcuts

#### Ctrl-C (SIGINT)

Interrupts and terminates the current foreground process or pipeline.

```bash
perxeuss@hostname:~$ sleep 100
^C
perxeuss@hostname:~$       # shell continues, sleep is killed
```

For pipelines, the **entire process group** is killed — not just one process.

---

#### Ctrl-Z (SIGTSTP)

Stops the current foreground process and moves it to the background job list.

```bash
perxeuss@hostname:~$ sleep 100
^Z
[1] Stopped sleep with pid 12345
perxeuss@hostname:~$       # shell continues, sleep is suspended
```

---

#### Ctrl-D (EOF)

Exits the shell.

```bash
perxeuss@hostname:~$ [Ctrl-D]
logout
```

---

## Internals

Detailed architecture and implementation notes — including execution flow, data
structures, the double-`setpgid` race condition fix, and signal handling design —
are available in [docs/internals.md](docs/internals.md).

---

## Project Structure

```
Psh-shell/
├── src/
│   ├── main.c          # Shell loop, initialization
│   ├── execute.c       # fork/exec, pipelines, redirection
│   ├── runner.c        # Command sequencing, builtin dispatch
│   ├── signals.c       # Signal handlers, fg process group tracking
│   ├── jobs.c          # Background job table management
│   ├── builtins.c      # cd, echo, env, which, setenv, etc.
│   ├── parser.c        # Syntax validation
│   ├── history.c       # Command history load/save
│   ├── prompt.c        # Dynamic prompt with ~ substitution
│   └── helpers.c       # Shared utilities
├── include/            # Header files
├── docs/
│   └── internals.md    # Architecture and implementation deep dive
├── Makefile
└── README.md
```

---

## Known Limitations

- No shell scripting (loops, conditionals, functions)
- No glob expansion (`*.c`, `file?.txt`)
- No `&&` exit-code-aware chaining (treated as `;`)
- No arrow key history navigation (yet)
- Designed for learning OS internals, not production use

---

## Compilation & Code Quality

- **Compiler:** GCC
- **Platform:** Linux (Ubuntu 22.04+)
- **System calls used:** `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `setpgid`, `tcsetpgrp`, `sigaction`, `kill`, `getcwd`, `gethostname`, `chdir`
- Modular design with clear separation of concerns
- Proper resource cleanup and job termination on exit