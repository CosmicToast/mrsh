#include <assert.h>
#include <mrsh/ast.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define L_LINE "│ "
#define L_VAL  "├─"
#define L_LAST "└─"
#define L_GAP  "  "

static size_t make_sub_prefix(const char *prefix, bool last, char *buf) {
	if (buf != NULL) {
		memcpy(buf, prefix, strlen(prefix) + 1);
		strcat(buf, last ? L_GAP : L_LINE);
	}
	return strlen(prefix) + strlen(L_LINE) + 1;
}

static void print_prefix(const char *prefix, bool last) {
	printf("%s%s", prefix, last ? L_LAST : L_VAL);
}

static const char *word_parameter_op_str(enum mrsh_word_parameter_op op) {
	switch (op) {
	case MRSH_PARAM_NONE:
		return NULL;
	case MRSH_PARAM_MINUS:
		return "-";
	case MRSH_PARAM_EQUAL:
		return "=";
	case MRSH_PARAM_QMARK:
		return "?";
	case MRSH_PARAM_PLUS:
		return "+";
	case MRSH_PARAM_LEADING_HASH:
		return "# (leading)";
	case MRSH_PARAM_PERCENT:
		return "%";
	case MRSH_PARAM_DPERCENT:
		return "%%";
	case MRSH_PARAM_HASH:
		return "#";
	case MRSH_PARAM_DHASH:
		return "##";
	}
	assert(false);
}

static void print_program(struct mrsh_program *prog, const char *prefix);

static void print_word(struct mrsh_word *word, const char *prefix) {
	char sub_prefix[make_sub_prefix(prefix, true, NULL)];

	switch (word->type) {
	case MRSH_WORD_STRING:;
		struct mrsh_word_string *ws = mrsh_word_get_string(word);
		assert(ws != NULL);
		printf("word_string%s %s\n",
			ws->single_quoted ? " (quoted)" : "", ws->str);
		break;
	case MRSH_WORD_PARAMETER:;
		struct mrsh_word_parameter *wp = mrsh_word_get_parameter(word);
		assert(wp != NULL);
		printf("word_parameter\n");

		print_prefix(prefix, wp->op == MRSH_PARAM_NONE && wp->arg == NULL);
		printf("name %s\n", wp->name);

		if (wp->op != MRSH_PARAM_NONE) {
			print_prefix(prefix, wp->arg == NULL);
			printf("op %s%s\n",
				wp->colon ? ":" : "", word_parameter_op_str(wp->op));
		}

		if (wp->arg != NULL) {
			make_sub_prefix(prefix, true, sub_prefix);

			print_prefix(prefix, true);
			printf("arg ─ ");
			print_word(wp->arg, sub_prefix);
		}
		break;
	case MRSH_WORD_COMMAND:;
		struct mrsh_word_command *wc = mrsh_word_get_command(word);
		assert(wc != NULL);

		make_sub_prefix(prefix, true, sub_prefix);

		print_prefix(prefix, true);
		printf("word_command%s ─ ", wc->back_quoted ? " (quoted)" : "");
		print_program(wc->program, sub_prefix);
		break;
	case MRSH_WORD_LIST:;
		struct mrsh_word_list *wl = mrsh_word_get_list(word);
		assert(wl != NULL);
		printf("word_list%s\n", wl->double_quoted ? " (quoted)" : "");

		for (size_t i = 0; i < wl->children.len; ++i) {
			struct mrsh_word *child = wl->children.data[i];
			bool last = i == wl->children.len - 1;

			make_sub_prefix(prefix, last, sub_prefix);

			print_prefix(prefix, last);
			print_word(child, sub_prefix);
		}
		break;
	}
}

static const char *io_redirect_op_str(enum mrsh_io_redirect_op op) {
	switch (op) {
	case MRSH_IO_LESS:
		return "<";
	case MRSH_IO_GREAT:
		return ">";
	case MRSH_IO_CLOBBER:
		return ">|";
	case MRSH_IO_DGREAT:
		return ">>";
	case MRSH_IO_LESSAND:
		return "<&";
	case MRSH_IO_GREATAND:
		return ">&";
	case MRSH_IO_LESSGREAT:
		return "<>";
	case MRSH_IO_DLESS:
		return "<<";
	case MRSH_IO_DLESSDASH:
		return "<<-";
	}
	assert(false);
}

static void print_io_redirect(struct mrsh_io_redirect *redir,
		const char *prefix) {
	printf("io_redirect\n");

	print_prefix(prefix, false);
	printf("io_number %d\n", redir->io_number);

	print_prefix(prefix, false);
	printf("op %s\n", io_redirect_op_str(redir->op));

	char sub_prefix[make_sub_prefix(prefix, true, NULL)];
	make_sub_prefix(prefix, true, sub_prefix);

	print_prefix(prefix, true);
	printf("name ─ ");
	print_word(redir->name, sub_prefix);
}

static void print_assignment(struct mrsh_assignment *assign,
		const char *prefix) {
	printf("assignment\n");

	print_prefix(prefix, false);
	printf("name %s\n", assign->name);

	char sub_prefix[make_sub_prefix(prefix, true, NULL)];
	make_sub_prefix(prefix, true, sub_prefix);

	print_prefix(prefix, true);
	printf("value ─ ");
	print_word(assign->value, sub_prefix);
}

static void print_simple_command(struct mrsh_simple_command *cmd,
		const char *prefix) {
	printf("simple_command\n");

	char sub_prefix[make_sub_prefix(prefix, false, NULL)];

	if (cmd->name != NULL) {
		bool last = cmd->arguments.len == 0 && cmd->io_redirects.len == 0
			&& cmd->assignments.len == 0;
		make_sub_prefix(prefix, last, sub_prefix);

		print_prefix(prefix, last);
		printf("name ─ ");
		print_word(cmd->name, sub_prefix);
	}

	for (size_t i = 0; i < cmd->arguments.len; ++i) {
		struct mrsh_word *arg = cmd->arguments.data[i];
		bool last = i == cmd->arguments.len - 1 && cmd->io_redirects.len == 0
			&& cmd->assignments.len == 0;

		make_sub_prefix(prefix, last, sub_prefix);

		print_prefix(prefix, last);
		printf("argument %zu ─ ", i + 1);
		print_word(arg, sub_prefix);
	}

	for (size_t i = 0; i < cmd->io_redirects.len; ++i) {
		struct mrsh_io_redirect *redir = cmd->io_redirects.data[i];
		bool last = i == cmd->io_redirects.len - 1 && cmd->assignments.len == 0;

		make_sub_prefix(prefix, last, sub_prefix);

		print_prefix(prefix, last);
		print_io_redirect(redir, sub_prefix);
	}

	for (size_t i = 0; i < cmd->assignments.len; ++i) {
		struct mrsh_assignment *assign = cmd->assignments.data[i];
		bool last = i == cmd->assignments.len - 1;

		make_sub_prefix(prefix, last, sub_prefix);

		print_prefix(prefix, last);
		print_assignment(assign, sub_prefix);
	}
}

static void print_command_list(struct mrsh_command_list *l, const char *prefix);

static void print_command_list_array(struct mrsh_array *array,
		const char *prefix) {
	for (size_t i = 0; i < array->len; ++i) {
		struct mrsh_command_list *l = array->data[i];
		bool last = i == array->len - 1;

		char sub_prefix[make_sub_prefix(prefix, last, NULL)];
		make_sub_prefix(prefix, last, sub_prefix);

		print_prefix(prefix, last);
		print_command_list(l, sub_prefix);
	}
}

static void print_brace_group(struct mrsh_brace_group *bg, const char *prefix) {
	printf("brace_group\n");

	print_command_list_array(&bg->body, prefix);
}

static void print_command(struct mrsh_command *cmd, const char *prefix);

static void print_if_clause(struct mrsh_if_clause *ic, const char *prefix) {
	printf("if_clause\n");

	char sub_prefix[make_sub_prefix(prefix, false, NULL)];
	make_sub_prefix(prefix, false, sub_prefix);

	print_prefix(prefix, false);
	printf("condition\n");
	print_command_list_array(&ic->condition, sub_prefix);

	bool last = ic->else_part == NULL;
	make_sub_prefix(prefix, last, sub_prefix);

	print_prefix(prefix, last);
	printf("body\n");
	print_command_list_array(&ic->body, sub_prefix);

	if (ic->else_part != NULL) {
		make_sub_prefix(prefix, true, sub_prefix);

		print_prefix(prefix, true);
		printf("else_part ─ ");
		print_command(ic->else_part, sub_prefix);
	}
}

static void print_function_definition(struct mrsh_function_definition *fd,
		const char *prefix) {
	printf("function_definition %s ─ ", fd->name);
	print_command(fd->body, prefix);
}

static void print_command(struct mrsh_command *cmd, const char *prefix) {
	switch (cmd->type) {
	case MRSH_SIMPLE_COMMAND:;
		struct mrsh_simple_command *sc = mrsh_command_get_simple_command(cmd);
		assert(sc != NULL);
		print_simple_command(sc, prefix);
		break;
	case MRSH_BRACE_GROUP:;
		struct mrsh_brace_group *bg = mrsh_command_get_brace_group(cmd);
		assert(bg != NULL);
		print_brace_group(bg, prefix);
		break;
	case MRSH_IF_CLAUSE:;
		struct mrsh_if_clause *ic = mrsh_command_get_if_clause(cmd);
		assert(ic != NULL);
		print_if_clause(ic, prefix);
		break;
	case MRSH_FUNCTION_DEFINITION:;
		struct mrsh_function_definition *fd =
			mrsh_command_get_function_definition(cmd);
		assert(fd != NULL);
		print_function_definition(fd, prefix);
		break;
	}
}

static void print_pipeline(struct mrsh_pipeline *pl, const char *prefix) {
	printf("pipeline%s\n", pl->bang ? " !" : "");

	for (size_t i = 0; i < pl->commands.len; ++i) {
		struct mrsh_command *cmd = pl->commands.data[i];
		bool last = i == pl->commands.len - 1;

		char sub_prefix[make_sub_prefix(prefix, last, NULL)];
		make_sub_prefix(prefix, last, sub_prefix);

		print_prefix(prefix, last);
		print_command(cmd, sub_prefix);
	}
}

static const char *binop_type_str(enum mrsh_binop_type type) {
	switch (type) {
	case MRSH_BINOP_AND:
		return "&&";
	case MRSH_BINOP_OR:
		return "||";
	}
	return NULL;
}

static void print_node(struct mrsh_node *node, const char *prefix);

static void print_binop(struct mrsh_binop *binop, const char *prefix) {
	printf("binop %s\n", binop_type_str(binop->type));

	char sub_prefix[make_sub_prefix(prefix, false, NULL)];

	make_sub_prefix(prefix, false, sub_prefix);
	print_prefix(prefix, false);
	print_node(binop->left, sub_prefix);

	make_sub_prefix(prefix, true, sub_prefix);
	print_prefix(prefix, true);
	print_node(binop->right, sub_prefix);
}

static void print_node(struct mrsh_node *node, const char *prefix) {
	switch (node->type) {
	case MRSH_NODE_PIPELINE:;
		struct mrsh_pipeline *pl = mrsh_node_get_pipeline(node);
		assert(pl != NULL);
		print_pipeline(pl, prefix);
		break;
	case MRSH_NODE_BINOP:;
		struct mrsh_binop *binop = mrsh_node_get_binop(node);
		assert(binop != NULL);
		print_binop(binop, prefix);
		break;
	}
}

static void print_command_list(struct mrsh_command_list *list,
		const char *prefix) {
	printf("command_list%s ─ ", list->ampersand ? " &" : "");

	print_node(list->node, prefix);
}

static void print_program(struct mrsh_program *prog, const char *prefix) {
	printf("program\n");

	print_command_list_array(&prog->body, prefix);
}

void mrsh_program_print(struct mrsh_program *prog) {
	print_program(prog, "");
}
