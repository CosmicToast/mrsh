#include <assert.h>
#include <mrsh/array.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "shell/process.h"

struct process *process_create(struct mrsh_state *state, pid_t pid) {
	struct process *proc = calloc(1, sizeof(struct process));
	proc->pid = pid;
	proc->state = state;
	mrsh_array_add(&state->processes, proc);
	return proc;
}

static void array_remove(struct mrsh_array *array, size_t i) {
	memmove(&array->data[i], &array->data[i + 1],
		(array->len - i - 1) * sizeof(void *));
	--array->len;
}

void process_destroy(struct process *proc) {
	struct mrsh_state *state = proc->state;
	for (size_t i = 0; i < state->processes.len; ++i) {
		if (state->processes.data[i] == proc) {
			array_remove(&state->processes, i);
			break;
		}
	}

	free(proc);
}

int process_poll(struct process *proc) {
	if (!proc->finished) {
		return -1;
	}
	return WEXITSTATUS(proc->stat);
}

void process_notify(struct mrsh_state *state, pid_t pid, int stat) {
	struct process *proc = NULL;
	bool found = false;
	for (size_t i = 0; i < state->processes.len; ++i) {
		proc = state->processes.data[i];
		if (proc->pid == pid) {
			found = true;
			break;
		}
	}
	if (!found) {
		return;
	}

	if (WIFEXITED(stat) || WIFSIGNALED(stat)) {
		proc->finished = true;
		proc->stat = stat;
	} else {
		assert(false);
	}
}
