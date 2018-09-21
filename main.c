#include <dirent.h>
#include <libconfig.h>
#include <locale.h>
#include <math.h>
#include <menu.h>
#include <mpv/client.h>
#include <ncurses.h>
#include <regex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"
#include "library.h"

#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

ITEM **create_items(char ***files);
int directory_count(const char *path);
int ext_valid(char *ext);
char *get_file_ext(const char *file);
char ***get_lib_dir(const char *library, struct vmn_library *lib);
ITEM **get_lib_items(struct vmn_library *lib);
void get_music_files(const char *library, struct vmn_library *lib);
int key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib);
int move_menu_backward(const char *path, struct vmn_config *cfg, struct vmn_library *lib);
int move_menu_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib);
mpv_handle *mpv_generate(struct vmn_config *cfg);
void mpv_queue(mpv_handle *ctx, const char *audio);
int mpv_wait(mpv_handle *ctx, int len, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib);
int path_in_lib(char *path, struct vmn_library *lib);
int qstrcmp(const void *a, const void *b);

int main() {
	setlocale(LC_CTYPE, "");
	struct vmn_config cfg = cfg_init();
	struct vmn_library lib = lib_init();
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	get_music_files(cfg.lib_dir, &lib);
	char ***root = get_lib_dir(cfg.lib_dir, &lib);
	lib.items = (ITEM ***)calloc(1, sizeof(ITEM **));
	lib.items[0] = create_items(root);
	lib.menu = (MENU **)calloc(1, sizeof(MENU *));
	lib.menu[0] = new_menu((ITEM **)lib.items[0]);
	set_menu_format(lib.menu[0], LINES, 0);
	menu_opts_off(lib.menu[0], O_ONEVALUE);
	menu_opts_off(lib.menu[0], O_SHOWDESC);
	WINDOW *win = newwin(0, 0, 0, 0);
	keypad(win, TRUE);
	set_menu_win(lib.menu[0], win);
	set_menu_sub(lib.menu[0], win);
	post_menu(lib.menu[0]);
	wrefresh(win);
	int c;
	int exit = 0;
	while (!exit) {
		c = wgetch(win);
		MENU *menu = lib.menu[lib.depth];
		ITEM **items = lib.items[lib.depth];
		exit = key_event(c, menu, items, &cfg, &lib);
	}
	//TODO: free memory properly
	//free(lib.items);
	//free(lib.menu);
	/*int i = 0;
	for (int i = 0; i < 4; ++i) {
		int length = item_count(lib.menu[i]);
		unpost_menu(lib.menu[i]);
		free_menu(lib.menu[i]);
		for (int j = 0; j < length; ++i) {
			free_item(lib.items[i][j]);
		}
		free(lib.items[i]);
	}*/
	vmn_config_destroy(&cfg);
	vmn_library_destroy(&lib);
	endwin();
	return 0;
}

ITEM **create_items(char ***files) {
	ITEM **items;
	int n = 0;
	while (files[0][n]) {
		++n;
	}
	items = (ITEM **)calloc(n, sizeof(ITEM *));
	int i;
	for (i = 0; i < n; ++i) {
		items[i] = new_item(files[0][i], files[1][i]);
	}
	items[i] = (ITEM *)NULL;
	return items;
}

int directory_count(const char *path) {
	const char *where = path;
	int i = 0;
	while ((where = strchr(where, '/'))) {
		++i;
		++where;
	};
	return i;
}

int ext_valid(char *ext) {
	const char *file_type[] = { "aac", "aiff", "alac", "ape", "flac", "m4a", "mp3", "ogg", "opus", "wav" };
	int len = 10;
	for (int i = 0; i < len; ++i) {
		if (strcmp(file_type[i], ext) == 0) {
			return 1;
		}
	}
	return 0;
}

char *get_file_ext(const char *file) {
	char *dot = strrchr(file, '.');
	if (!dot || dot == file) {
		return "";
	}
	return dot + 1;
}

ITEM **get_lib_items(struct vmn_library *lib) {
	ITEM **items;
	qsort(lib->files, lib->length, sizeof(char *), qstrcmp);
	items = (ITEM **)calloc(lib->length + 1, sizeof(ITEM *));
	for (int i = 0; i < lib->length; ++i) {
		items[i] = new_item(lib->files[i], NULL);
	}
	items[lib->length] = (ITEM *)NULL;
	return items;
}

char ***get_lib_dir(const char *library, struct vmn_library *lib) {
	struct dirent *dp;
	DIR *dir = opendir(library);
	int max_line_len = 1024;
	int lines_allocated = 1000;

	if (!dir) {
		return 0;
	}

	char ***dir_info = (char ***)malloc(sizeof(char **)*2);
	dir_info[0] = (char **)malloc(sizeof(char *)*lines_allocated);
	dir_info[1] = (char **)malloc(sizeof(char *)*lines_allocated);

	int i = 0;
	while ((dp = readdir(dir)) != NULL) {
		if (i >= lines_allocated) {
			int new_size;
			new_size = lines_allocated*2;
			dir_info[0] = (char **)realloc(dir_info[0],sizeof(char *)*new_size);
			dir_info[1] = (char **)realloc(dir_info[1],sizeof(char *)*new_size);
			lines_allocated = new_size;
		}
		dir_info[0][i] = malloc(max_line_len);
		dir_info[1][i] = malloc(max_line_len);
		char path[1024];
		strcpy(path, library);
		strcat(path, "/");
		strcat(path, dp->d_name);
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
			if (dp->d_type == DT_DIR && path_in_lib(path, lib)) {
				strcpy(dir_info[0][i], dp->d_name);
				strcpy(dir_info[1][i], path);
				++i;
			}
			if (dp->d_type == DT_REG) {
				char *ext = get_file_ext(dp->d_name);
				if (ext_valid(ext)) {
					strcpy(dir_info[0][i], dp->d_name);
					strcpy(dir_info[1][i], path);
					++i;
				}
			}
		}
	}
	
	dir_info[0][i] = '\0';
	dir_info[1][i] = '\0';
	closedir(dir);
	qsort(dir_info[0], i, sizeof(char *), qstrcmp);
	qsort(dir_info[1], i, sizeof(char *), qstrcmp);
	return dir_info;
}

void get_music_files(const char *library, struct vmn_library *lib) {
	struct dirent *dp;
	DIR *dir = opendir(library);

	if (!dir) {
		return;
	}

	while ((dp = readdir(dir)) != NULL) {
		char path[1024];
		strcpy(path, library);
		strcat(path, "/");
		strcat(path, dp->d_name);
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
			if (dp->d_type == DT_DIR) {
				get_music_files(path, lib);
			} else {
				char *ext = get_file_ext(dp->d_name);
				if (ext_valid(ext)) {
					vmn_library_add(lib, path);
				}
			}
		}
	}
	closedir(dir);
}

int key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib) {
	int init_pos;
	int end_pos;
	int exit;
	ITEM *cur;
	const char *path;
	mpv_handle *ctx = mpv_generate(cfg);

	switch(c) {
	case 'i':
	case ' ':
		cur = current_item(menu);
		menu_driver(menu, REQ_TOGGLE_ITEM);
		exit = 0;
		break;
	case 'u':
		for (int i = 0; i < item_count(menu); ++i) {
			if (item_value(items[i])) {
				set_item_value(items[i], false);
			}
		}
		cfg->select = 0;
		exit = 0;
		break;
	case 'y':
		for (int i = 0; i < item_count(menu); ++i) {
			if (!item_value(items[i])) {
				set_item_value(items[i], true);
			}
		}
		exit = 0;
		break;
	case 'v':
		if (cfg->select) {
			cfg->select = 0;
			cfg->select_pos = 0;
			set_item_value(items[cfg->select_pos], false);
		} else {
			cfg->select = 1;
			cur = current_item(menu);
			set_item_value(cur, true);
			cfg->select_pos = item_index(cur);
		}
		exit = 0;
		break;
	case 'k':
	case KEY_UP:
		if (cfg->select) {
			cur = current_item(menu);
			int cur_pos = item_index(cur);
			if (cur_pos > cfg->select_pos) {
				set_item_value(cur, false);
			}
			if (cur_pos < cfg->select_pos) {
				set_item_value(cur, true);
			}
		}
		menu_driver(menu, REQ_PREV_ITEM);
		cur = current_item(menu);
		if (cfg->select) {
			cur = current_item(menu);
			set_item_value(cur, true);
		}
		exit = 0;
		break;
	case 'j':
	case KEY_DOWN:
		if (cfg->select) {
			cur = current_item(menu);
			int cur_pos = item_index(cur);
			if (cur_pos > cfg->select_pos) {
				set_item_value(cur, true);
			}
			if (cur_pos < cfg->select_pos) {
				set_item_value(cur, false);
			}
		}
		menu_driver(menu, REQ_NEXT_ITEM);
		if (cfg->select) {
			cur = current_item(menu);
			set_item_value(cur, true);
		}
		exit = 0;
		break;
	case 'l':
	case KEY_RIGHT:
		cur = current_item(menu);
		path = item_description(cur);
		move_menu_forward(path, cfg, lib);
		exit = 0;
		break;
	case 'h':
	case KEY_LEFT:
		cur = current_item(menu);
		path = item_description(cur);
		move_menu_backward(path, cfg, lib);
		exit = 0;
		break;
	case 'g':
	case KEY_HOME:
		menu_driver(menu, REQ_FIRST_ITEM);
		if (cfg->select) {
			for (int i = 0; i < item_count(menu); ++i) {
				if (i > cfg->select_pos) {
					set_item_value(items[i], false);
				}
				if (i < cfg->select_pos) {
					set_item_value(items[i], true);
				}
			}
		}
		exit = 0;
		break;
	case 'G':
	case KEY_END:
		menu_driver(menu, REQ_LAST_ITEM);
		if (cfg->select) {
			for (int i = 0; i < item_count(menu); ++i) {
				if (i > cfg->select_pos) {
					set_item_value(items[i], true);
				}
				if (i < cfg->select_pos) {
					set_item_value(items[i], false);
				}
			}
		}
		exit = 0;
		break;
	case CTRL('b'):
	case KEY_PPAGE:
		if (cfg->select) {
			cur = current_item(menu);
			init_pos = item_index(cur);
		}
		menu_driver(menu, REQ_SCR_UPAGE);
		if (cfg->select) {
			cur = current_item(menu);
			end_pos = item_index(cur);
			for (int i = end_pos; i <= init_pos; ++i) {
				if (i > cfg->select_pos) {
					set_item_value(items[i], false);
				}
				if (i < cfg->select_pos) {
					set_item_value(items[i], true);
				}
			}
		}
		exit = 0;
		break;
	case CTRL('f'):
	case KEY_NPAGE:
		if (cfg->select) {
			cur = current_item(menu);
			init_pos = item_index(cur);
		}
		menu_driver(menu, REQ_SCR_DPAGE);
		if (cfg->select) {
			cur = current_item(menu);
			end_pos = item_index(cur);
			for (int i = init_pos; i <= end_pos; ++i) {
				if (i > cfg->select_pos) {
					set_item_value(items[i], true);
				}
				if (i < cfg->select_pos) {
					set_item_value(items[i], false);
				}
			}
		}
		exit = 0;
		break;
	case 10:
		mpv_initialize(ctx);
		int n = 0;
		for (int i = 0; i < item_count(menu); ++i) {
			if (item_value(items[i])) {
				path = item_description(items[i]);
				mpv_queue(ctx, path);
				++n;
			}
		}
		if (n) {
			//exit = mpv_wait(ctx, n, menu, items, cfg, lib);
		} else {
			cur = current_item(menu);
			path = item_description(cur);
			mpv_queue(ctx, path);
			//exit = mpv_wait(ctx, 1, menu, items, cfg, lib);
		}
		//TODO: fix mpv's event loop detection
		exit = 0;
		break;
	case 'q':
		exit = 1;
		break;
	default:
		exit = 0;
		break;
	}
	wrefresh(menu_win(menu));
	if (exit) {
		mpv_destroy(ctx);
	}
	return exit;
}

int move_menu_backward(const char *path, struct vmn_config *cfg, struct vmn_library *lib) {
	if (lib->depth == 0) {
		return 0;
	}
	--lib->depth;
	WINDOW *win = menu_win(lib->menu[lib->depth]);
	wmove(win, 0, 0);
	wrefresh(win);
	return 0;
}

int move_menu_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib) {
	char ***dir = get_lib_dir(path, lib);
	if (!dir) {
		return 0;
	}
	++lib->depth;
	double startx = getmaxx(stdscr);
	//TODO: resize previous menus
	/*for (int i = 0; i < lib->depth; ++i) {
		wresize(menu_win(lib->menu[i]), 0, (startx*i+1)/(lib->depth+1));
		wrefresh(menu_win(lib->menu[i]));
	}*/
	//make new menu
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*lib->depth+1);
	lib->items[lib->depth] = create_items(dir);
	lib->menu = (MENU **)realloc(lib->menu, sizeof(MENU *)*lib->depth+1);
	lib->menu[lib->depth] = new_menu((ITEM **)lib->items[lib->depth]);
	set_menu_format(lib->menu[lib->depth], LINES, 0);
	menu_opts_off(lib->menu[lib->depth], O_ONEVALUE);
	menu_opts_off(lib->menu[lib->depth], O_SHOWDESC);
	WINDOW *win = newwin(0, 0, 0, (startx*lib->depth)/(lib->depth+1));
	set_menu_win(lib->menu[lib->depth], win);
	set_menu_sub(lib->menu[lib->depth], win);
	post_menu(lib->menu[lib->depth]);
	wmove(win, 0, 0);
	wrefresh(win);
	return 0;
}

mpv_handle *mpv_generate(struct vmn_config *cfg) {
	mpv_handle *ctx = mpv_create();
	mpv_set_option_string(ctx, "input-default-bindings", "yes");
	mpv_set_option_string(ctx, "input-vo-keyboard", "yes");
	mpv_set_option_string(ctx, "force-window", "yes");
	mpv_set_option_string(ctx, "config-dir", cfg->mpv_cfg_dir);
	mpv_set_option_string(ctx, "config", cfg->mpv_cfg);
	int val = 1;
	mpv_set_option(ctx, "osc", MPV_FORMAT_FLAG, &val);
	return ctx;
}

void mpv_queue(mpv_handle *ctx, const char *audio) {
	const char *cmd[] = {"loadfile", audio, "append-play", NULL};
	mpv_command(ctx, cmd);
}

int mpv_wait(mpv_handle *ctx, int len, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib) {
	int n = 0;
	int c;
	int exit;
	while (1) {
		c = getch();
		exit = key_event(c, menu, items, cfg, lib);
		mpv_event *event = mpv_wait_event(ctx, 0);
		if (event->event_id == MPV_EVENT_SHUTDOWN) {
			break;
		}
		if (event->event_id == MPV_EVENT_END_FILE) {
			++n;
			if (n == len) {
				break;
			}
		}
		if (exit == 1) {
			return 1;
		}
	}
	mpv_destroy(ctx);
	return 0;
}

int path_in_lib(char *path, struct vmn_library *lib) {
	regex_t regex;
	int status;
	regcomp(&regex, path, 0);
	for (int i = 0; i < lib->length; ++i) {
		status = regexec(&regex, lib->files[i], 0, NULL, 0);
		if (status == 0) {
			regfree(&regex);
			return 1;
			break;
		}
	}
	regfree(&regex);
	return 0;
}

int qstrcmp(const void *a, const void *b) {
	const char *aa = *(const char**)a;
	const char *bb = *(const char**)b;
	return strcasecmp(aa, bb);
}
