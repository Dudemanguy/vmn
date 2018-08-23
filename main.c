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

int cmpstr(const void *a, const void *b);
int ext_valid(char *ext);
char *get_cfg();
char *get_cfg_dir();
char *get_cfg_lib();
char *get_file_ext(const char *file);
void get_music_files(const char *base);
ITEM **get_lib_items();
void mpv_queue(mpv_handle *ctx, const char *audio);
void mpv_wait(mpv_handle *ctx);
void show_library();

int main() {
	setlocale(LC_CTYPE, "");
	const char *library;
	char *cfg_file = get_cfg();

	config_t cfg;
	config_init(&cfg);

	if (!config_read_file(&cfg, cfg_file)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}

	config_lookup_string(&cfg, "library", &library);
	char *lib = get_cfg_lib();
	remove(lib);
	get_music_files(library);
	show_library();

	return 0;
}

int cmpstr(const void *a, const void *b) {
	const char *aa = *(const char**)a;
	const char *bb = *(const char**)b;
	return strcmp(aa, bb);
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

void mpv_queue(mpv_handle *ctx, const char *audio) {
	const char *cmd[] = {"loadfile", audio, "append-play", NULL};
	mpv_command(ctx, cmd);
}

void mpv_wait(mpv_handle *ctx) {
	while (1) {
		mpv_event *event = mpv_wait_event(ctx, 10000);
		if (event->event_id == MPV_EVENT_SHUTDOWN) {
			break;
		}
	}
	mpv_terminate_destroy(ctx);
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

char *get_cfg_lib() {
	char *home = getenv("HOME"); 
	const char *lib = "/.config/vmn/lib";
	char *path = malloc(strlen(home) + strlen(lib) + 1);
	strcpy(path, home);
	strcat(path, lib);
	return path;
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
	qsort(lines, i, sizeof(char *), cmpstr);
	items = (ITEM **)calloc(i + 1, sizeof(ITEM *));
	for (int k = 0; k < i; ++k) {
		items[k] = new_item(lines[k], NULL);
	}
	items[i] = (ITEM *)NULL;
	return items;
}

void show_library() {
	MENU *menu;
	int c;
	int i = 0;
	ITEM **items = get_lib_items();
	
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	menu = new_menu((ITEM **)items);
	set_menu_format(menu, 50, 0);
	menu_opts_off(menu, O_ONEVALUE);
	post_menu(menu);
	refresh();
	const char *name;
	mpv_handle *ctx = mpv_create();
	mpv_set_option_string(ctx, "input-default-bindings", "yes");
	mpv_set_option_string(ctx, "input-vo-keyboard", "yes");
	mpv_initialize(ctx);

	while ((c = getch()) != 'q') {
		switch(c) {
		case ' ':
		case 'i':
			menu_driver(menu, REQ_TOGGLE_ITEM);
			break;
		case KEY_UP:
		case 'k':
			menu_driver(menu, REQ_UP_ITEM);
			break;
		case KEY_DOWN:
		case 'j':
			menu_driver(menu, REQ_DOWN_ITEM);
			break;
		case KEY_HOME:
		case 'g':
			menu_driver(menu, REQ_FIRST_ITEM);
			break;
		case KEY_END:
		case 'G':
			menu_driver(menu, REQ_LAST_ITEM);
			break;
		case KEY_PPAGE:
			menu_driver(menu, REQ_SCR_UPAGE);
			break;
		case KEY_NPAGE:
			menu_driver(menu, REQ_SCR_DPAGE);
			break;
		case 10:
			for (int j = 0; j < item_count(menu); ++j) {
				if (item_value(items[j])) {
					name = item_name(items[j]);
					mpv_queue(ctx, name);
				}
			}
			break;
		}
	}
	mpv_wait(ctx);

	unpost_menu(menu);
	free_menu(menu);
	while (items[i]) {
		free_item(items[i]);
		++i;
	}
	endwin();
}
