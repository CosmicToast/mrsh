#include <string.h>
#include <stdlib.h>
#include "shell/task.h"

struct task_assignment {
	struct task task;
	struct mrsh_array *assignments;
};

static int task_assignment_poll(struct task *task, struct context *ctx) {
	struct task_assignment *ta = (struct task_assignment *)task;

	for (size_t i = 0; i < ta->assignments->len; ++i) {
		struct mrsh_assignment *assign = ta->assignments->data[i];
		char *new_value = mrsh_word_str(assign->value);
		// TODO: MRSH_OPT_ALLEXPORT
		mrsh_env_set(ctx->state, assign->name, new_value, MRSH_VAR_ATTRIB_NONE);
	}

	return 0;
}

static const struct task_interface task_assignment_impl = {
	.poll = task_assignment_poll,
};

struct task *task_assignment_create(struct mrsh_array *assignments) {
	struct task_assignment *ta = calloc(1, sizeof(struct task_assignment));
	task_init(&ta->task, &task_assignment_impl);
	ta->assignments = assignments;
	return &ta->task;
}
