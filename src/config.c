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

int check_arg(struct vmn_config *cfg, char *arg) {
	char *valid[6] = {"", "--input-mode=", "--library=", "--sort=", "--tags=", "--view="};
	int len[6];
	for (int i = 0; i < 6; ++i) {
		len[i] = strlen(valid[i]);
	}
	for (int i = 1; i < 6; ++i) {
		if (strncmp(arg, valid[i], len[i]) == 0) {
			return i;
		}
	}
	return 0;
}

int check_func(char *func) {
	if (strcmp(func, "f1") == 0) {
		return KEY_F(1);
	} else if (strcmp(func, "f2") == 0) {
		return KEY_F(2);
	} else if (strcmp(func, "f3") == 0) {
		return KEY_F(3);
	} else if (strcmp(func, "f4") == 0) {
		return KEY_F(4);
	} else if (strcmp(func, "f5") == 0) {
		return KEY_F(5);
	} else if (strcmp(func, "f6") == 0) {
		return KEY_F(6);
	} else if (strcmp(func, "f7") == 0) {
		return KEY_F(7);
	} else if (strcmp(func, "f8") == 0) {
		return KEY_F(8);
	} else if (strcmp(func, "f9") == 0) {
		return KEY_F(9);
	} else if (strcmp(func, "f10") == 0) {
		return KEY_F(10);
	} else if (strcmp(func, "f11") == 0) {
		return KEY_F(11);
	} else if (strcmp(func, "f12") == 0) {
		return KEY_F(12);
	} else {
		return 0;
	}
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
	if (strcmp(str, "metadata") == 0) {
		return 1;
	} else if (strcmp(str, "filename") == 0) {
		return 1;
	} else if (strcmp(str, "tracknumber") == 0) {
		return 1;
	} else if (strcmp(str, "random") == 0) {
		return 1;
	} else {
		return 0;
	}
}

enum vmn_config_sort default_sort(char *tags) {
	if (strcasecmp(tags, "artist") == 0) {
		return S_DATA;
	} else if (strcasecmp(tags, "album") == 0) {
		return S_DATA;
	} else if (strcasecmp(tags, "album_artist") == 0 ) {
		return S_DATA;
	} else if (strcasecmp(tags, "date") == 0) {
		return S_DATA;
	} else if (strcasecmp(tags, "title") == 0) {
		return S_NUMB;
	} else {
		return S_DATA;
	}
}

char *get_cfg() {
	char *home = getenv("HOME"); 
	const char *cfg = "/.config/vmn/config";
	char *path = malloc(strlen(home) + strlen(cfg) + 1);
	strcpy(path, home);
	strcat(path, cfg);
	return path;
}

char *get_cfg_dir() {
	char *home = getenv("HOME");
	const char *cfgdir = "/.config/vmn";
	char *path = malloc(strlen(home) + strlen(cfgdir) + 1);
	strcpy(path, home);
	strcat(path, cfgdir);
	return path;
}

char *get_default_lib() {
	char *home = getenv("HOME"); 
	const char *library = "/Music";
	char *path = malloc(strlen(home) + strlen(library) + 1);
	strcpy(path, home);
	strcat(path, library);
	return path;
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

char *read_arg(char *arg) {
	char *out = strtok(arg, "=");
	out = strtok(NULL, "");
	return out;
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
		if (!strcmp(token, "Ctrl") == 0) {
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

int read_cfg_key(char *opt) {
	int macro = check_macro(opt);
	if (macro) {
		return macro;
	}
	int func = check_func(opt);
	if (func) {
		return func;
	}
	regex_t regex;
	regcomp(&regex, "Ctrl", 0);
	int ctrl = regexec(&regex, opt, 0, NULL, 0);
	if (ctrl == 0) {
		regfree(&regex);
		return CTRL(parse_modifier(opt));
	}
	regfree(&regex);
	if (atoi(opt)) {
		return atoi(opt);
	} else {
		return opt[0];
	}
}

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

void vmn_set_option(struct vmn_config *cfg, char *opt, char *value) {
	char *opt_arr[5] = {"input-mode", "library", "tags", "sort", "view"};
	char *key_arr[27] = {"beginning", "command", "end", "escape", "move-backward",
		"move-down", "move-forward", "move-up", "mpv-kill", "mute", "page-down",
		"page-up", "playnext", "playpause", "playprev", "queue", "queue-all",
		"queue-clear", "search", "search-next", "search-prev", "start", "visual",
		"vmn-quit", "vmn-refresh", "voldown", "volup"};

	if (strcmp(opt, opt_arr[0]) == 0) {
		if ((strcmp(value, "yes") == 0) || strcmp(value, "no") == 0) {
			cfg->input_mode = strdup(value);
		} else {
			printf("input-mode can only be set to 'yes' or 'no'\n");
		}
	} else if (strcmp(opt, opt_arr[1]) == 0) {
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
	} else if (strcmp(opt, opt_arr[2]) == 0) {
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
			free(len_check);
		}
	} else if (strcmp(opt, opt_arr[3]) == 0) {
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
	} else if (strcmp(opt, opt_arr[4]) == 0) {
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
	}

	int key;
	if (strcmp(opt, key_arr[0]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.beginning = key;
		}
	} else if (strcmp(opt, key_arr[1]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.command = key;
		}
	} else if (strcmp(opt, key_arr[2]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.end = key;
		}
	} else if (strcmp(opt, key_arr[3]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.escape = key;
		}
	} else if (strcmp(opt, key_arr[4]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.move_backward = key;
		}
	} else if (strcmp(opt, key_arr[5]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.move_down = key;
		}
	} else if (strcmp(opt, key_arr[6]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.move_forward = key;
		}
	} else if (strcmp(opt, key_arr[7]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.move_up = key;
		}
	} else if (strcmp(opt, key_arr[8]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.mpv_kill = key;
		}
	} else if (strcmp(opt, key_arr[9]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.mute = key;
		}
	} else if (strcmp(opt, key_arr[10]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.page_down = key;
		}
	} else if (strcmp(opt, key_arr[11]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.page_up = key;
		}
	} else if (strcmp(opt, key_arr[12]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.playnext = key;
		}
	} else if (strcmp(opt, key_arr[13]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.playpause = key;
		}
	} else if (strcmp(opt, key_arr[14]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.playprev = key;
		}
	} else if (strcmp(opt, key_arr[15]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.queue = key;
		}
	} else if (strcmp(opt, key_arr[16]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.queue_all = key;
		}
	} else if (strcmp(opt, key_arr[17]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.queue_clear = key;
		}
	} else if (strcmp(opt, key_arr[18]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.search = key;
		}
	} else if (strcmp(opt, key_arr[19]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.search_next = key;
		}
	} else if (strcmp(opt, key_arr[20]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.search_prev = key;
		}
	} else if (strcmp(opt, key_arr[21]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.start = key;
		}
	} else if (strcmp(opt, key_arr[22]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.visual = key;
		}
	} else if (strcmp(opt, key_arr[23]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.vmn_quit = key;
		}
	} else if (strcmp(opt, key_arr[24]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.vmn_refresh = key;
		}
	} else if (strcmp(opt, key_arr[25]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.voldown = key;
		}
	} else if (strcmp(opt, key_arr[26]) == 0) {
		if ((key = read_cfg_key(value))) {
			cfg->key.volup = key;
		}
	}

	mpv_handle *test_ctx = mpv_create();
	int mpv_err = mpv_set_option_string(test_ctx, opt, NULL);
	if (mpv_err != -5) {
		mpv_cfg_add(cfg, opt, value);
	}
	mpv_destroy(test_ctx);
}

void read_cfg_file(struct vmn_config *cfg, char *cfg_file) {
	FILE *file = fopen(cfg_file, "r");
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
		fgets(cur, 4096, file);
		if (cur[0] == '#') {
			continue;
		}
		char **split = cfg_split(cur);
		vmn_set_option(cfg, split[0], split[1]);
		free(split[0]);
		free(split[1]);
		free(split);
	}
	free(cur);
	fclose(file);
}

struct vmn_key key_default() {
	struct vmn_key key;
	key.beginning = 'g';
	key.command = ':';
	key.end = 'G';
	key.escape = CTRL('[');
	key.move_backward = 'h';
	key.move_down = 'j';
	key.move_forward = 'l';
	key.move_up = 'k';
	key.mute = 'm';
	key.mpv_kill = 'Q';
	key.page_down = CTRL('f');
	key.page_up = CTRL('b');
	key.playnext = '>';
	key.playpause = ' ';
	key.playprev = '<';
	key.queue = 'i';
	key.queue_all = 'y';
	key.queue_clear = 'u';
	key.search = '/';
	key.search_next = 'n';
	key.search_prev = 'N';
	key.start = 10;
	key.visual = 'v';
	key.vmn_quit = 'q';
	key.vmn_refresh = 'a';
	key.voldown = '9';
	key.volup = '0';
	return key;
}

struct vmn_config cfg_init(int argc, char *argv[]) {
	struct vmn_config cfg;
	cfg_default(&cfg);
	cfg.key = key_default();
	char *cfg_dir = get_cfg_dir();
	DIR *dir = opendir(cfg_dir);
	if (!dir) {
		int err = mkdir(cfg_dir, 0755);
		free(cfg_dir);
		if (err) {
			printf("An error occured while trying to create the config directory. Make sure your permissions are correct.\n");
			cfg.err = 1;
			return cfg;
		}
	} else {
		closedir(dir);
		free(cfg_dir);
	}
	char *cfg_file = get_cfg();
	read_cfg_file(&cfg, cfg_file);

	char *input;
	char *library;
	char *sort;
	char *tags;
	char *viewcfg;
	int pos[argc];
	int input_arg = 0;
	int lib_arg = 0;
	int tags_arg = 0;
	int sort_arg = 0;
	int view_arg = 0;
	
	//check for any command line arguments
	//these take priority over any config file options
	for (int i = 1; i < argc; ++i) {
		pos[i] = check_arg(&cfg, argv[i]);
	}

	for (int i = 1; i < argc; ++i) {
		if (pos[i] == 1) {
			input_arg = i;
		}
		if (pos[i] == 2) {
			lib_arg = i;
		}
		if (pos[i] == 3) {
			sort_arg = i;
		}
		if (pos[i] == 4) {
			tags_arg = i;
		}
		if (pos[i] == 5) {
			view_arg = i;
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

	free(cfg_file);
	return cfg;
}

void vmn_config_destroy(struct vmn_config *cfg) {
	free(cfg->input_mode);
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
