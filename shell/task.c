#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "shell.h"

void task_init(struct task *task, const struct task_interface *impl) {
	assert(impl->poll);
	task->impl = impl;
	task->status = TASK_STATUS_WAIT;
}

void task_destroy(struct task *task) {
	if (task == NULL) {
		return;
	}

	if (task->impl->destroy) {
		task->impl->destroy(task);
	} else {
		free(task);
	}
}

int task_poll(struct task *task, struct context *ctx) {
	if (task->status == TASK_STATUS_WAIT) {
		task->status = task->impl->poll(task, ctx);
	}
	return task->status;
}

int task_run(struct task *task, struct context *ctx) {
	while (true) {
		int ret = task_poll(task, ctx);
		if (ret != TASK_STATUS_WAIT) {
			ctx->state->last_status = ret;
			return ret;
		}

		int stat;
		pid_t pid = waitpid(0, &stat, 0);
		if (pid == -1) {
			if (errno == EINTR) {
				continue;
			}
			fprintf(stderr, "failed to waitpid(): %s\n", strerror(errno));
			return -1;
		}

		process_notify(pid, stat);
	}
}
