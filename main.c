#include <dirent.h>
#include <libconfig.h>
#include <locale.h>
#include <menu.h>
#include <mpv/client.h>
#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"

#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

int ext_valid(char *ext);
char *get_file_ext(const char *file);
void get_music_files(const char *base);
ITEM **get_lib_items();
int key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg);
mpv_handle *mpv_generate(struct vmn_config *cfg);
void mpv_queue(mpv_handle *ctx, const char *audio);
int mpv_wait(mpv_handle *ctx, int len, MENU *menu, ITEM **items, struct vmn_config *cfg);
int qstrcmp(const void *a, const void *b);
MENU *set_library(ITEM **items);

int main() {
	setlocale(LC_CTYPE, "");
	struct vmn_config cfg = cfg_init();
	char *lib = get_cfg_lib();
	char *cfg_file = get_cfg();
	check_dir();
	check_cfg(cfg_file);
	int mpv_config = read_cfg_int(cfg_file, "mpv_config");
	if ((mpv_config == 0) || (mpv_config == 1)) {
		cfg.mpv_config = mpv_config;
	} else {
		cfg.mpv_config = 1;
	}
	const char *library = read_cfg_string(cfg_file, "library");
	remove(lib);
	get_music_files(library);
	ITEM **items = get_lib_items();
	MENU *menu = set_library(items);
	post_menu(menu);
	refresh();
	int c;
	int exit = 0;
	while (!exit) {
		c = getch();
		exit = key_event(c, menu, items, &cfg);
	}
	unpost_menu(menu);
	free_menu(menu);
	int i = 0;
	while (items[i]) {
		free_item(items[i]);
		++i;
	}
	endwin();
	return 0;
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

void get_music_files(const char *library) {
	struct dirent *dp;
	DIR *dir = opendir(library);
	char *lib = get_cfg_lib();
	FILE *fp = fopen(lib, "a+");

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
				get_music_files(path);
			} else {
				char *ext = get_file_ext(dp->d_name);
				if (ext_valid(ext)) {
					fputs(path, fp);
					fputs("\n", fp);
				}
			}
		}
	}
	fclose(fp);
	closedir(dir);
}

ITEM **get_lib_items() {
	ITEM **items;
	int lines_allocated = 1000;
	int max_line_len = 4096;
	char *lib = get_cfg_lib();

	char **lines = (char **)malloc(sizeof(char*)*lines_allocated);
	FILE *fp = fopen(lib, "r");

	int i;
	for (i = 0; 1; ++i) {
		int j;

		if (i >= lines_allocated) {
			int new_size;
			new_size = lines_allocated*2;
			lines = (char **)realloc(lines,sizeof(char*)*new_size);
			lines_allocated = new_size;
		}
		lines[i] = malloc(max_line_len);
		if (fgets(lines[i], max_line_len-1, fp)==NULL)
			break;
		for (j=strlen(lines[i])-1; j>=0 && (lines[i][j]=='\n' || lines[i][j]=='\r'); j--) {
			;
		}
		lines[i][j+1]='\0';
	}
    fclose(fp);
	qsort(lines, i, sizeof(char *), qstrcmp);
	items = (ITEM **)calloc(i + 1, sizeof(ITEM *));
	for (int k = 0; k < i; ++k) {
		items[k] = new_item(lines[k], NULL);
	}
	items[i] = (ITEM *)NULL;
	return items;
}

int key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg) {
	int init_pos;
	int end_pos;
	int exit;
	ITEM *cur;
	const char *name;
	mpv_handle *ctx;

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
		int cur_pos = item_index(cur);
		if (cfg->select) {
			cur = current_item(menu);
			if (cur_pos > cfg->select_pos) {
				set_item_value(cur, true);
			}
			if (cfg->select_pos != cur_pos) {
				set_item_value(cur, false);
			}
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
		ctx = mpv_generate(cfg);
		int n = 0;
		for (int i = 0; i < item_count(menu); ++i) {
			if (item_value(items[i])) {
				name = item_name(items[i]);
				mpv_queue(ctx, name);
				++n;
			}
		}
		if (n) {
			exit = mpv_wait(ctx, n, menu, items, cfg);
		} else {
			cur = current_item(menu);
			name = item_name(cur);
			mpv_queue(ctx, name);
			exit = mpv_wait(ctx, 1, menu, items, cfg);
		}
		break;
	case 'q':
		mpv_terminate_destroy(ctx);
		exit = 1;
		break;
	default:
		exit = 0;
		break;
	}
	return exit;
}

mpv_handle *mpv_generate(struct vmn_config *cfg) {
	mpv_handle *ctx = mpv_create();
	mpv_set_option_string(ctx, "input-default-bindings", "yes");
	mpv_set_option_string(ctx, "input-vo-keyboard", "yes");
	if (cfg->mpv_config) {
		mpv_set_option_string(ctx, "config", "yes");
	}
	int val = 1;
	mpv_set_option(ctx, "osc", MPV_FORMAT_FLAG, &val);
	mpv_initialize(ctx);
	return ctx;
}

void mpv_queue(mpv_handle *ctx, const char *audio) {
	const char *cmd[] = {"loadfile", audio, "append-play", NULL};
	mpv_command(ctx, cmd);
}

int mpv_wait(mpv_handle *ctx, int len, MENU *menu, ITEM **items, struct vmn_config *cfg) {
	int n = 0;
	int c;
	int exit;
	while (1) {
		c = getch();
		exit = key_event(c, menu, items, cfg);
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
	mpv_terminate_destroy(ctx);
	return 0;
}

int qstrcmp(const void *a, const void *b) {
	const char *aa = *(const char**)a;
	const char *bb = *(const char**)b;
	return strcmp(aa, bb);
}

MENU *set_library(ITEM **items) {
	MENU *menu;
	
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	menu = new_menu((ITEM **)items);
	set_menu_format(menu, LINES, 0);
	menu_opts_off(menu, O_ONEVALUE);
	return menu;
}
