#ifndef _MRSH_SHELL_H
#define _MRSH_SHELL_H

#include <mrsh/ast.h>
#include <mrsh/hashtable.h>
#include <stdint.h>
#include <stdio.h>

enum mrsh_option {
	// -a: When this option is on, the export attribute shall be set for each
	// variable to which an assignment is performed.
	MRSH_OPT_ALLEXPORT = 1 << 0,
	// -b: Shall cause the shell to notify the user asynchronously of background
	// job completions.
	MRSH_OPT_NOTIFY = 1 << 1,
	// -C: Prevent existing files from being overwritten by the shell's '>'
	// redirection operator; the ">|" redirection operator shall override this
	// noclobber option for an individual file.
	MRSH_OPT_NOCLOBBER = 1 << 2,
	// -e: When this option is on, when any command fails (for any of the
	// reasons listed in Consequences of Shell Errors or by returning an exit
	// status greater than zero), the shell immediately shall exit
	MRSH_OPT_ERREXIT = 1 << 3,
	// -f: The shell shall disable pathname expansion.
	MRSH_OPT_NOGLOB = 1 << 4,
	// -h: Locate and remember utilities invoked by functions as those functions
	// are defined (the utilities are normally located when the function is
	// executed).
	MRSH_OPT_PRELOOKUP = 1 << 5,
	// -m: Immediately before the shell issues a prompt after completion of the
	// background job, a message reporting the exit status of the background job
	// shall be written to standard error. If a foreground job stops, the shell
	// shall write a message to standard error to that effect, formatted as
	// described by the jobs utility.
	MRSH_OPT_MONITOR = 1 << 6,
	// -n: The shell shall read commands but does not execute them; this can be
	// used to check for shell script syntax errors. An interactive shell may
	// ignore this option.
	MRSH_OPT_NOEXEC = 1 << 7,
	// -o ignoreeof: Prevent an interactive shell from exiting on end-of-file.
	MRSH_OPT_IGNOREEOF = 1 << 8,
	// -o nolog: Prevent the entry of function definitions into the command
	// history
	MRSH_OPT_NOLOG = 1 << 9,
	// -o vi: Allow shell command line editing using the built-in vi editor.
	MRSH_OPT_VI = 1 << 10,
	// -u: When the shell tries to expand an unset parameter other than the '@'
	// and '*' special parameters, it shall write a message to standard error
	// and the expansion shall fail.
	MRSH_OPT_NOUNSET = 1 << 11,
	// -v: The shell shall write its input to standard error as it is read.
	MRSH_OPT_VERBOSE = 1 << 12,
	// -x: The shell shall write to standard error a trace for each command
	// after it expands the command and before it executes it.
	MRSH_OPT_XTRACE = 1 << 13,
	// Defaults for an interactive session
	MRSH_OPT_INTERACTIVE = MRSH_OPT_MONITOR,
};

struct mrsh_state {
	int exit;
	uint32_t options; // enum mrsh_option
	FILE *input;
	char **argv;
	int argc;
	bool interactive;
	struct mrsh_hashtable variables; // char *
	struct mrsh_hashtable aliases; // char *
	int last_status;
};

void mrsh_state_init(struct mrsh_state *state);
void mrsh_state_finish(struct mrsh_state *state);
int mrsh_run_program(struct mrsh_state *state, struct mrsh_program *prog);
int mrsh_run_word(struct mrsh_state *state, struct mrsh_word **word);

#endif
