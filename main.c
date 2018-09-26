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
void destroy_last_menu(struct vmn_library *lib);
int ext_valid(char *ext);
char *get_file_ext(const char *file);
char ***get_lib_dir(const char *library, struct vmn_library *lib);
ITEM **get_lib_items(struct vmn_library *lib);
int get_music_files(const char *library, struct vmn_library *lib);
int key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib);
int move_menu_backward(const char *path, struct vmn_config *cfg, struct vmn_library *lib);
int move_menu_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib);
mpv_handle *mpv_generate(struct vmn_config *cfg);
void mpv_queue(mpv_handle *ctx, const char *audio);
int path_in_lib(char *path, struct vmn_library *lib);
int qstrcmp(const void *a, const void *b);

int main() {
	setlocale(LC_CTYPE, "");
	struct vmn_config cfg = cfg_init();
	struct vmn_library lib = lib_init();
	int invalid = get_music_files(cfg.lib_dir, &lib);
	if (invalid) {
		vmn_config_destroy(&cfg);
		vmn_library_destroy(&lib);
		printf("Either the directory does not exist or no audio files were found.\n");
		return 0;
	}
	initscr();
	cbreak();
	noecho();
	lib.entries = (char ****)calloc(1, sizeof(char ***));
	lib.entries[0] = get_lib_dir(cfg.lib_dir, &lib);
	lib.items = (ITEM ***)calloc(1, sizeof(ITEM **));
	lib.items[0] = create_items(lib.entries[0]);
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
	int c;
	int exit = 0;
	mpv_handle *ctx;
	const char *path;
	ctx = mpv_generate(&cfg);
	while (1) {
		mpv_event *event = mpv_wait_event(ctx, 0);
		if (event->event_id == MPV_EVENT_SHUTDOWN) {
			mpv_terminate_destroy(ctx);
			ctx = mpv_generate(&cfg);
		}
		if (event->event_id == MPV_EVENT_END_FILE) {
			char *idle = mpv_get_property_string(ctx, "idle-active");
			if (strcmp(idle, "yes") == 0) {
				mpv_terminate_destroy(ctx);
				ctx = mpv_generate(&cfg);
			}
		}
		
		MENU *menu = lib.menu[lib.depth];
		ITEM **items = lib.items[lib.depth];
		c = wgetch(win);
		exit = key_event(c, menu, items, &cfg, &lib);
		int n = 0;
		if (lib.mpv_active) {
			mpv_initialize(ctx);
			for (int i = 0; i < item_count(menu); ++i) {
				if (item_value(items[i])) {
					path = item_description(items[i]);
					mpv_queue(ctx, path);
					++n;
				}
			}
			if (!n) {
				ITEM *cur = current_item(menu);
				path = item_description(cur);
				mpv_queue(ctx, path);
			}
			lib.mpv_active = 0;
		}
		if (exit) {
			break;
		}
	}
	mpv_destroy(ctx);
	vmn_config_destroy(&cfg);
	vmn_library_destroy(&lib);
	endwin();
	return 0;
}

ITEM **create_items(char ***entries) {
	ITEM **items;
	int n = 0;
	while (entries[0][n]) {
		++n;
	}
	items = (ITEM **)calloc(n+1, sizeof(ITEM *));
	int i;
	for (i = 0; i < n; ++i) {
		items[i] = new_item(entries[0][i], entries[1][i]);
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

void destroy_last_menu(struct vmn_library *lib) {
	int n = item_count(lib->menu[lib->depth]);
	unpost_menu(lib->menu[lib->depth]);
	wrefresh(menu_win(lib->menu[lib->depth]));
	delwin(menu_win(lib->menu[lib->depth]));
	free_menu(lib->menu[lib->depth]);
	for (int i = 0; i < n; ++i) {
		free_item(lib->items[lib->depth][i]);
		free(lib->entries[lib->depth][0][i]);
		free(lib->entries[lib->depth][1][i]);
	}
	free(lib->entries[lib->depth][0]);
	free(lib->entries[lib->depth][1]);
	free(lib->entries[lib->depth]);
	free(lib->items[lib->depth]);
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

	if (!dir) {
		return 0;
	}

	char ***dir_info = (char ***)malloc(sizeof(char **)*2);
	dir_info[0] = (char **)malloc(sizeof(char *)*1);
	dir_info[1] = (char **)malloc(sizeof(char *)*1);

	int i = 0;
	while ((dp = readdir(dir)) != NULL) {
		char path[1024];
		strcpy(path, library);
		strcat(path, "/");
		strcat(path, dp->d_name);
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
			if (dp->d_type == DT_DIR && path_in_lib(path, lib)) {
				dir_info[0] = (char **)realloc(dir_info[0],sizeof(char *)*(i+1));
				dir_info[1] = (char **)realloc(dir_info[1],sizeof(char *)*(i+1));
				dir_info[0][i] = malloc(sizeof(char *)*(strlen(dp->d_name) + 1));
				dir_info[1][i] = malloc(sizeof(char *)*(strlen(path) + 1));
				strcpy(dir_info[0][i], dp->d_name);
				strcpy(dir_info[1][i], path);
				++i;
			}
			if (dp->d_type == DT_REG) {
				char *ext = get_file_ext(dp->d_name);
				if (ext_valid(ext)) {
					dir_info[0] = (char **)realloc(dir_info[0],sizeof(char *)*(i+1));
					dir_info[1] = (char **)realloc(dir_info[1],sizeof(char *)*(i+1));
					dir_info[0][i] = malloc(sizeof(char *)*(strlen(dp->d_name) + 1));
					dir_info[1][i] = malloc(sizeof(char *)*(strlen(path) + 1));
					strcpy(dir_info[0][i], dp->d_name);
					strcpy(dir_info[1][i], path);
					++i;
				}
			}
		}
	}
	
	closedir(dir);
	qsort(dir_info[0], i, sizeof(char *), qstrcmp);
	qsort(dir_info[1], i, sizeof(char *), qstrcmp);
	dir_info[0] = (char **)realloc(dir_info[0],sizeof(char *)*(i+1));
	dir_info[1] = (char **)realloc(dir_info[1],sizeof(char *)*(i+1));
	dir_info[0][i] = '\0';
	dir_info[1][i] = '\0';
	return dir_info;
}

int get_music_files(const char *library, struct vmn_library *lib) {
	struct dirent *dp;
	DIR *dir = opendir(library);

	if (!dir) {
		return 1;
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

	if (lib->length == 0) {
		return 1;
	} else {
		return 0;
	}
}

int key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib) {
	int init_pos;
	int end_pos;
	int exit;
	ITEM *cur;
	const char *path;

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
		++lib->mpv_active;
		exit = 0;
		break;
	case 'q':
		exit = 1;
		break;
	default:
		exit = 0;
		break;
	}
	wrefresh(menu_win(lib->menu[lib->depth]));
	return exit;
}

int move_menu_backward(const char *path, struct vmn_config *cfg, struct vmn_library *lib) {
	if (lib->depth == 0) {
		return 0;
	}
	destroy_last_menu(lib);
	--lib->depth;
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*(lib->depth+1));
	lib->menu = (MENU **)realloc(lib->menu, sizeof(MENU *)*(lib->depth+1));
	double startx = getmaxx(stdscr);
	for (int i = 1; i <= lib->depth; ++i) {
		unpost_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
		wresize(menu_win(lib->menu[i]), 0, (startx*i)/(lib->depth+1));
		mvwin(menu_win(lib->menu[i]), 0, (startx*i)/(lib->depth+1));
		post_menu(lib->menu[i]);
	}
	wrefresh(menu_win(lib->menu[lib->depth]));
	return 0;
}

int move_menu_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib) {
	++lib->depth;
	lib->entries = (char ****)realloc(lib->entries, sizeof(char ***)*(lib->depth+1));
	lib->entries[lib->depth] = get_lib_dir(path, lib);
	if (!lib->entries[lib->depth]) {
		--lib->depth;
		lib->entries = (char ****)realloc(lib->entries, sizeof(char ***)*(lib->depth+1));
		return 0;
	}
	double startx = getmaxx(stdscr);
	for (int i = 1; i < lib->depth; ++i) {
		wresize(menu_win(lib->menu[i]), 0, (startx*i)/(lib->depth+1));
		mvwin(menu_win(lib->menu[i]), 0, (startx*i)/(lib->depth+1));
		wrefresh(menu_win(lib->menu[i]));
	}
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*(lib->depth+1));
	lib->items[lib->depth] = create_items(lib->entries[lib->depth]);
	lib->menu = (MENU **)realloc(lib->menu, sizeof(MENU *)*(lib->depth+1));
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
