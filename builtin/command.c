#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include "mrsh/getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "shell/path.h"
#include "mrsh/builtin.h"

static const char command_usage[] = "usage: command [-v|-V|-p] "
	"command_name [argument...]\n";

static int command_v(struct mrsh_state *state, const char *command_name) {
	size_t len_command_name = strlen(command_name);

	const char *look_alias =
		mrsh_hashtable_get(&state->aliases, command_name);
	if (look_alias != NULL) {
		printf("alias %s='%s'\n", command_name, look_alias);
		return 0;
	}

	const char *look_fn =
		mrsh_hashtable_get(&state->functions, command_name);
	if (look_fn != NULL) {
		printf("%s\n", command_name);
		return 0;
	}

	if (mrsh_has_builtin(command_name)) {
		printf("%s\n", command_name);
		return 0;
	}

	for (size_t i = 0; i < keywords_len; ++i) {
		if (strlen(keywords[i]) == len_command_name &&
				strcmp(command_name, keywords[i]) == 0) {
			printf("%s\n", command_name);
			return 0;
		}
	}

	const char *expanded = expand_path(state, command_name, 1);
	if (expanded != NULL) {
		printf("%s\n", expanded);
		return 0;
	}

	return 127;
}

int builtin_command(struct mrsh_state *state, int argc, char *argv[]) {
	mrsh_optind = 0;
	int opt;

	while ((opt = mrsh_getopt(argc, argv, ":vVp")) != -1) {
		switch (opt) {
		case 'v':
			if (argc != 3) {
				return 1;
			}
			return command_v(state, argv[mrsh_optind]);
		case 'V':
		case 'p':
			fprintf(stderr, "command: `-V` and `-p` and no arg not "
					"yet implemented\n");
			return 1;
		default:
			fprintf(stderr, "command: unknown option -- %c\n",
					mrsh_optopt);
			fprintf(stderr, command_usage);
			return 1;
		}
	}

	return 0;
}
