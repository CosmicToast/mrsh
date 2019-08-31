#include <assert.h>
#include <mrsh/getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "builtin.h"
#include "shell/job.h"
#include "shell/shell.h"
#include "shell/task.h"

// TODO: [-l|-p] [job_id...]
static const char jobs_usage[] = "usage: jobs\n";

static char *job_state_str(struct mrsh_job *job) {
	int status = job_poll(job);
	// TODO code in parentheses for Done and Stopped
	switch (status) {
	case TASK_STATUS_WAIT:
		return "Running";
	case TASK_STATUS_ERROR:
		return "Error";
	case TASK_STATUS_STOPPED:
		return "Stopped";
	default:
		assert(status >= 0);
		return "Done";
	}
}

int builtin_jobs(struct mrsh_state *state, int argc, char *argv[]) {
	mrsh_optind = 0;
	int opt;
	while ((opt = mrsh_getopt(argc, argv, ":")) != -1) {
		switch (opt) {
		default:
			fprintf(stderr, "jobs: unknown option -- %c\n", mrsh_optopt);
			fprintf(stderr, jobs_usage);
			return EXIT_FAILURE;
		}
	}

	struct mrsh_job *current = job_by_id(state, "%+", false);

	for (size_t i = 0; i < state->jobs.len; i++) {
		struct mrsh_job *job = state->jobs.data[i];
		if (job_poll(job) >= 0) {
			continue;
		}
		char *cmd = mrsh_node_format(job->node);
		printf("[%d] %c %s %s\n", job->job_id, job == current ? '+' : ' ',
				job_state_str(job), cmd);
		free(cmd);
	}

	return EXIT_SUCCESS;
}
