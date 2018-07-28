#include <string.h>
#include <stdlib.h>
#include <mrsh/builtin.h>
#include "shell.h"

struct task_builtin {
	struct task task;
	struct mrsh_simple_command *sc;
};

static int task_builtin_poll(struct task *task, struct context *ctx) {
	struct task_builtin *tb = (struct task_builtin *)task;
	struct mrsh_simple_command *sc = tb->sc;

	int argc = 1 + sc->arguments.len;
	char *argv[argc + 1];
	argv[0] = sc->name;
	memcpy(argv + 1, sc->arguments.data, sc->arguments.len * sizeof(void *));
	argv[argc] = NULL;

	// TODO: redirections
	int ret = mrsh_run_builtin(ctx->state, argc, argv);
	if (ret < 0) {
		return TASK_STATUS_ERROR;
	}
	return ret;
}

static const struct task_interface task_builtin_impl = {
	.poll = task_builtin_poll,
};

struct task *task_builtin_create(struct mrsh_simple_command *sc) {
	if (!mrsh_has_builtin(sc->name)) {
		return NULL;
	}

	struct task_builtin *tb = calloc(1, sizeof(struct task_builtin));
	task_init(&tb->task, &task_builtin_impl);
	tb->sc = sc;
	return &tb->task;
}
