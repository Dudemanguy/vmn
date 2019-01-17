#include <ctype.h>
#include <dirent.h>
#include <libconfig.h>
#include <ncurses.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"

#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif


int check_arg(struct vmn_config *cfg, char *arg) {
	char *valid[8] = {"", "--input-mode=", "--library=", "--mpv-cfg=", "--mpv-cfg-dir=", "--sort=", "--tags=", "--view="};
	int i = 0;
	int status;
	for (i = 0; i < 8; ++i) {
		regex_t regex;
		regcomp(&regex, valid[i], 0);
		status = regexec(&regex, arg, 0, NULL, 0);
		if (status == 0) {
			if (i > 0) {
				regfree(&regex);
				return i;
				break;
			}
		}
		regfree(&regex);
	}
	return 0;
}

int check_cfg(char *cfg_file) {
	config_t cfg;
	config_init(&cfg);

	if (!config_read_file(&cfg, cfg_file)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}
	return 0;
}

void check_dir() {
	char *cfgdir = get_cfg_dir();
	struct stat st = {0};
	if (stat(cfgdir, &st) == -1) {
		mkdir(cfgdir, 0777);
	}
}

int check_func(const char *func) {
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

int check_macro(const char *macro) {
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
	} else if (strcmp(str, "track-number") == 0) {
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

char *read_arg(char *arg) {
	char *sep = "=";
	char *out = strtok(arg, sep);
	out = strtok(NULL, "");
	if (out) {
		return out;
	} else {
		return "";
	}
}

char **parse_arg(char *arg) {
	char *str = remove_spaces(arg);
	char *token = strtok(str, ",");
	int len = 10;
	char **arr = (char **)calloc(10, sizeof(char*));
	int i = 0;
	while (token != NULL) {
		if (i == len) {
			len = len*2;
			arr = (char **)realloc(arr,sizeof(char*)*len);
		}
		arr[i] = malloc(strlen(token) + 1);
		strcpy(arr[i], token);
		token = strtok(NULL, ",");
		++i;
	}
	arr[i] = '\0';
	arr = (char **)realloc(arr, sizeof(char*)*(i+1));
	free(str);
	return arr;
}

int parse_modifier(const char *key) {
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
				if (atoi(token)) {
					return atoi(token);
				} else {
					return token[0];
				}
			}
		}
		token = strtok(NULL, "+");
	}
	return 0;
}

int read_cfg_key(config_t *libcfg, const char *opt) {
	const char *key;
	if (!config_lookup_string(libcfg, opt, &key)) {
		return 0;
	} else {
		int macro = check_macro(key);
		if (macro) {
			return macro;
		}
		int func = check_func(key);
		if (func) {
			return func;
		}
		regex_t regex;
		regcomp(&regex, "Ctrl", 0);
		int ctrl = regexec(&regex, key, 0, NULL, 0);
		if (ctrl == 0) {
			return CTRL(parse_modifier(key));
		}
		regfree(&regex);
		if (atoi(key)) {
			return atoi(key);
		} else {
			return key[0];
		}
	}
}

char *read_cfg_str(config_t *libcfg, const char *opt) {
	const char *output;
	if (!config_lookup_string(libcfg, opt, &output)) {
		return "";
	}
	char *out = strdup(output);
	return out;
}

char *remove_spaces(char *str) {
	char *trim = (char *)calloc(strlen(str) + 1, sizeof(char));
	int i = 0;
	while (*str != '\0') {
		if (!isspace(*str)) {
			trim[i] = *str;
			++i;
		}
		++str;
	}
	trim[i] = '\0';
	trim = (char *)realloc(trim, sizeof(char)*(i+1));
	return trim;
}

struct vmn_key key_init(config_t *libcfg) {
	struct vmn_key key;
	key.beginning = read_cfg_key(libcfg, "beginning");
	if (!key.beginning) {
		key.beginning = 'g';
	}
	key.end = read_cfg_key(libcfg, "end");
	if (!key.end) {
		key.end = 'G';
	}
	key.move_backward = read_cfg_key(libcfg, "move-backward");
	if (!key.move_backward) {
		key.move_backward = 'h';
	}
	key.move_down = read_cfg_key(libcfg, "move-down");
	if (!key.move_down) {
		key.move_down = 'j';
	}
	key.move_forward = read_cfg_key(libcfg, "move-forward");
	if (!key.move_forward) {
		key.move_forward = 'l';
	}
	key.move_up = read_cfg_key(libcfg, "move-up");
	if (!key.move_up) {
		key.move_up = 'k';
	}
	key.mute = read_cfg_key(libcfg, "mute");
	if (!key.mute) {
		key.mute = 'm';
	}
	key.mpv_kill = read_cfg_key(libcfg, "mpv-kill");
	if (!key.mpv_kill) {
		key.mpv_kill = 'Q';
	}
	key.page_down = read_cfg_key(libcfg, "page-down");
	if (!key.page_down) {
		key.page_down = CTRL('f');
	}
	key.page_up = read_cfg_key(libcfg, "page-up");
	if (!key.page_up) {
		key.page_up = CTRL('b');
	}
	key.playnext = read_cfg_key(libcfg, "playnext");
	if (!key.playnext) {
		key.playnext = '>';
	}
	key.playpause = read_cfg_key(libcfg, "playpause");
	if (!key.playpause) {
		key.playpause = ' ';
	}
	key.playprev = read_cfg_key(libcfg, "playprev");
	if (!key.playprev) {
		key.playprev = '<';
	}
	key.queue = read_cfg_key(libcfg, "queue");
	if (!key.queue) {
		key.queue = 'i';
	}
	key.queue_all = read_cfg_key(libcfg, "queue-all");
	if (!key.queue_all) {
		key.queue_all = 'y';
	}
	key.queue_clear = read_cfg_key(libcfg, "queue-clear");
	if (!key.queue_clear) {
		key.queue_clear = 'u';
	}
	key.quit = read_cfg_key(libcfg, "quit");
	if (!key.quit) {
		key.quit = CTRL('[');
	}
	key.search = read_cfg_key(libcfg, "search");
	if (!key.search) {
		key.search = '/';
	}
	key.search_next = read_cfg_key(libcfg, "search-next");
	if (!key.search_next) {
		key.search_next = 'n';
	}
	key.search_prev = read_cfg_key(libcfg, "search-prev");
	if (!key.search_prev) {
		key.search_prev = 'N';
	}
	key.start = read_cfg_key(libcfg, "start");
	if (!key.start) {
		key.start = 10;
	}
	key.visual = read_cfg_key(libcfg, "visual");
	if (!key.visual) {
		key.visual = 'v';
	}
	key.vmn_quit = read_cfg_key(libcfg, "vmn-quit");
	if (!key.vmn_quit) {
		key.vmn_quit = 'q';
	}
	key.vmn_refresh = read_cfg_key(libcfg, "vmn-refresh");
	if (!key.vmn_refresh) {
		key.vmn_refresh = 'a';
	}
	key.voldown = read_cfg_key(libcfg, "voldown");
	if (!key.voldown) {
		key.voldown = '9';
	}
	key.volup = read_cfg_key(libcfg, "volup");
	if (!key.volup) {
		key.volup = '0';
	}
	return key;
}

struct vmn_config cfg_init(int argc, char *argv[]) {
	config_t libcfg;
	config_init(&libcfg);
	struct vmn_config cfg;
	char *cfg_file = get_cfg();
	config_read_file(&libcfg, cfg_file);
	cfg.key = key_init(&libcfg);
	const char *input;
	const char *library;
	const char *mpv_cfg;
	const char *mpv_cfg_dir;
	const char *sort;
	const char *tags;
	const char *viewcfg;
	int pos[7] = {0, 0, 0, 0, 0, 0, 0};
	int input_arg = 0;
	int lib_arg = 0;
	int mpv_arg = 0;
	int mpv_dir_arg = 0;
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
			mpv_arg = i;
		}
		if (pos[i] == 4) {
			mpv_dir_arg = i;
		}
		if (pos[i] == 5) {
			sort_arg = i;
		}
		if (pos[i] == 6) {
			tags_arg = i;
		}
		if (pos[i] == 7) {
			view_arg = i;
		}
	}

	if (input_arg) {
		input = read_arg(argv[input_arg]);
		if ((strcmp(input, "yes") == 0) || (strcmp(input, "no") == 0)) {
			cfg.input_mode = strdup(input);
		} else {
			cfg.input_mode = "yes";
		}
	} else {
		cfg.input_mode = read_cfg_str(&libcfg, "input-mode");
		if ((!strcmp(cfg.input_mode, "yes") == 0) && (!strcmp(cfg.input_mode, "no") == 0) &&
				(!strcmp(cfg.input_mode, "") == 0)) {
			printf("input-mode can only be set to 'yes' or 'no'\n");
			cfg.input_mode = "no";
		}
	}

	if (lib_arg) {
		library = read_arg(argv[lib_arg]);
		DIR *dir = opendir(library);
		if (dir) {
			cfg.lib_dir = strdup(library);
		} else {
			cfg.lib_dir = get_default_lib();
			printf("Library directory not found. Falling back to default.\n");
		}
		closedir(dir);
	} else {
		cfg.lib_dir = read_cfg_str(&libcfg, "library");
		if (strcmp(cfg.lib_dir, "") == 0) {
			cfg.lib_dir = get_default_lib();
		} else {
			DIR *dir = opendir(cfg.lib_dir);
			if (!dir) {
				cfg.lib_dir = get_default_lib();
				printf("Library directory not found. Falling back to default.\n");
			}
			closedir(dir);
		}
	}

	if (mpv_arg) {
		mpv_cfg = read_arg(argv[mpv_arg]);
		if ((strcmp(mpv_cfg, "yes") == 0) || (strcmp(mpv_cfg, "no") == 0)) {
			cfg.mpv_cfg = strdup(mpv_cfg);
		} else {
			cfg.mpv_cfg = "yes";
		}
	} else {
		cfg.mpv_cfg = read_cfg_str(&libcfg, "mpv-cfg");
		if ((!strcmp(cfg.mpv_cfg, "yes") == 0) && (!strcmp(cfg.mpv_cfg, "no") == 0) &&
				(!strcmp(cfg.mpv_cfg, "") == 0)) {
			printf("mpv-cfg can only be set to 'yes' or 'no'\n");
			cfg.mpv_cfg = "yes";
		}
	}

	if (mpv_dir_arg) {
		mpv_cfg_dir = read_arg(argv[mpv_dir_arg]);
		DIR *dir = opendir(mpv_cfg_dir);
		if (dir) {
			cfg.mpv_cfg_dir = strdup(mpv_cfg_dir);
		} else {
			cfg.mpv_cfg_dir = get_cfg_dir();
			printf("Mpv config directory not found. Falling back to default.\n");
		}
		closedir(dir);
	} else {
		cfg.mpv_cfg_dir = read_cfg_str(&libcfg, "mpv-cfg-dir");
		if (strcmp(cfg.mpv_cfg_dir, "") == 0) {
			cfg.mpv_cfg_dir = get_cfg_dir();
		} else {
			DIR *dir = opendir(cfg.mpv_cfg_dir);
			if (!dir) {
				cfg.mpv_cfg_dir = get_cfg_dir();
				printf("mpv config directory not found. Falling back to default.\n");
			}
			closedir(dir);
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
			cfg.view = V_PATH;
			printf("Invalid view specified. Falling back to default.\n");
		}
	} else {
		if (!config_lookup_string(&libcfg, "view", &viewcfg)) {
			cfg.view = V_PATH;
		} else {
			if (strcmp(viewcfg, "file-path") == 0) {
				cfg.view = V_PATH;
			} else if (strcmp(viewcfg, "metadata") == 0) {
				cfg.view = V_DATA;
			} else if (strcmp(viewcfg, "song-only") == 0) {
				cfg.view = V_SONG;
			} else {
				cfg.view = V_PATH;
				printf("Invalid view specified. Falling back to default.\n");
			}
		}
	}

	if (cfg.view == V_DATA) {
		if (tags_arg) {
			tags = read_arg(argv[tags_arg]);
			char *tag_clone = strdup(tags);
			char *len_check = strdup(tags);
			char *token = strtok(len_check, ",");
			int j = 0;
			while (token != NULL) {
				token = strtok(NULL, ",");
				++j;
			}
			cfg.tags_len = j;
			cfg.tags = parse_arg(tag_clone);
			free(tag_clone);
			free(len_check);
		} else {
			char *valid_tags = read_cfg_str(&libcfg, "tags");
			if (strcmp(valid_tags, "") == 0) {
				char *default_tags = "artist,album,title";
				cfg.tags = parse_arg(default_tags);
				cfg.tags_len = 3;
			} else {
				cfg.tags = parse_arg(valid_tags);
			}
		}

		if (sort_arg) {
			sort = read_arg(argv[sort_arg]);
			char *sort_clone = strdup(sort);
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
			cfg.sort_len = j;
			free(len_check);
			if (!valid) {
				printf("Invalid sort argument specified. Resetting to default. \n");
				free(sort_clone);
				sort_arg = 0;
			} else {
				if (tags_arg) {
					if (cfg.tags_len != cfg.sort_len) {
						printf("The length of the sort argument must be exactly equal to the length of the tags argument. Resetting to default. \n");
						sort_arg = 0;
					} else {
						char **sort_arr = parse_arg(sort_clone);
						cfg.sort = (enum vmn_config_sort *)calloc(cfg.tags_len, sizeof(enum vmn_config_sort));
						for (int k = 0; k < cfg.tags_len; ++k) {
							if (strcmp(sort_arr[k], "metadata") == 0) {
								cfg.sort[k] = S_DATA;
							} else if (strcmp(sort_arr[k], "filename") == 0) {
								cfg.sort[k] = S_FILE;
							} else if (strcmp(sort_arr[k], "none") == 0) {
								cfg.sort[k] = S_NONE;
							} else if (strcmp(sort_arr[k], "track-number") == 0) {
								cfg.sort[k] = S_NUMB;
							} else if (strcmp(sort_arr[k], "random") == 0) {
								cfg.sort[k] = S_RAND;
							}
						}
						for (int k = 0; k < cfg.sort_len; ++k) {
							free(sort_arr[k]);
						}
						free(sort_arr);
					}
				} else {
					if (cfg.sort_len != 3) {
						printf("The length of the sort argument must be exactly equal to the default length of tags (3). Resetting to default. \n");
						sort_arg = 0;
					} else {
						char **sort_arr = parse_arg(sort_clone);
						cfg.sort = (enum vmn_config_sort *)calloc(cfg.tags_len, sizeof(enum vmn_config_sort));
						for (int k = 0; k < cfg.tags_len; ++k) {
							if (strcmp(sort_arr[k], "metadata") == 0) {
								cfg.sort[k] = S_DATA;
							} else if (strcmp(sort_arr[k], "filename") == 0) {
								cfg.sort[k] = S_FILE;
							} else if (strcmp(sort_arr[k], "none") == 0) {
								cfg.sort[k] = S_NONE;
							} else if (strcmp(sort_arr[k], "track-number") == 0) {
								cfg.sort[k] = S_NUMB;
							} else if (strcmp(sort_arr[k], "random") == 0) {
								cfg.sort[k] = S_RAND;
							}
						}
						for (int k = 0; k < cfg.sort_len; ++k) {
							free(sort_arr[k]);
						}
						free(sort_arr);
					}
				}
				free(sort_clone);
			}
		} else {
			char *valid_sort = read_cfg_str(&libcfg, "sort");
			if (strcmp(valid_sort, "") == 0) {
				cfg.sort = (enum vmn_config_sort *)calloc(cfg.tags_len, sizeof(enum vmn_config_sort));
				cfg.sort_len = cfg.tags_len;
				for (int i = 0; i < cfg.tags_len; ++i) {
					cfg.sort[i] = default_sort(cfg.tags[i]);
				}
			}
		}
	}

	cfg.select = 0;
	cfg.select_pos = 0;

	free(cfg_file);
	config_destroy(&libcfg);
	return cfg;
}

void vmn_config_destroy(struct vmn_config *cfg) {
	free(cfg->lib_dir);
	free(cfg->mpv_cfg_dir);
	if (cfg->view == V_DATA) {
		for (int i = 0; i < cfg->tags_len; ++i) {
			free(cfg->tags[i]);
		}
		free(cfg->tags);
		free(cfg->sort);
	}
}
