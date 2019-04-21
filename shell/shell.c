#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <mrsh/hashtable.h>
#include <mrsh/parser.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell/job.h"
#include "shell/shell.h"
#include "shell/process.h"
#include "shell/task.h"

void mrsh_function_destroy(struct mrsh_function *fn) {
	if (!fn) {
		return;
	}
	mrsh_command_destroy(fn->body);
	free(fn);
}

void mrsh_state_init(struct mrsh_state *state) {
	state->exit = -1;
	state->fd = -1;
	state->interactive = isatty(STDIN_FILENO);
	state->options = state->interactive ? MRSH_OPT_INTERACTIVE : 0;
	state->args = calloc(1, sizeof(struct mrsh_call_frame));
}

static const char *get_alias(const char *name, void *data) {
	struct mrsh_state *state = data;
	return mrsh_hashtable_get(&state->aliases, name);
}

void mrsh_state_set_parser_alias_func(
		struct mrsh_state *state, struct mrsh_parser *parser) {
	mrsh_parser_set_alias_func(parser, get_alias, state);
}

static void state_string_finish_iterator(const char *key, void *value,
		void *user_data) {
	free(value);
}

static void variable_destroy(struct mrsh_variable *var) {
	if (!var) {
		return;
	}
	free(var->value);
	free(var);
}

static void state_var_finish_iterator(const char *key, void *value,
		void *user_data) {
	variable_destroy((struct mrsh_variable *)value);
}

static void state_fn_finish_iterator(const char *key, void *value, void *_) {
	mrsh_function_destroy((struct mrsh_function *)value);
}

static void call_frame_destroy(struct mrsh_call_frame *args) {
	for (int i = 0; i < args->argc; ++i) {
		free(args->argv[i]);
	}
	free(args->argv);
	free(args);
}

void mrsh_state_finish(struct mrsh_state *state) {
	mrsh_hashtable_for_each(&state->variables, state_var_finish_iterator, NULL);
	mrsh_hashtable_finish(&state->variables);
	mrsh_hashtable_for_each(&state->functions, state_fn_finish_iterator, NULL);
	mrsh_hashtable_finish(&state->functions);
	mrsh_hashtable_for_each(&state->aliases,
		state_string_finish_iterator, NULL);
	mrsh_hashtable_finish(&state->aliases);
	while (state->jobs.len > 0) {
		job_destroy(state->jobs.data[state->jobs.len - 1]);
	}
	mrsh_array_finish(&state->jobs);
	while (state->processes.len > 0) {
		process_destroy(state->processes.data[state->processes.len - 1]);
	}
	mrsh_array_finish(&state->processes);
	struct mrsh_call_frame *args = state->args;
	while (args) {
		struct mrsh_call_frame *prev = args->prev;
		call_frame_destroy(args);
		args = prev;
	}
}

void mrsh_env_set(struct mrsh_state *state,
		const char *key, const char *value, uint32_t attribs) {
	struct mrsh_variable *var = calloc(1, sizeof(struct mrsh_variable));
	if (!var) {
		fprintf(stderr, "Failed to allocate shell variable");
		exit(1);
	}
	var->value = strdup(value);
	var->attribs = attribs;
	struct mrsh_variable *old = mrsh_hashtable_set(&state->variables, key, var);
	variable_destroy(old);
}

void mrsh_env_unset(struct mrsh_state *state, const char *key) {
	variable_destroy(mrsh_hashtable_del(&state->variables, key));
}

const char *mrsh_env_get(struct mrsh_state *state,
		const char *key, uint32_t *attribs) {
	struct mrsh_variable *var = mrsh_hashtable_get(&state->variables, key);
	if (var && attribs) {
		*attribs = var->attribs;
	}
	return var ? var->value : NULL;
}

void mrsh_push_args(struct mrsh_state *state, int argc, const char *argv[]) {
	struct mrsh_call_frame *next = calloc(1, sizeof(struct mrsh_call_frame));
	next->argc = argc;
	next->argv = malloc(sizeof(char *) * argc);
	for (int i = 0; i < argc; ++i) {
		next->argv[i] = strdup(argv[i]);
	}
	next->prev = state->args;
	state->args = next;
}

void mrsh_pop_args(struct mrsh_state *state) {
	struct mrsh_call_frame *args = state->args;
	assert(args->prev != NULL);
	state->args = args->prev;
	call_frame_destroy(args);
}

int mrsh_run_program(struct mrsh_state *state, struct mrsh_program *prog) {
	struct task *task = task_for_command_list_array(&prog->body);

	struct context ctx = {
		.state = state,
	};
	int ret = task_run(task, &ctx);
	task_destroy(task);
	return ret;
}

int mrsh_run_word(struct mrsh_state *state, struct mrsh_word **word) {
	struct task *task = task_word_create(word, TILDE_EXPANSION_NAME);

	int last_status = state->last_status;
	struct context ctx = {
		.state = state,
	};
	int ret = task_run(task, &ctx);
	task_destroy(task);
	state->last_status = last_status;
	return ret;
}

/**
 * Create a new process group. We need to do this in the parent and in the child
 * to protect aginst race conditions.
 */
static pid_t create_process_group(pid_t pid) {
	pid_t pgid = pid;
	if (setpgid(pid, pgid) != 0) {
		fprintf(stderr, "setpgid failed: %s\n", strerror(errno));
		return -1;
	}
	return pgid;
}

pid_t subshell_fork(struct context *ctx, struct process **process_ptr) {
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		return false;
	} else if (pid == 0) {
		if (process_ptr != NULL) {
			*process_ptr = NULL;
		}

		if (ctx->state->options & MRSH_OPT_MONITOR) {
			// Create a job for all children processes
			pid_t pgid = create_process_group(getpid());
			if (pgid < 0) {
				exit(1);
			}
			ctx->job = job_create(ctx->state, pgid);
		}

		return 0;
	}

	struct process *proc = process_create(ctx->state, pid);
	if (process_ptr != NULL) {
		*process_ptr = proc;
	}

	if (ctx->state->options & MRSH_OPT_MONITOR) {
		pid_t pgid = create_process_group(pid);
		if (pgid < 0) {
			return false;
		}

		// Create a background job
		struct mrsh_job *job = job_create(ctx->state, pgid);
		job_add_process(job, proc);
	}

	return pid;
}
