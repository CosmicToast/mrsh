#ifndef _MRSH_AST_H
#define _MRSH_AST_H

#include <mrsh/array.h>
#include <stdbool.h>

enum mrsh_token_type {
	MRSH_TOKEN_STRING,
	MRSH_TOKEN_PARAMETER,
	MRSH_TOKEN_COMMAND,
	MRSH_TOKEN_LIST,
};

/**
 * A token can be:
 * - An unquoted or a single-quoted string
 * - An unquoted or a double-quoted list of tokens
 */
struct mrsh_token {
	enum mrsh_token_type type;
};

/**
 * A string token is a type of token. It can be unquoted or single-quoted.
 */
struct mrsh_token_string {
	struct mrsh_token token;
	char *str;
	bool single_quoted;
};

/**
 * A token parameter is a type of token candidate for parameter expansion. The
 * format is either `$name` or `${expression}`.
 */
struct mrsh_token_parameter {
	struct mrsh_token token;
	char *name;
	char *op; // can be NULL
	struct mrsh_token *arg; // can be NULL
};

/**
 * A token command is a type of token candidate for command substitution. The
 * format is either `` `command` `` or `$(command)`.
 */
struct mrsh_token_command {
	struct mrsh_token token;
	char *command;
	bool back_quoted;
};

/**
 * A token list is a type of token. It can be unquoted or double-quoted.
 */
struct mrsh_token_list {
	struct mrsh_token token;
	struct mrsh_array children; // struct mrsh_token *
	bool double_quoted;
};

/**
 * An IO redirection operator. The format is: `[io_number]op name`.
 */
struct mrsh_io_redirect {
	int io_number; // -1 if unspecified
	char *op; // one of <, >, >|, >>, <&, <>, <<, <<-
	struct mrsh_token *name; // filename or here-document delimiter
	char *here_document; // only for << and <<-
};

/**
 * A variable assignment. The format is: `name=value`.
 */
struct mrsh_assignment {
	char *name;
	struct mrsh_token *value;
};

enum mrsh_command_type {
	MRSH_SIMPLE_COMMAND,
	MRSH_BRACE_GROUP,
	MRSH_IF_CLAUSE,
	MRSH_FUNCTION_DEFINITION,
};

/**
 * A command. It is either a simple command, a brace group or an if clause.
 */
struct mrsh_command {
	enum mrsh_command_type type;
};

/**
 * A simple command is a type of command. It contains a command name, followed
 * by command arguments. It can also contain IO redirections and variable
 * assignments.
 */
struct mrsh_simple_command {
	struct mrsh_command command;
	struct mrsh_token *name; // can be NULL if it contains only assignments
	struct mrsh_array arguments; // struct mrsh_token *
	struct mrsh_array io_redirects; // struct mrsh_io_redirect *
	struct mrsh_array assignments; // struct mrsh_assignment *
};

/**
 * A brace group is a type of command. It contains command lists and executes
 * them in the current process environment. The format is:
 * `{ compound-list ; }`.
 */
struct mrsh_brace_group {
	struct mrsh_command command;
	struct mrsh_array body; // struct mrsh_command_list *
};

/**
 * An if clause is a type of command. The format is:
 *
 *   if compound-list
 *   then
 *       compound-list
 *   [elif compound-list
 *   then
 *       compound-list] ...
 *   [else
 *       compound-list]
 *   fi
 */
struct mrsh_if_clause {
	struct mrsh_command command;
	struct mrsh_array condition; // struct mrsh_command_list *
	struct mrsh_array body; // struct mrsh_command_list *
	struct mrsh_command *else_part; // can be NULL
};

/**
 * A function definition is a type of command. The format is:
 *
 *   fname ( ) compound-command [io-redirect ...]
 */
struct mrsh_function_definition {
	struct mrsh_command command;
	char *name;
	struct mrsh_command *body;
};

enum mrsh_node_type {
	MRSH_NODE_PIPELINE,
	MRSH_NODE_BINOP,
};

/**
 * A node is an AND-OR list component. It is either a pipeline or a binary
 * operation.
 */
struct mrsh_node {
	enum mrsh_node_type type;
};

/**
 * A pipeline is a type of node which consists of multiple commands
 * separated by `|`. The format is: `[!] command1 [ | command2 ...]`.
 */
struct mrsh_pipeline {
	struct mrsh_node node;
	struct mrsh_array commands; // struct mrsh_command *
	bool bang; // whether the pipeline begins with `!`
};

enum mrsh_binop_type {
	MRSH_BINOP_AND, // `&&`
	MRSH_BINOP_OR, // `||`
};

/**
 * A binary operation is a type of node which consists of multiple pipelines
 * separated by `&&` or `||`.
 */
struct mrsh_binop {
	struct mrsh_node node;
	enum mrsh_binop_type type;
	struct mrsh_node *left, *right;
};

/**
 * A command list contains AND-OR lists separated by `;` (for sequential
 * execution) or `&` (for asynchronous execution).
 */
struct mrsh_command_list {
	struct mrsh_node *node;
	bool ampersand; // whether the command list ends with `&`
};

/**
 * A shell program. It contains command lists.
 */
struct mrsh_program {
	struct mrsh_array body; // struct mrsh_command_list *
};

void mrsh_token_destroy(struct mrsh_token *token);
void mrsh_io_redirect_destroy(struct mrsh_io_redirect *redir);
void mrsh_assignment_destroy(struct mrsh_assignment *assign);
void mrsh_command_destroy(struct mrsh_command *cmd);
void mrsh_node_destroy(struct mrsh_node *node);
void mrsh_command_list_destroy(struct mrsh_command_list *l);
void mrsh_program_destroy(struct mrsh_program *prog);
struct mrsh_token_string *mrsh_token_string_create(char *str,
	bool single_quoted);
struct mrsh_token_parameter *mrsh_token_parameter_create(char *name, char *op,
	struct mrsh_token *arg);
struct mrsh_token_command *mrsh_token_command_create(char *command,
	bool back_quoted);
struct mrsh_token_list *mrsh_token_list_create(struct mrsh_array *children,
	bool double_quoted);
struct mrsh_token_string *mrsh_token_get_string(struct mrsh_token *token);
struct mrsh_token_parameter *mrsh_token_get_parameter(struct mrsh_token *token);
struct mrsh_token_command *mrsh_token_get_command(struct mrsh_token *token);
struct mrsh_token_list *mrsh_token_get_list(struct mrsh_token *token);
struct mrsh_simple_command *mrsh_simple_command_create(struct mrsh_token *name,
	struct mrsh_array *arguments, struct mrsh_array *io_redirects,
	struct mrsh_array *assignments);
struct mrsh_brace_group *mrsh_brace_group_create(struct mrsh_array *body);
struct mrsh_if_clause *mrsh_if_clause_create(struct mrsh_array *condition,
	struct mrsh_array *body, struct mrsh_command *else_part);
struct mrsh_function_definition *mrsh_function_definition_create(char *name,
	struct mrsh_command *body);
struct mrsh_simple_command *mrsh_command_get_simple_command(
	struct mrsh_command *cmd);
struct mrsh_brace_group *mrsh_command_get_brace_group(struct mrsh_command *cmd);
struct mrsh_if_clause *mrsh_command_get_if_clause(struct mrsh_command *cmd);
struct mrsh_function_definition *mrsh_command_get_function_definition(
	struct mrsh_command *cmd);

struct mrsh_pipeline *mrsh_pipeline_create(struct mrsh_array *commands,
	bool bang);
struct mrsh_binop *mrsh_binop_create(enum mrsh_binop_type type,
	struct mrsh_node *left, struct mrsh_node *right);
struct mrsh_pipeline *mrsh_node_get_pipeline(struct mrsh_node *node);
struct mrsh_binop *mrsh_node_get_binop(struct mrsh_node *node);

char *mrsh_token_str(struct mrsh_token *token);
void mrsh_program_print(struct mrsh_program *prog);

#endif
