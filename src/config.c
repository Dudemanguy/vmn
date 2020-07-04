#include <dirent.h>
#include <limits.h>
#include <mpv/client.h>
#include <ncurses.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"
#include "utils.h"

#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

int check_arg(struct vmn_config *cfg, char *arg);
int check_sort(char *str);
enum vmn_config_sort default_sort(char *tags);
void get_cfg_file(struct vmn_config *cfg);
char *get_default_lib();
void key_default(struct vmn_config *cfg);
char **parse_arg(char *arg);
char *read_arg(char *arg);
void read_cfg_file(struct vmn_config *cfg);
char *read_dir_arg(char *arg);
void set_option(struct vmn_config *cfg, char *opt, char *value);
int set_option_key(struct vmn_config *cfg, char *opt, char *value);
int set_option_mpv(struct vmn_config *cfg, char *opt, char *value);
int set_option_vmn(struct vmn_config *cfg, char *opt, char *value);

void cfg_default(struct vmn_config *cfg) {
	cfg->input_mode = strdup("no");
	cfg->lib_dir = get_default_lib();
	cfg->tags_len = 3;
	char *default_tags = "artist,album,title";
	cfg->tags = parse_arg(default_tags);
	cfg->sort = (enum vmn_config_sort *)calloc(cfg->tags_len, sizeof(enum vmn_config_sort));
	for (int i = 0; i < cfg->tags_len; ++i) {
		cfg->sort[i] = default_sort(cfg->tags[i]);
	}
	cfg->view = V_DATA;

	//create default mpv_opts array
	cfg->mpv_opts_len = 8;
	cfg->mpv_opts = malloc(cfg->mpv_opts_len*sizeof(char*));
	cfg->mpv_opts[0] = strdup("input-default-bindings");
	cfg->mpv_opts[1] = strdup("yes");
	cfg->mpv_opts[2] = strdup("input-vo-keyboard");
	cfg->mpv_opts[3] = strdup("yes");
	cfg->mpv_opts[4] = strdup("force-window");
	cfg->mpv_opts[5] = strdup("yes");
	cfg->mpv_opts[6] = strdup("osc");
	cfg->mpv_opts[7] = strdup("yes");
}

struct vmn_config cfg_init(int argc, char *argv[]) {
	struct vmn_config cfg;
	cfg.key_count = 27;
	cfg_default(&cfg);
	key_default(&cfg);
	get_cfg_file(&cfg);
	if (cfg.err) {
		printf("An error occured while trying to create the config directory. Make sure your permissions to ~/.config are correct.\n");
		return cfg;
	}
	read_cfg_file(&cfg);
	char *headless;
	char *input;
	char *library;
	char *sort;
	char *tags;
	char *viewcfg;
	int pos[argc];
	int headless_arg = 0;
	int input_arg = 0;
	int lib_arg = 0;
	int tags_arg = 0;
	int sort_arg = 0;
	int view_arg = 0;
	
	//check for any command line arguments
	//these take priority over any config file options
	for (int i = 1; i < argc; ++i) {
		pos[i] = check_arg(&cfg, argv[i]);
		if (pos[i] == -1) {
			printf("Error: invalid argument specified.\n");
			cfg.err = 1;
			return cfg;
		}
	}

	for (int i = 1; i < argc; ++i) {
		if (pos[i] == 1) {
			headless_arg = i;
		}
		if (pos[i] == 2) {
			input_arg = i;
		}
		if (pos[i] == 3) {
			lib_arg = i;
		}
		if (pos[i] == 4) {
			sort_arg = i;
		}
		if (pos[i] == 5) {
			tags_arg = i;
		}
		if (pos[i] == 6) {
			view_arg = i;
		}
	}

	if (headless_arg) {
		headless = read_arg(argv[headless_arg]);
		if (strcmp(headless, "yes") == 0) {
			mpv_cfg_add(&cfg, "force-window", "no");
			mpv_cfg_add(&cfg, "video", "no");
			mpv_cfg_add(&cfg, "osc", "no");
		} else if (strcmp(headless, "no") == 0) {
			mpv_cfg_add(&cfg, "force-window", "yes");
			mpv_cfg_add(&cfg, "video", "yes");
			mpv_cfg_add(&cfg, "osc", "yes");
		} else {
			printf("headless can only be set to 'yes' or 'no'\n");
		}
	}

	if (input_arg) {
		input = read_arg(argv[input_arg]);
		if ((strcmp(input, "yes") == 0) || (strcmp(input, "no") == 0)) {
			free(cfg.input_mode);
			cfg.input_mode = strdup(input);
		} else {
			printf("input-mode can only be set to 'yes' or 'no'\n");
		}
	}

	if (lib_arg) {
		library = read_dir_arg(argv[lib_arg]);
		if (strcmp(library, "") == 0) {
			printf("Library directory not found. Falling back to default.\n");
		} else {
			free(cfg.lib_dir);
			cfg.lib_dir = strdup(library);
			free(library);
		}
	}

	if (view_arg) {
		viewcfg = read_arg(argv[view_arg]);
		if (strcmp(viewcfg, "file-path") == 0) {
			cfg.view = V_PATH;
		} else if (strcmp(viewcfg, "metadata") == 0) {
			cfg.view = V_DATA;
		} else if (strcmp(viewcfg, "song-only") == 0) {
			cfg.view = V_SONG;
		} else {
			cfg.view = V_DATA;
			printf("Invalid view specified. Falling back to default.\n");
		}
	}

	if (cfg.view == V_DATA) {
		if (tags_arg) {
			for (int i = 0; i < cfg.tags_len; ++i) {
				free(cfg.tags[i]);
			}
			free(cfg.tags);
			tags = read_arg(argv[tags_arg]);
			char *len_check = strdup(tags);
			char *token = strtok(len_check, ",");
			int j = 0;
			while (token != NULL) {
				token = strtok(NULL, ",");
				++j;
			}
			cfg.tags_len = j;
			cfg.tags = parse_arg(tags);
			free(cfg.sort);
			cfg.sort = (enum vmn_config_sort *)calloc(cfg.tags_len, sizeof(enum vmn_config_sort));
			for (int i = 0; i < cfg.tags_len; ++i) {
				cfg.sort[i] = default_sort(cfg.tags[i]);
			}
			free(len_check);
		}

		if (sort_arg) {
			sort = read_arg(argv[sort_arg]);
			char *len_check = strdup(sort);
			char *token = strtok(len_check, ",");
			int valid = check_sort(token);
			int j = 0;
			while (token != NULL) {
				if (!valid) {
					break;
				}
				token = strtok(NULL, ",");
				++j;
			}
			free(len_check);
			if (!valid) {
				printf("Invalid sort argument specified. Resetting to default. \n");
				free(cfg.sort);
				cfg.sort = (enum vmn_config_sort *)calloc(cfg.tags_len, sizeof(enum vmn_config_sort));
				for (int i = 0; i < cfg.tags_len; ++i) {
					cfg.sort[i] = default_sort(cfg.tags[i]);
				}
			} else {
				if (cfg.tags_len != j) {
					printf("The length of the sort argument must be exactly equal to the length of the tags argument. Resetting to default. \n");
					free(cfg.sort);
					cfg.sort = (enum vmn_config_sort *)calloc(cfg.tags_len, sizeof(enum vmn_config_sort));
					for (int i = 0; i < cfg.tags_len; ++i) {
						cfg.sort[i] = default_sort(cfg.tags[i]);
					}
				} else {
					char **sort_arr = parse_arg(sort);
					free(cfg.sort);
					cfg.sort = (enum vmn_config_sort *)calloc(cfg.tags_len, sizeof(enum vmn_config_sort));
					for (int i = 0; i < cfg.tags_len; ++i) {
						if (strcmp(sort_arr[i], "metadata") == 0) {
							cfg.sort[i] = S_DATA;
						} else if (strcmp(sort_arr[i], "filename") == 0) {
							cfg.sort[i] = S_FILE;
						} else if (strcmp(sort_arr[i], "none") == 0) {
							cfg.sort[i] = S_NONE;
						} else if (strcmp(sort_arr[i], "tracknumber") == 0) {
							cfg.sort[i] = S_NUMB;
						} else if (strcmp(sort_arr[i], "random") == 0) {
							cfg.sort[i] = S_RAND;
						}
					}
					for (int i = 0; i < cfg.tags_len; ++i) {
						free(sort_arr[i]);
					}
					free(sort_arr);
				}
			}
		}
	}
	return cfg;
}

int check_arg(struct vmn_config *cfg, char *arg) {
	char *valid[7] = {"", "--headless=", "--input-mode=", "--library=", "--sort=", "--tags=", "--view="};
	int len[7];
	for (int i = 0; i < 7; ++i) {
		len[i] = strlen(valid[i]);
	}
	for (int i = 1; i < 7; ++i) {
		if (strncmp(arg, valid[i], len[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int check_func(char *func) {
	char *func_keys[12] = {"f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12"};
	for (int i = 0; i < 12; ++i) {
		if (strcmp(func, func_keys[i]) == 0) {
			return KEY_F(i+1);
		}
	}
	return 0;
}

int check_macro(char *macro) {
	if (strcmp(macro, "KEY_DOWN") == 0) {
		return KEY_DOWN;
	} else if (strcmp(macro, "KEY_UP") == 0) {
		return KEY_UP;
	} else if (strcmp(macro, "KEY_LEFT") == 0) {
		return KEY_LEFT;
	} else if (strcmp(macro, "KEY_RIGHT") == 0) {
		return KEY_RIGHT;
	} else if (strcmp(macro, "KEY_HOME") == 0) {
		return KEY_HOME;
	} else if (strcmp(macro, "KEY_BACKSPACE") == 0) {
		return KEY_BACKSPACE;
	} else if (strcmp(macro, "KEY_F0") == 0) {
		return KEY_F0;
	} else if (strcmp(macro, "KEY_IC") == 0) {
		return KEY_IC;
	} else if (strcmp(macro, "KEY_NPAGE") == 0) {
		return KEY_NPAGE;
	} else if (strcmp(macro, "KEY_PPAGE") == 0) {
		return KEY_PPAGE;
	} else if (strcmp(macro, "KEY_ENTER") == 0) {
		return KEY_ENTER;
	} else if (strcmp(macro, "KEY_PRINT") == 0) {
		return KEY_PRINT;
	} else if (strcmp(macro, "KEY_END") == 0) {
		return KEY_END;
	} else {
		return 0;
	}
}

int check_sort(char *str) {
	char *sort_values[4] = {"metadata", "filename", "tracknumber", "random"};
	for (int i = 0; i < 4; ++i) {
		if (strcmp(str, sort_values[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

enum vmn_config_sort default_sort(char *tags) {
	if (strcasecmp(tags, "title") == 0) {
		return S_NUMB;
	} else {
		return S_DATA;
	}
}

void get_cfg_file(struct vmn_config *cfg) {
	char *home = getenv("HOME");
	const char *path = "/.config/vmn";
	char *cfg_dir = malloc(strlen(home) + strlen(path) + 1);
	strcpy(cfg_dir, home);
	strcat(cfg_dir, path);
	DIR *dir = opendir(cfg_dir);
	if (!dir) {
		int err = mkdir(cfg_dir, 0755);
		if (err) {
			printf("An error occured while trying to create the config directory. Make sure your permissions are correct.\n");
			free(cfg_dir);
			cfg->err = 1;
		}
	}
	closedir(dir);
	const char *file = "/config";
	char *cfg_file = malloc(strlen(cfg_dir) + strlen(file) + 1);
	strcpy(cfg_file, cfg_dir);
	strcat(cfg_file, file);
	free(cfg_dir);
	cfg->cfg_file = cfg_file;
	cfg->err = 0;
}

char *get_default_lib() {
	char *home = getenv("HOME"); 
	const char *library = "/Music";
	char *path = malloc(strlen(home) + strlen(library) + 1);
	strcpy(path, home);
	strcat(path, library);
	return path;
}

void key_default(struct vmn_config *cfg) {
	cfg->keys = malloc(sizeof(int)*cfg->key_count);
	cfg->keys[BEGINNING] = 'g';
	cfg->keys[COMMAND] = ':';
	cfg->keys[END] = 'G';
	cfg->keys[ESCAPE] = CTRL('[');
	cfg->keys[MOVE_BACKWARD] = 'h';
	cfg->keys[MOVE_DOWN] = 'j';
	cfg->keys[MOVE_FORWARD] = 'l';
	cfg->keys[MOVE_UP] = 'k';
	cfg->keys[MUTE] = 'm';
	cfg->keys[MPV_KILL] = 'Q';
	cfg->keys[PAGE_DOWN] = CTRL('f');
	cfg->keys[PAGE_UP] = CTRL('b');
	cfg->keys[PLAYNEXT] = '>';
	cfg->keys[PLAYPAUSE] = ' ';
	cfg->keys[PLAYPREV] = '<';
	cfg->keys[QUEUE] = 'i';
	cfg->keys[QUEUE_ALL] = 'y';
	cfg->keys[QUEUE_CLEAR] = 'u';
	cfg->keys[SEARCH] = '/';
	cfg->keys[SEARCH_NEXT] = 'n';
	cfg->keys[SEARCH_PREV] = 'N';
	cfg->keys[START] = 10;
	cfg->keys[VISUAL] = 'v';
	cfg->keys[VMN_QUIT] = 'q';
	cfg->keys[VMN_REFRESH] = 'a';
	cfg->keys[VOLDOWN] = '9';
	cfg->keys[VOLUP] = '0';
}

void mpv_cfg_add(struct vmn_config *cfg, char *opt, char *value) {
	int pos;
	int match = 0;
	for (int i = 0; i < cfg->mpv_opts_len; ++i) {
		if (strcmp(opt, cfg->mpv_opts[i]) == 0) {
			match = 1;
			pos = i;
			break;
		}
	}
	if (match) {
		free(cfg->mpv_opts[pos+1]);
		cfg->mpv_opts[pos+1] = strdup(value);
	} else {
		cfg->mpv_opts_len += 2;
		cfg->mpv_opts = (char **)realloc(cfg->mpv_opts, sizeof(char*)*cfg->mpv_opts_len);
		cfg->mpv_opts[cfg->mpv_opts_len-2] = strdup(opt);
		cfg->mpv_opts[cfg->mpv_opts_len-1] = strdup(value);
	}
}

void mpv_set_opts(mpv_handle *ctx, struct vmn_config *cfg) {
	for (int i = 0; i < cfg->mpv_opts_len; i=i+2) {
		mpv_set_option_string(ctx, cfg->mpv_opts[i], cfg->mpv_opts[i+1]);
	}
}

char **parse_arg(char *arg) {
	char *str = remove_spaces(arg);
	int len = char_count(str, ',');
	char **arr = (char **)calloc(len+1, sizeof(char*));
	char *token = strtok(str, ",");
	int i = 0;
	while (token != NULL) {
		arr[i] = malloc(strlen(token) + 1);
		strcpy(arr[i], token);
		token = strtok(NULL, ",");
		++i;
	}
	free(str);
	return arr;
}

int parse_modifier(char *key) {
	int parsed_key = 0;
	char *str = strdup(key);
	char *token = strtok(str, "+");
	while (token != NULL) {
		if (!(strcmp(token, "Ctrl") == 0)) {
			int macro = check_macro(token);
			int func = check_func(token);
			if (macro || func) {
				printf("Ctrl modifier does not work with ncurses macros. Resetting keybind to default.\n");
				return 0;
			} else {
				parsed_key = atoi(token);
				if (!parsed_key) {
					parsed_key = token[0];
				}
			}
			break;
		}
		token = strtok(NULL, "+");
	}
	free(str);
	return parsed_key;
}


char *read_arg(char *arg) {
	char *out = strtok(arg, "=");
	out = strtok(NULL, "");
	return out;
}

void read_cfg_file(struct vmn_config *cfg) {
	FILE *file = fopen(cfg->cfg_file, "r");
	if (!file) {
		return;
	}
	int file_len = 0;
	char c;
	while ((c = fgetc(file)) != EOF) {
		if (c == '\n') {
			++file_len;
		}
	}
	rewind(file);
	char *cur = (char *)calloc(4096, sizeof(char));
	for (int i = 0; i < file_len; ++i) {
		if (fgets(cur, 4096, file) == NULL) {
			cfg->err = 1;
			printf("An error occured while trying to read the config file. Make sure your permissions are correct.\n");
		}
		if (cur[0] == '#') {
			continue;
		}
		struct char_split split = line_split(cur, "=");
		set_option(cfg, split.arr[0], split.arr[1]);
		char_split_destroy(&split);
	}
	free(cur);
	fclose(file);
}

int read_cfg_key(char *value) {
	int macro = check_macro(value);
	if (macro) {
		return macro;
	}
	int func = check_func(value);
	if (func) {
		return func;
	}
	regex_t regex;
	regcomp(&regex, "Ctrl", 0);
	int ctrl = regexec(&regex, value, 0, NULL, 0);
	if (ctrl == 0) {
		regfree(&regex);
		return CTRL(parse_modifier(value));
	}
	regfree(&regex);
	if (atoi(value)) {
		return atoi(value);
	} else {
		return value[0];
	}
}

char *read_dir_arg(char *arg) {
	char *path;
	char *tmp;
	char *out = strtok(arg, "=");
	out = strtok(NULL, "");
	if ((out[0] == '~') && (out[1] == '/')) {
		char *out_shift = out + 1;
		char *home = getenv("HOME");
		tmp = malloc(strlen(home) + strlen(out_shift) + 1);
		strcpy(tmp, home);
		strcat(tmp, out_shift);
	} else {
		tmp = strdup(out);
	}
	path = realpath(tmp, NULL);
	free(tmp);
	if (!path) {
		return "";
	} else {
		return path;
	}
}

void set_option(struct vmn_config *cfg, char *opt, char *value) {
	int ret = 0;
	ret = set_option_vmn(cfg, opt, value);
	if (ret) {
		return;
	}
	ret = set_option_key(cfg, opt, value);
	if (ret) {
		return;
	}
	ret = set_option_mpv(cfg, opt, value);
}

int set_option_key(struct vmn_config *cfg, char *opt, char *value) {
	const char *key_arr[] = {"beginning", "command", "end", "escape", "move-backward",
		"move-down", "move-forward", "move-up", "mpv-kill", "mute", "page-down",
		"page-up", "playnext", "playpause", "playprev", "queue", "queue-all",
		"queue-clear", "search", "search-next", "search-prev", "start", "visual",
		"vmn-quit", "vmn-refresh", "voldown", "volup"};

	int key;
	for (int i = 0; i < cfg->key_count; ++i) {
		if (strcmp(opt, key_arr[i]) == 0) {
			if ((key = read_cfg_key(value))) {
				cfg->keys[i] = key;
			}
			return 1;
		}
	}
	return 0;
}

int set_option_mpv(struct vmn_config *cfg, char *opt, char *value) {
	mpv_handle *test_ctx = mpv_create();
	int mpv_err = mpv_set_option_string(test_ctx, opt, NULL);
	if (mpv_err != -5) {
		mpv_cfg_add(cfg, opt, value);
	}
	mpv_terminate_destroy(test_ctx);
	return 0;
}

int set_option_vmn(struct vmn_config *cfg, char *opt, char *value) {
	char *opt_arr[6] = {"headless", "input-mode", "library", "tags", "sort", "view"};
	if (strcmp(opt, opt_arr[0]) == 0) {
		if (strcmp(value, "yes") == 0) {
			mpv_cfg_add(cfg, "force-window", "no");
			mpv_cfg_add(cfg, "video", "no");
			mpv_cfg_add(cfg, "osc", "no");
		} else if (strcmp(value, "no") == 0) {
		} else {
			printf("headless can be only set to 'yes' or 'no'\n");
		}
		return 1;
	} else if (strcmp(opt, opt_arr[1]) == 0) {
		if ((strcmp(value, "yes") == 0) || strcmp(value, "no") == 0) {
			cfg->input_mode = strdup(value);
		} else {
			printf("input-mode can only be set to 'yes' or 'no'\n");
		}
		return 1;
	} else if (strcmp(opt, opt_arr[2]) == 0) {
		char *library;
		char *tmp;
		if ((value[0] == '~') && (value[1] == '/')) {
			char *shift = strdup(value) + 1;
			char *home = getenv("HOME");
			tmp = malloc(strlen(home) + strlen(shift) + 1);
			strcpy(tmp, home);
			strcat(tmp, shift);
		} else {
			tmp = strdup(value);
		}
		library = realpath(tmp, NULL);
		free(tmp);
		if (!library) {
			printf("Library directory not found. Falling back to default.\n");
		} else {
			free(cfg->lib_dir);
			cfg->lib_dir = strdup(library);
			free(library);
		}
		return 1;
	} else if (strcmp(opt, opt_arr[3]) == 0) {
		if (strcmp(value, "") == 0) {
			printf("No tags specified. Falling back to default.\n");
		} else {
			char *len_check = strdup(value);
			char *token = strtok(len_check, ",");
			int i = 0;
			while (token != NULL) {
				token = strtok(NULL, ",");
				++i;
			}
			for (int j = 0; j < cfg->tags_len; ++j) {
				free(cfg->tags[j]);
			}
			free(cfg->tags);
			cfg->tags_len = i;
			cfg->tags = parse_arg(value);
			free(cfg->sort);
			cfg->sort = (enum vmn_config_sort *)calloc(cfg->tags_len, sizeof(enum vmn_config_sort));
			for (int j = 0; j < cfg->tags_len; ++j) {
				cfg->sort[j] = default_sort(cfg->tags[j]);
			}
			free(len_check);
		}
		return 1;
	} else if (strcmp(opt, opt_arr[4]) == 0) {
		char *len_check = strdup(value);
		char *token = strtok(len_check, ",");
		int valid = check_sort(token);
		int i = 0;
		while (token != NULL) {
			if (!valid) {
				break;
			}
			valid = check_sort(token);
			token = strtok(NULL, ",");
			++i;
		}
		free(len_check);
		if (!valid) {
			printf("Invalid sort argument specified. Resetting to default. \n");
			free(cfg->sort);
			cfg->sort = (enum vmn_config_sort *)calloc(cfg->tags_len, sizeof(enum vmn_config_sort));
			for (int j = 0; j < cfg->tags_len; ++j) {
				cfg->sort[j] = default_sort(cfg->tags[j]);
			}
		} else if (i != cfg->tags_len) {
			printf("The length of the sort argument must be exactly equal to the length of the tags argument. Resetting to default. \n");
			free(cfg->sort);
			cfg->sort = (enum vmn_config_sort *)calloc(cfg->tags_len, sizeof(enum vmn_config_sort));
			for (int j = 0; j < cfg->tags_len; ++j) {
				cfg->sort[j] = default_sort(cfg->tags[j]);
			}
		} else {
			free(cfg->sort);
			char **sort_arr = parse_arg(value);
			cfg->sort = (enum vmn_config_sort *)calloc(cfg->tags_len, sizeof(enum vmn_config_sort));
			for (int j = 0; j < cfg->tags_len; ++j) {
				if (strcmp(sort_arr[i], "metadata") == 0) {
					cfg->sort[j] = S_DATA;
				} else if (strcmp(sort_arr[j], "filename") == 0) {
					cfg->sort[j] = S_FILE;
				} else if (strcmp(sort_arr[j], "none") == 0) {
					cfg->sort[j] = S_NONE;
				} else if (strcmp(sort_arr[j], "tracknumber") == 0) {
					cfg->sort[j] = S_NUMB;
				} else if (strcmp(sort_arr[j], "random") == 0) {
					cfg->sort[j] = S_RAND;
				}
			}
			for (int j = 0; j < cfg->tags_len; ++j) {
				free(sort_arr[j]);
			}
			free(sort_arr);
		}
		return 1;
	} else if (strcmp(opt, opt_arr[5]) == 0) {
		if (strcmp(value, "file-path") == 0) {
			cfg->view = V_PATH;
		} else if (strcmp(value, "metadata") == 0) {
			cfg->view = V_DATA;
		} else if (strcmp(value, "song-only") == 0) {
			cfg->view = V_SONG;
		} else {
			cfg->view = V_DATA;
			printf("Invalid view specified. Falling back to default.\n");
		}
		return 1;
	}
	return 0;
}

void vmn_config_destroy(struct vmn_config *cfg) {
	free(cfg->cfg_file);
	free(cfg->input_mode);
	free(cfg->keys);
	free(cfg->lib_dir);
	for (int i = 0; i < cfg->mpv_opts_len; ++i) {
		free(cfg->mpv_opts[i]);
	}
	free(cfg->mpv_opts);
	if (cfg->view == V_DATA) {
		for (int i = 0; i < cfg->tags_len; ++i) {
			free(cfg->tags[i]);
		}
	}
	free(cfg->tags);
	free(cfg->sort);
}
