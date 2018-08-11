#ifndef _BUILTIN_H
#define _BUILTIN_H

struct mrsh_state;

typedef int (*mrsh_builtin_func_t)(struct mrsh_state *state,
	int argc, char *argv[]);

int builtin_colon(struct mrsh_state *state, int argc, char *argv[]);
int builtin_exit(struct mrsh_state *state, int argc, char *argv[]);
int builtin_times(struct mrsh_state *state, int argc, char *argv[]);

int set(struct mrsh_state *state, int argc, char *argv[], bool cmdline);
int builtin_set(struct mrsh_state *state, int argc, char *argv[]);
const char *print_options(struct mrsh_state *state);

#endif
