#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "shell.h"

void mrsh_state_init(struct mrsh_state *state) {
	state->exit = -1;
	state->interactive = isatty(STDIN_FILENO);
	state->options = state->interactive ? MRSH_OPT_INTERACTIVE : 0;
	state->input = stdin;
}

static void state_string_finish_iterator(const char *key, void *value,
		void *user_data) {
	free(value);
}

void mrsh_state_finish(struct mrsh_state *state) {
	mrsh_hashtable_for_each(&state->variables,
		state_string_finish_iterator, NULL);
	mrsh_hashtable_finish(&state->variables);
	mrsh_hashtable_for_each(&state->aliases,
		state_string_finish_iterator, NULL);
	mrsh_hashtable_finish(&state->aliases);
	for (int i = 0; i < state->argc; ++i) {
		free(state->argv[i]);
	}
	free(state->argv);
}

static struct task *handle_simple_command(struct mrsh_simple_command *sc) {
	struct task *task_list = task_list_create();

	for (size_t i = 0; i < sc->assignments.len; ++i) {
		struct mrsh_assignment *assign = sc->assignments.data[i];
		task_list_add(task_list,
			task_word_create(&assign->value, TILDE_EXPANSION_ASSIGNMENT));
	}

	if (sc->name == NULL) {
		task_list_add(task_list, task_assignment_create(&sc->assignments));
		return task_list;
	}

	task_list_add(task_list,
		task_word_create(&sc->name, TILDE_EXPANSION_NAME));

	for (size_t i = 0; i < sc->arguments.len; ++i) {
		struct mrsh_word **arg_ptr =
			(struct mrsh_word **)&sc->arguments.data[i];
		task_list_add(task_list,
			task_word_create(arg_ptr, TILDE_EXPANSION_NAME));
	}

	for (size_t i = 0; i < sc->io_redirects.len; ++i) {
		struct mrsh_io_redirect *redir = sc->io_redirects.data[i];
		task_list_add(task_list,
			task_word_create(&redir->name, TILDE_EXPANSION_NAME));
		for (size_t j = 0; j < redir->here_document.len; ++j) {
			struct mrsh_word **line_word_ptr =
				(struct mrsh_word **)&redir->here_document.data[j];
			task_list_add(task_list,
				task_word_create(line_word_ptr, TILDE_EXPANSION_NAME));
		}
	}

	task_list_add(task_list, task_command_create(sc));

	return task_list;
}

static struct task *handle_node(struct mrsh_node *node);

static struct task *handle_command_list_array(struct mrsh_array *array) {
	struct task *task_list = task_list_create();

	for (size_t i = 0; i < array->len; ++i) {
		struct mrsh_command_list *list = array->data[i];
		struct task *child = handle_node(list->node);
		if (list->ampersand) {
			child = task_async_create(child);
		}
		task_list_add(task_list, child);
	}

	return task_list;
}

static struct task *handle_command(struct mrsh_command *cmd);

static struct task *handle_if_clause(struct mrsh_if_clause *ic) {
	struct task *condition = handle_command_list_array(&ic->condition);
	struct task *body = handle_command_list_array(&ic->body);
	struct task *else_part = NULL;
	if (ic->else_part != NULL) {
		else_part = handle_command(ic->else_part);
	}
	return task_if_clause_create(condition, body, else_part);
}

static struct task *handle_command(struct mrsh_command *cmd) {
	switch (cmd->type) {
	case MRSH_SIMPLE_COMMAND:;
		struct mrsh_simple_command *sc = mrsh_command_get_simple_command(cmd);
		return handle_simple_command(sc);
	case MRSH_BRACE_GROUP:;
		struct mrsh_brace_group *bg = mrsh_command_get_brace_group(cmd);
		return handle_command_list_array(&bg->body);
	case MRSH_SUBSHELL:;
		struct mrsh_subshell *s = mrsh_command_get_subshell(cmd);
		return task_subshell_create(handle_command_list_array(&s->body));
	case MRSH_IF_CLAUSE:;
		struct mrsh_if_clause *ic = mrsh_command_get_if_clause(cmd);
		return handle_if_clause(ic);
	case MRSH_FOR_CLAUSE:
	case MRSH_LOOP_CLAUSE:
	case MRSH_CASE_CLAUSE:
	case MRSH_FUNCTION_DEFINITION:
		assert(false); // TODO: implement this
	}
	assert(false);
}

static struct task *handle_pipeline(struct mrsh_pipeline *pl) {
	struct task *task_pipeline = task_pipeline_create();

	for (size_t i = 0; i < pl->commands.len; ++i) {
		struct mrsh_command *cmd = pl->commands.data[i];
		task_pipeline_add(task_pipeline, handle_command(cmd));
	}

	return task_pipeline;
}

static struct task *handle_binop(struct mrsh_binop *binop) {
	struct task *left = handle_node(binop->left);
	struct task *right = handle_node(binop->right);
	return task_binop_create(binop->type, left, right);
}

static struct task *handle_node(struct mrsh_node *node) {
	switch (node->type) {
	case MRSH_NODE_PIPELINE:;
		struct mrsh_pipeline *pl = mrsh_node_get_pipeline(node);
		return handle_pipeline(pl);
	case MRSH_NODE_BINOP:;
		struct mrsh_binop *binop = mrsh_node_get_binop(node);
		return handle_binop(binop);
	}
	assert(false);
}

int mrsh_run_program(struct mrsh_state *state, struct mrsh_program *prog) {
	struct task *task = handle_command_list_array(&prog->body);

	struct context ctx = {
		.state = state,
		.stdin_fileno = -1,
		.stdout_fileno = -1,
	};
	int ret = task_run(task, &ctx);
	task_destroy(task);
	return ret;
}

int mrsh_run_word(struct mrsh_state *state, struct mrsh_word **word) {
	struct task *task = task_word_create(word, TILDE_EXPANSION_NAME);

	struct context ctx = {
		.state = state,
		.stdin_fileno = -1,
		.stdout_fileno = -1,
	};
	int ret = task_run(task, &ctx);
	task_destroy(task);
	return ret;
}
