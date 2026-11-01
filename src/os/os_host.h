/* Made by rio_hostHeader (rio -H) from the Rio module that declares these host procs: don't edit.
   A struct parameter comes as 2 words: rio_getStruct(vm, a + i, &s, sizeof s) copies it into s.
   Register the module's source with its procs (_SOURCE, below: rio_addSource), and each proc with
   its _SIG (_HOST_PROCS lists them all). */
#include <stdint.h>
#include "rio.h"

typedef struct {
  rio_View out;
  int32_t errorsToo;
  int32_t shell;
} os_RunOptions;
typedef char os_RunOptions_size[sizeof(os_RunOptions) == 16 ? 1 : -1]; /* (Rio's size) */
#define os_RunOptions_LAYOUT "{out:Blob,errorsToo:Int,shell:Int}"

#define os_argCount_SIG ">i"
#define os_arg_SIG "ib>i"
#define os_env_SIG "sb>i"
#define os_envSet_SIG "ss>i"
#define os_exit_SIG "i"
#define os_write_SIG "s"
#define os_writeErr_SIG "s"
#define os_readLine_SIG "sb>i"
#define os_readAll_SIG "b>i"
#define os_fileRead_SIG "sb>i"
#define os_fileWrite_SIG "ss>i"
#define os_fileAppend_SIG "ss>i"
#define os_fileSize_SIG "s>i"
#define os_fileKind_SIG "s>i"
#define os_fileDelete_SIG "s>i"
#define os_fileRename_SIG "ss>i"
#define os_dir_SIG "sb>i"
#define os_dirMake_SIG "s>i"
#define os_cwd_SIG "b>i"
#define os_cwdSet_SIG "s>i"
#define os_run_SIG "[s]" os_RunOptions_LAYOUT ">ii"
#define os_clock_SIG ">f"
#define os_time_SIG ">f"
#define os_date_SIG "b>i"
#define os_sleep_SIG "f"

#define os_HOST_PROCS(X) \
  X(argCount, os_argCount_SIG) \
  X(arg, os_arg_SIG) \
  X(env, os_env_SIG) \
  X(envSet, os_envSet_SIG) \
  X(exit, os_exit_SIG) \
  X(write, os_write_SIG) \
  X(writeErr, os_writeErr_SIG) \
  X(readLine, os_readLine_SIG) \
  X(readAll, os_readAll_SIG) \
  X(fileRead, os_fileRead_SIG) \
  X(fileWrite, os_fileWrite_SIG) \
  X(fileAppend, os_fileAppend_SIG) \
  X(fileSize, os_fileSize_SIG) \
  X(fileKind, os_fileKind_SIG) \
  X(fileDelete, os_fileDelete_SIG) \
  X(fileRename, os_fileRename_SIG) \
  X(dir, os_dir_SIG) \
  X(dirMake, os_dirMake_SIG) \
  X(cwd, os_cwd_SIG) \
  X(cwdSet, os_cwdSet_SIG) \
  X(run, os_run_SIG) \
  X(clock, os_clock_SIG) \
  X(time, os_time_SIG) \
  X(date, os_date_SIG) \
  X(sleep, os_sleep_SIG)

#define os_HASH 0x0c1296ebu /* (rio_hashText of the source) */
#define os_SOURCE \
  "# os.rio: module os, the rio command's: the program's arguments and environment, text in and out, files\n" \
  "# and folders, running other programs, and time. A program imports it (import os), and it's in rio as\n" \
  "# it runs a program, in its REPL, and in a program's C (rio -c); not in riocore, Rio as a library, whose\n" \
  "# hosts have modules of their own (Rinc's pc). os.h has the procs, in C, and os_host.h, made from this\n" \
  "# (rio -H src/os/os_host.h src/os/os.rio), their sigs. All of module os is here, in src/os: Rio itself,\n" \
  "# riocore, is the rest of src, and has nothing of it.\n" \
  "#\n" \
  "# As in rinc's pc: what a proc reads goes into a Blob it's given, as much as fits, and it says the whole\n" \
  "# length (or -1: there's none); what a proc does says 0, or -1 when it can't. Paths are as the system\n" \
  "# takes them (on Windows, / works as \\ does).\n" \
  "\n" \
  "# ---------------------------------------------------------------- the program\n" \
  "\n" \
  "argCount* :: host proc() -> Int                   # the arguments after the program's file: rio game.rio a b, 2\n" \
  "arg* :: host proc(i: Int, out: Blob) -> Int       # argument i (from 0): its length, or -1\n" \
  "env* :: host proc(name: String, out: Blob) -> Int # an environment variable: its length, or -1 (not set)\n" \
  "envSet* :: host proc(name: String, value: String) -> Int # set for the program and the ones it runs: 0, or -1\n" \
  "exit* :: host proc(code: Int)                     # the program ends here, with this exit code\n" \
  "\n" \
  "# ---------------------------------------------------------------- text in and out\n" \
  "\n" \
  "write* :: host proc(text: String)       # to standard output, as it is (log adds a line break, and spaces)\n" \
  "writeErr* :: host proc(text: String)    # to standard error\n" \
  "# a line typed, after prompt, edited as it's typed (when it's typed at a terminal: arrows, Home, End,\n" \
  "# Ctrl+Left and Right by words, Backspace, Delete, Ctrl+K, Ctrl+U, Ctrl+W, Ctrl+Y, Up and Down for the\n" \
  "# lines typed before): its length (without its line break), or -1 at the end of the input\n" \
  "readLine* :: host proc(prompt: String, out: Blob) -> Int\n" \
  "readAll* :: host proc(out: Blob) -> Int # standard input, to its end: as much as fits, and how long it was\n" \
  "\n" \
  "# ---------------------------------------------------------------- files and folders\n" \
  "\n" \
  "fileRead* :: host proc(path: String, out: Blob) -> Int      # as much as fits: its whole length, or -1\n" \
  "fileWrite* :: host proc(path: String, data: String) -> Int  # made, or replaced: 0, or -1\n" \
  "fileAppend* :: host proc(path: String, data: String) -> Int # added at its end (made if it isn't there): 0, or -1\n" \
  "fileSize* :: host proc(path: String) -> Int                 # its length, or -1 (not a file)\n" \
  "fileKind* :: host proc(path: String) -> Int                 # 0 nothing there, 1 a file, 2 a folder\n" \
  "fileDelete* :: host proc(path: String) -> Int               # a file, or an empty folder: 0, or -1\n" \
  "fileRename* :: host proc(from: String, to: String) -> Int    # (moved, too): 0, or -1\n" \
  "dir* :: host proc(path: String, out: Blob) -> Int           # its entries, a line each (\"d name\", \"f name\"), folders first: how many, or -1\n" \
  "dirMake* :: host proc(path: String) -> Int                  # a folder (the folder it's in, there already): 0, or -1\n" \
  "cwd* :: host proc(out: Blob) -> Int                         # the folder relative paths start from: its length\n" \
  "cwdSet* :: host proc(path: String) -> Int                   # another one: 0, or -1\n" \
  "\n" \
  "# ---------------------------------------------------------------- other programs\n" \
  "\n" \
  "# how os.run runs a program; left out, as all zeros: its output goes on to the program's own\n" \
  "RunOptions* :: struct\n" \
  "  out: Blob        # what it writes to standard output, kept here (as much as fits)\n" \
  "  errorsToo: Bool  # and what it writes to standard error, in out too\n" \
  "  shell: Bool      # args[0] a command line, for the system's shell (sh, or cmd on Windows): its pipes,\n" \
  "                   # redirects and wildcards, and its quoting, the command's to get right\n" \
  "end\n" \
  "# a program, run with these arguments, just as they are (no shell between: nothing to quote; it's found\n" \
  "# as the system finds commands, by its path, else on the PATH), and waited for: {its exit code (-1 if\n" \
  "# it couldn't run), how long what it wrote to out was}. os.run({\"git\", \"status\"})[0]\n" \
  "run* :: host proc(args: []String, options?: RunOptions) -> [2]Int\n" \
  "# ---------------------------------------------------------------- time\n" \
  "\n" \
  "clock* :: host proc() -> Float           # the processor's time the program has used, in seconds\n" \
  "time* :: host proc() -> Float            # seconds since the program started, as a clock on the wall counts\n" \
  "date* :: host proc(out: Blob) -> Int     # the date and time here, 2026-11-01 14:03:05: its length (19)\n" \
  "sleep* :: host proc(seconds: Float)      # nothing, for this long\n"
