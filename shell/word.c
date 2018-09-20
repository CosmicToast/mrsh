#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <glob.h>
#include <mrsh/buffer.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "shell/shell.h"
#include "shell/word.h"

void expand_tilde(struct mrsh_state *state, char **str_ptr) {
	char *str = *str_ptr;
	if (str[0] != '~') {
		return;
	}

	const char *end = str + strlen(str);
	const char *slash = strchr(str, '/');
	if (slash == NULL) {
		slash = end;
	}

	char *name = NULL;
	if (slash > str + 1) {
		name = strndup(str + 1, slash - str - 1);
	}

	const char *dir = NULL;
	if (name == NULL) {
		dir = mrsh_hashtable_get(&state->variables, "HOME");
	} else {
		struct passwd *pw = getpwnam(name);
		if (pw != NULL) {
			dir = pw->pw_dir;
		}
	}
	free(name);

	if (dir == NULL) {
		return;
	}

	size_t dir_len = strlen(dir);
	size_t trailing_len = end - slash;
	char *expanded = malloc(dir_len + trailing_len + 1);
	if (expanded == NULL) {
		return;
	}
	memcpy(expanded, dir, dir_len);
	memcpy(expanded + dir_len, slash, trailing_len);
	expanded[dir_len + trailing_len] = '\0';

	free(str);
	*str_ptr = expanded;
}

struct split_fields_data {
	const char *ifs, *ifs_non_space;
	bool in_ifs, in_ifs_non_space;
};

static void _split_fields(struct mrsh_array *fields, struct mrsh_buffer *buf,
		struct mrsh_word *word, bool double_quoted,
		struct split_fields_data *data) {
	switch (word->type) {
	case MRSH_WORD_STRING:;
		struct mrsh_word_string *ws = mrsh_word_get_string(word);

		if (double_quoted || ws->single_quoted) {
			mrsh_buffer_append(buf, ws->str, strlen(ws->str));
			data->in_ifs = data->in_ifs_non_space = false;
			return;
		}

		size_t len = strlen(ws->str);
		for (size_t i = 0; i < len; ++i) {
			char c = ws->str[i];
			if (strchr(data->ifs, c) == NULL) {
				mrsh_buffer_append_char(buf, c);
				data->in_ifs = data->in_ifs_non_space = false;
				continue;
			}

			bool is_ifs_non_space = strchr(data->ifs_non_space, c) != NULL;
			if (!data->in_ifs || (is_ifs_non_space && data->in_ifs_non_space)) {
				mrsh_buffer_append_char(buf, '\0');
				char *str = mrsh_buffer_steal(buf);
				mrsh_array_add(fields, str);
				data->in_ifs = true;
			} else if (is_ifs_non_space) {
				data->in_ifs_non_space = true;
			}
		}
		break;
	case MRSH_WORD_LIST:;
		struct mrsh_word_list *wl = mrsh_word_get_list(word);
		for (size_t i = 0; i < wl->children.len; ++i) {
			struct mrsh_word *child = wl->children.data[i];
			_split_fields(fields, buf, child,
				double_quoted || wl->double_quoted, data);
		}
		break;
	default:
		assert(false);
	}
}

void split_fields(struct mrsh_array *fields, struct mrsh_word *word,
		const char *ifs) {
	if (ifs == NULL) {
		ifs = " \t\n";
	} else if (ifs[0] == '\0') {
		char *str = mrsh_word_str(word);
		mrsh_array_add(fields, str);
		return;
	}

	size_t ifs_len = strlen(ifs);
	char *ifs_non_space = calloc(ifs_len, sizeof(char));
	size_t ifs_non_space_len = 0;
	for (size_t i = 0; i < ifs_len; ++i) {
		if (!isspace(ifs[i])) {
			ifs_non_space[ifs_non_space_len++] = ifs[i];
		}
	}

	struct mrsh_buffer buf = {0};
	struct split_fields_data data = {
		.ifs = ifs,
		.ifs_non_space = ifs_non_space,
		.in_ifs = true,
	};
	_split_fields(fields, &buf, word, false, &data);
	if (!data.in_ifs) {
		mrsh_buffer_append_char(&buf, '\0');
		char *str = mrsh_buffer_steal(&buf);
		mrsh_array_add(fields, str);
	}
	mrsh_buffer_finish(&buf);

	free(ifs_non_space);
}

bool expand_pathnames(struct mrsh_array *expanded, struct mrsh_array *fields) {
	const char metachars[] = "*?[";

	for (size_t i = 0; i < fields->len; ++i) {
		const char *field = fields->data[i];

		if (strpbrk(field, metachars) == NULL) {
			mrsh_array_add(expanded, strdup(field));
			continue;
		}

		glob_t glob_buf;
		int ret = glob(field, GLOB_NOSORT | GLOB_NOCHECK, NULL, &glob_buf);
		if (ret == 0) {
			for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
				mrsh_array_add(expanded, strdup(glob_buf.gl_pathv[i]));
			}
			globfree(&glob_buf);
		} else {
			assert(ret != GLOB_NOMATCH);
			fprintf(stderr, "glob() failed\n");
			return false;
		}
	}

	return true;
}
