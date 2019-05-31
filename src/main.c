#include <dirent.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <locale.h>
#include <math.h>
#include <menu.h>
#include <mpv/client.h>
#include <ncurses.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"
#include "library.h"
#include "command.h"
#include "utils.h"

ITEM **create_meta_items(char ***metadata);
ITEM **create_path_items(char ***files);
void input_mode(struct vmn_config *cfg);
char ***get_lib_dir(const char *library, struct vmn_library *lib);
char ***get_metadata(struct vmn_config *cfg, struct vmn_library *lib);
int get_music_files(const char *library, struct vmn_library *lib);
void key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib);
void meta_path_find(struct vmn_config *cfg, struct vmn_library *lib, const char *name, int index);
int move_menu_meta_backward(struct vmn_library *lib);
int move_menu_path_backward(struct vmn_library *lib);
int move_menu_meta_forward(struct vmn_config *cfg, struct vmn_library *lib);
int move_menu_path_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib);
mpv_handle *mpv_generate(struct vmn_config *cfg);
void mpv_queue(mpv_handle *ctx, const char *audio);
int path_in_lib(char *path, struct vmn_library *lib);
void resize_detected();
void sort_select(struct vmn_config *cfg, struct vmn_library *lib, char ***metadata, int len);
int **trackorder(struct vmn_config *cfg, struct vmn_library *lib, char ***metadata, int len);
void tracksort(char ***metadata, int **order, int len);

/* for handling terminal resize events */
int resize;

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");
	struct vmn_config cfg = cfg_init(argc, argv);
	if (cfg.err) {
		return 0;
	}
	struct vmn_library lib = lib_init();
	if (strcmp(cfg.input_mode, "yes") == 0) {
		input_mode(&cfg);
	}
	int invalid = get_music_files(cfg.lib_dir, &lib);
	qsort(lib.files, lib.length, sizeof(char *), qstrcmp);
	if (invalid) {
		if (cfg.view == V_DATA) {
			vmn_library_sort(&lib, cfg.lib_dir);
		}
		vmn_config_destroy(&cfg);
		for (int i = 0; i < lib.length; ++i) {
			free(lib.files[i]);
		}
		free(lib.files);
		printf("No audio files were found.\n");
		return 0;
	}
	initscr();
	cbreak();
	noecho();
	if (cfg.view == V_PATH) {
		lib.entries = (char ****)calloc(1, sizeof(char ***));
		lib.entries[0] = get_lib_dir(cfg.lib_dir, &lib);
		lib.items = (ITEM ***)calloc(1, sizeof(ITEM **));
		lib.items[0] = create_path_items(lib.entries[0]);
	}
	if (cfg.view == V_DATA) {
		lib.entries = (char ****)calloc(1, sizeof(char ***));
		vmn_library_metadata(&lib);
		vmn_library_sort(&lib, cfg.lib_dir);
		lib.selections = (char **)calloc(cfg.tags_len, sizeof(char *));
		lib.entries[0] = get_metadata(&cfg, &lib);
		lib.items = (ITEM ***)calloc(1, sizeof(ITEM **));
		lib.items[0] = create_meta_items(lib.entries[0]);
	}
	if (cfg.view == V_SONG) {
		lib.entries = (char ****)calloc(1, sizeof(char ***));
		lib.entries[0] = (char ***)calloc(2, sizeof(char **));
		lib.entries[0][0] = lib.files;
		lib.entries[0][1] = lib.files;
		lib.items = (ITEM ***)calloc(1, sizeof(ITEM **));
		lib.items[0] = (ITEM **)calloc(lib.length+1, sizeof(ITEM *));
		for (int i = 0; i < lib.length; ++i) {
			lib.items[0][i] = new_item(lib.entries[0][0][i], lib.entries[0][1][i]);
		}
		lib.items[0][lib.length] = (ITEM *)NULL;
	}
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
	if (cfg.view == V_DATA) {
		ITEM *cur = current_item(lib.menu[0]);
		const char *name = item_name(cur);
		vmn_library_selections_add(&lib, name);
	}
	int c;
	lib.ctx = mpv_generate(&cfg);
	while (1) {
		signal(SIGWINCH, resize_detected);
		mpv_event *event = mpv_wait_event(lib.ctx, 0);
		wtimeout(win, 20);
		c = wgetch(win);
		key_event(c, lib.menu[lib.depth-1], lib.items[lib.depth-1], &cfg, &lib);
		if (resize) {
			resize = 0;
			struct winsize ws;
			ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
			resize_term(ws.ws_row, ws.ws_col);
			int maxx = getmaxx(stdscr);
			for (int i = 0; i < lib.depth; ++i) {
				int cur_pos = item_index(current_item(lib.menu[i]));
				unpost_menu(lib.menu[i]);
				set_menu_format(lib.menu[i], LINES, 0);
				set_top_row(lib.menu[i], cur_pos);
				mvwin(menu_win(lib.menu[i]), 0, (int)(maxx*i)/(lib.depth));
				post_menu(lib.menu[i]);
				wrefresh(menu_win(lib.menu[i]));
			}
		}
		if (event->event_id == MPV_EVENT_SHUTDOWN) {
			mpv_terminate_destroy(lib.ctx);
			lib.mpv_active = 0;
			lib.ctx = mpv_generate(&cfg);
		} else if (event->event_id == MPV_EVENT_END_FILE) {
			char *idle = mpv_get_property_string(lib.ctx, "idle-active");
			if (strcmp(idle, "yes") == 0) {
				mpv_terminate_destroy(lib.ctx);
				lib.mpv_active = 0;
				lib.ctx = mpv_generate(&cfg);
			}
			mpv_free(idle);
		}
		if (cfg.view == V_DATA) {
			ITEM *cur = current_item(lib.menu[lib.depth-1]);
			const char *name = item_name(cur);
			if (name) {
				vmn_library_selections_add(&lib, name);
			}
		}
		if (lib.mpv_kill) {
			mpv_terminate_destroy(lib.ctx);
			lib.ctx = NULL;
			lib.ctx = mpv_generate(&cfg);
			lib.mpv_kill = 0;
			lib.mpv_active = 0;
		}
		if (lib.vmn_quit) {
			break;
		}
	}
	mpv_destroy(lib.ctx);
	vmn_config_destroy(&cfg);
	if (cfg.view == V_PATH) {
		vmn_library_destroy(&lib);
	}
	if (cfg.view == V_DATA) {
		vmn_library_destroy(&lib);
		for (int i = 0; i < cfg.tags_len; ++i) {
			free(lib.selections[i]);
		}
		free(lib.selections);
	}
	if (cfg.view == V_SONG) {
		for (int i = 0; i < lib.length; ++i) {
			free(lib.files[i]);
		}
		free(lib.files);
		free(lib.entries[0]);
		free(lib.entries);
		unpost_menu(lib.menu[0]);
		free_menu(lib.menu[0]);
		free(lib.menu);
		for (int i = 0; i < lib.length; ++i) {
			free_item(lib.items[0][i]);
		}
		free(lib.items[0]);
		free(lib.items);
	}
	endwin();
	return 0;
}

void create_visual_window(struct vmn_library *lib) {
	lib->visual = newwin(1, 0, LINES - 1, 0);
	mvwprintw(lib->visual, 0, 0, "-- VISUAL --\n");
	wrefresh(lib->visual);
}

ITEM **create_meta_items(char ***metadata) {
	ITEM **items;
	int n = 0;
	while (metadata[0][n]) {
		++n;
	}
	items = (ITEM **)calloc(n+1, sizeof(ITEM *));
	for (int i = 0; i < n; ++i) {
		items[i] = new_item(metadata[0][i], metadata[1][i]);
	}
	items[n] = (ITEM *)NULL;
	return items;
}

ITEM **create_path_items(char ***entries) {
	ITEM **items;
	int n = 0;
	while (entries[0][n]) {
		++n;
	}
	items = (ITEM **)calloc(n+1, sizeof(ITEM *));
	for (int i = 0; i < n; ++i) {
		items[i] = new_item(entries[0][i], entries[1][i]);
	}
	items[n] = (ITEM *)NULL;
	return items;
}

void destroy_last_menu_meta(struct vmn_library *lib) {
	int n = item_count(lib->menu[lib->depth-1]);
	unpost_menu(lib->menu[lib->depth-1]);
	wrefresh(menu_win(lib->menu[lib->depth-1]));
	delwin(menu_win(lib->menu[lib->depth-1]));
	free_menu(lib->menu[lib->depth-1]);
	for (int i = 0; i < n; ++i) {
		free_item(lib->items[lib->depth-1][i]);
		free(lib->entries[lib->depth-1][0][i]);
		free(lib->entries[lib->depth-1][1][i]);
	}
	free(lib->items[lib->depth-1]);
	free(lib->entries[lib->depth-1][0]);
	free(lib->entries[lib->depth-1][1]);
	free(lib->entries[lib->depth-1]);
}

void destroy_last_menu_path(struct vmn_library *lib) {
	int n = item_count(lib->menu[lib->depth-1]);
	unpost_menu(lib->menu[lib->depth-1]);
	wrefresh(menu_win(lib->menu[lib->depth-1]));
	delwin(menu_win(lib->menu[lib->depth-1]));
	free_menu(lib->menu[lib->depth-1]);
	for (int i = 0; i < n; ++i) {
		free_item(lib->items[lib->depth-1][i]);
		free(lib->entries[lib->depth-1][0][i]);
		free(lib->entries[lib->depth-1][1][i]);
	}
	free(lib->entries[lib->depth-1][0]);
	free(lib->entries[lib->depth-1][1]);
	free(lib->entries[lib->depth-1]);
	free(lib->items[lib->depth-1]);
}

void destroy_visual_window(struct vmn_library *lib) {
	delwin(lib->visual);
	for (int i = 0; i < lib->depth; ++i) {
		unpost_menu(lib->menu[i]);
		post_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
	}
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

char ***get_metadata(struct vmn_config *cfg, struct vmn_library *lib) {
	char *home = getenv("HOME"); 
	const char *cfg_path = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg_path) + 1);
	strcpy(path, home);
	strcat(path, cfg_path);
	FILE *cache = fopen(path, "r");
	char c;
	int file_len = 0;
	while ((c = fgetc(cache)) != EOF) {
		if (c == '\n') {
			++file_len;
		}
	}
	rewind(cache);
	char *cur = malloc(4096*sizeof(char));
	int len = 0;
	int match = 0;
	int prev;
	int tag_exist = 0;
	int unknown_len = 16;
	int *unknown = (int *)calloc(unknown_len, sizeof(int));
	int unknown_pos = 0;
	char ***metadata = (char ***)calloc(2, sizeof(char **));
	metadata[0] = (char **)calloc(lib->length+1, sizeof(char *));
	metadata[1] = (char **)calloc(lib->length+1, sizeof(char *));
	char *temp;
	int valid_sel = 0;
	for (int i = 0; i < file_len; ++i) {
		fgets(cur, 4096, cache);
		int in_lib = check_vmn_lib(lib, cur, cfg->lib_dir);
		if (!in_lib) {
			continue;
		}
		if (lib->selections[0]) {
			prev = check_vmn_cache(lib, cur, cfg->tags);
			if (!prev) {
				continue;
			}
			if (!valid_sel) {
				valid_sel = is_sel(lib->selections[lib->depth-2], cur);
			}
			int prev_known = is_known(cfg->tags[lib->depth-2], cur);
			if (!prev_known) {
				if (unknown_pos == unknown_len) {
					unknown_len = unknown_len*2;
					unknown = (int *)realloc(unknown, unknown_len*sizeof(int));
				}
				unknown[unknown_pos] = i;
				++unknown_pos;
				continue;
			}
		}
		int known = is_known(cfg->tags[lib->depth-1], cur);
		char *filename = read_vmn_cache(cur, "");
		if (!known) {
			if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
				metadata[0][len] = (char *)calloc(strlen(filename+1) + 1, sizeof(char));
				strcpy(metadata[0][len], filename+1);
				metadata[1][len] = (char *)calloc(strlen(filename) + 1, sizeof(char));
				strcpy(metadata[1][len], filename);
				++len;
			} else {
				if (!tag_exist) {
					metadata[0][len] = (char *)calloc(strlen(cfg->tags[lib->depth-1]) + strlen("Unknown ") + 1, sizeof(char));
					strcpy(metadata[0][len], "Unknown ");
					strcat(metadata[0][len], cfg->tags[lib->depth-1]);
					tag_exist = 1;
					++len;
				}
			}
			free(filename);
		} else {
			temp = read_vmn_cache(cur, cfg->tags[lib->depth-1]);
			for (int j = 0; j < len; ++j) {
				if ((strcmp(temp, metadata[0][j]) == 0) && (strcmp(cfg->tags[lib->depth-1], "title") != 0)) {
					match = 1;
					break;
				}
			}
			if (match) {
				match = 0;
				free(filename);
				free(temp);
				continue;
			} else {
				metadata[0][len] = malloc(sizeof(char *)*(strlen(temp)+1));
				strcpy(metadata[0][len], temp);
				if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
					metadata[1][len] = malloc(sizeof(char *)*(strlen(filename)+1));
					strcpy(metadata[1][len], filename);
				}
				++len;
				free(filename);
				free(temp);
			}
		}
	}
	if (unknown_pos && !valid_sel) {
		rewind(cache);
		unknown_pos = 0;
		for (int i = 0; i < file_len; ++i) {
			fgets(cur, 4096, cache);
			char *filename = read_vmn_cache(cur, "");
			if (i == unknown[unknown_pos]) {
				++unknown_pos;
				int known = is_known(cfg->tags[lib->depth-1], cur);
				if (!known) {
					if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
						metadata[0][len] = (char *)calloc(strlen(filename+1) + 1, sizeof(char));
						strcpy(metadata[0][len], filename+1);
						metadata[1][len] = (char *)calloc(strlen(filename) + 1, sizeof(char));
						strcpy(metadata[1][len], filename);
						++len;
					} else {
						if (!tag_exist) {
							metadata[0][len] = (char *)calloc(strlen(cfg->tags[lib->depth-1]) + strlen("Unknown ") + 1, sizeof(char));
							strcpy(metadata[0][len], "Unknown ");
							strcat(metadata[0][len], cfg->tags[lib->depth-1]);
							tag_exist = 1;
							++len;
						}
					}
					free(filename);
				} else {
					temp = read_vmn_cache(cur, cfg->tags[lib->depth-1]);
					for (int j = 0; j < len; ++j) {
						if (strcmp(temp, metadata[0][j]) == 0) {
							match = 1;
							break;
						}
					}
					if (match) {
						match = 0;
						free(filename);
						free(temp);
						continue;
					} else {
						metadata[0][len] = malloc(sizeof(char *)*(strlen(temp)+1));
						strcpy(metadata[0][len], temp);
						if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
							metadata[1][len] = malloc(sizeof(char *)*(strlen(filename)+1));
							strcpy(metadata[1][len], filename);
						}
						++len;
						free(filename);
						free(temp);
					}
				}
			}
		}
	}
	fclose(cache);
	free(cur);
	free(path);
	free(unknown);
	metadata[0] = (char **)realloc(metadata[0],sizeof(char *)*(len+1));
	metadata[1] = (char **)realloc(metadata[1],sizeof(char *)*(len+1));
	metadata[0][len] = '\0';
	metadata[1][len] = '\0';
	sort_select(cfg, lib, metadata, len);
	return metadata;
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

void input_mode(struct vmn_config *cfg) {
		int key;
		initscr();
		noecho();
		keypad(stdscr, TRUE);
		while (1) {
			mvprintw(0, 0, "Input mode is enabled. Keycodes will be returned on the screen. Use ESC or ctrl+[ to exit.\n");
			key = getch();
			mvprintw(1, 0, "Key = %d\n", key);
			if (key == cfg->key.escape) {
				clear();
				mvprintw(0, 0, "Are you sure you want to quit input mode? Hit 'y' to confirm.\n");
				int quit = getch();
				if (quit == 'y') {
					break;
				} else {
					clear();
				}
			}
			refresh();
		}
		clear();
		refresh();
		endwin();
}

void key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib) {
	int init_pos;
	int end_pos;
	ITEM *cur;
	const char *name;
	const char *path;

	if (c == cfg->key.beginning) {
		menu_driver(menu, REQ_FIRST_ITEM);
		if (lib->select) {
			for (int i = 0; i < item_count(menu); ++i) {
				if (i > lib->select_pos) {
					set_item_value(items[i], false);
				}
				if (i < lib->select_pos) {
					set_item_value(items[i], true);
				}
			}
		}
	} else if (c == cfg->key.command) {
		init_command_mode(cfg, lib);
	} else if (c == cfg->key.end) {
		menu_driver(menu, REQ_LAST_ITEM);
		if (lib->select) {
			for (int i = 0; i < item_count(menu); ++i) {
				if (i > lib->select_pos) {
					set_item_value(items[i], true);
				}
				if (i < lib->select_pos) {
					set_item_value(items[i], false);
				}
			}
		}
	} else if (c == cfg->key.move_backward) {
		if (cfg->view == V_PATH) {
			cur = current_item(menu);
			path = item_description(cur);
			move_menu_path_backward(lib);
		} else if (cfg->view == V_DATA) {
			move_menu_meta_backward(lib);
		}
	} else if (c == cfg->key.move_down) {
		if (lib->select) {
			cur = current_item(menu);
			int cur_pos = item_index(cur);
			if (cur_pos > lib->select_pos) {
				set_item_value(cur, true);
			}
			if (cur_pos < lib->select_pos) {
				set_item_value(cur, false);
			}
		}
		menu_driver(menu, REQ_NEXT_ITEM);
		if (lib->select) {
			cur = current_item(menu);
			set_item_value(cur, true);
		}
	} else if (c == cfg->key.move_forward) {
		cur = current_item(menu);
		if (cfg->view == V_PATH) {
			path = item_description(cur);
			move_menu_path_forward(path, cfg, lib);
		} else if (cfg->view == V_DATA) {
			move_menu_meta_forward(cfg, lib);
		}
	} else if (c == cfg->key.move_up) {
		if (lib->select) {
			cur = current_item(menu);
			int cur_pos = item_index(cur);
			if (cur_pos > lib->select_pos) {
				set_item_value(cur, false);
			}
			if (cur_pos < lib->select_pos) {
				set_item_value(cur, true);
			}
		}
		menu_driver(menu, REQ_PREV_ITEM);
		if (lib->select) {
			cur = current_item(menu);
			set_item_value(cur, true);
		}
	} else if (c == cfg->key.mpv_kill) {
		++lib->mpv_kill;
	} else if (c == cfg->key.mute) {
		if (lib->mpv_active) {
			mpv_command_string(lib->ctx, "cycle mute");
		}
	} else if (c == cfg->key.page_down) {
		if (lib->select) {
			cur = current_item(menu);
			init_pos = item_index(cur);
		}
		menu_driver(menu, REQ_SCR_DPAGE);
		if (lib->select) {
			cur = current_item(menu);
			end_pos = item_index(cur);
			for (int i = init_pos; i <= end_pos; ++i) {
				if (i > lib->select_pos) {
					set_item_value(items[i], true);
				}
				if (i < lib->select_pos) {
					set_item_value(items[i], false);
				}
			}
		}
	} else if (c == cfg->key.page_up) {
		if (lib->select) {
			cur = current_item(menu);
			init_pos = item_index(cur);
		}
		menu_driver(menu, REQ_SCR_UPAGE);
		if (lib->select) {
			cur = current_item(menu);
			end_pos = item_index(cur);
			for (int i = end_pos; i <= init_pos; ++i) {
				if (i > lib->select_pos) {
					set_item_value(items[i], false);
				}
				if (i < lib->select_pos) {
					set_item_value(items[i], true);
				}
			}
		}
	} else if (c == cfg->key.playnext) {
		if (lib->mpv_active) {
			mpv_command_string(lib->ctx, "playlist-next");
		}
	} else if (c == cfg->key.playpause) {
		if (lib->mpv_active) {
			mpv_command_string(lib->ctx, "cycle pause");
		}
	} else if (c == cfg->key.playprev) {
		if (lib->mpv_active) {
			mpv_command_string(lib->ctx, "playlist-prev");
		}
	} else if (c == cfg->key.queue) {
		cur = current_item(menu);
		menu_driver(menu, REQ_TOGGLE_ITEM);
	} else if (c == cfg->key.queue_all) {
		lib->select = 1;
		cur = current_item(menu);
		lib->select_pos = item_index(cur);
		for (int i = 0; i < item_count(menu); ++i) {
			if (!item_value(items[i])) {
				set_item_value(items[i], true);
			}
		}
		create_visual_window(lib);
	} else if (c == cfg->key.queue_clear || c == cfg->key.escape) {
		for (int i = 0; i < item_count(menu); ++i) {
			if (item_value(items[i])) {
				set_item_value(items[i], false);
			}
		}
		destroy_visual_window(lib);
		lib->select = 0;
		lib->select_pos = 0;
	} else if (c == cfg->key.search) {
		menu_driver(menu, REQ_CLEAR_PATTERN);
		if (lib->select) {
			destroy_visual_window(lib);
		}
		lib->search = newwin(1, 0, LINES - 1, 0);
		char *entry = strdup("");
		int pos = 0;
		while (1) {
			mvwprintw(lib->search, 0, 0, "/%s\n", entry);
			char key = wgetch(lib->search);
			if (key == 127) {
				if (pos) {
					menu_driver(menu, REQ_BACK_PATTERN);
					--pos;
					remove_char(entry);
					entry = realloc(entry, sizeof(char)*(pos+1));
					menu_driver(menu, REQ_PREV_MATCH);
				}
			} else {
				menu_driver(menu, key);
				menu_driver(menu, REQ_NEXT_MATCH);
				entry = realloc(entry, sizeof(char)*(pos+2));
				append_char(entry, key);
				++pos;
			}
			wrefresh(menu_win(lib->menu[lib->depth-1]));
			if (key == cfg->key.escape || key == 10 || key == 27) {
				free(entry);
				delwin(lib->search);
				for (int i = 0; i < lib->depth; ++i) {
					unpost_menu(lib->menu[i]);
					post_menu(lib->menu[i]);
					wrefresh(menu_win(lib->menu[i]));
				}
				if (lib->select) {
					create_visual_window(lib);
				}
				break;
			}
		}
	} else if (c == cfg->key.search_next) {
		menu_driver(menu, REQ_NEXT_MATCH);
	} else if (c == cfg->key.search_prev) {
		menu_driver(menu, REQ_PREV_MATCH);
	} else if (c == cfg->key.start) {
		int n = 0;
		if (!lib->mpv_active) {
			++lib->mpv_active;
			mpv_initialize(lib->ctx);
		}
		if ((cfg->view == V_PATH) || (cfg->view == V_SONG)) {
			for (int i = 0; i < item_count(menu); ++i) {
				if (item_value(items[i])) {
					path = item_description(items[i]);
					mpv_queue(lib->ctx, path);
					++n;
				}
			}
			if (!n) {
				ITEM *cur = current_item(menu);
				path = item_description(cur);
				mpv_queue(lib->ctx, path);
			}
		}
		if (cfg->view == V_DATA) {
			for (int i = 0; i < item_count(menu); ++i) {
				if (item_value(items[i])) {
					if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
						path = item_description(items[i]);
						mpv_queue(lib->ctx, path);
						++n;
					} else {
						name = item_name(items[i]);
						meta_path_find(cfg, lib, name, i);
						++n;
					}
				}
			}
			if (!n) {
				ITEM *cur = current_item(menu);
				int index = item_index(cur);
				if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
					path = item_description(cur);
					mpv_queue(lib->ctx, path);
				} else {
					name = item_name(cur);
					meta_path_find(cfg, lib, name, index);
				}
			}
		}
	} else if (c == cfg->key.visual) {
		if (lib->select) {
			lib->select = 0;
			lib->select_pos = 0;
			destroy_visual_window(lib);
		} else {
			menu_driver(menu, REQ_CLEAR_PATTERN);
			create_visual_window(lib);
			lib->select = 1;
			cur = current_item(menu);
			set_item_value(cur, true);
			lib->select_pos = item_index(cur);
		}
	} else if (c == cfg->key.vmn_quit) {
		++lib->vmn_quit;
	} else if (c == cfg->key.vmn_refresh) {
		if (cfg->view == V_DATA) {
			vmn_library_refresh(lib, cfg->tags[lib->depth-1]);
			vmn_library_sort(lib, cfg->lib_dir);
		}
	} else if (c == cfg->key.voldown) {
		if (lib->mpv_active) {
			const char *cmd[] = {"add", "volume", "-2", NULL};
			mpv_command(lib->ctx, cmd);
		}
	} else if (c == cfg->key.volup) {
		if (lib->mpv_active) {
			const char *cmd[] = {"add", "volume", "2", NULL};
			mpv_command(lib->ctx, cmd);
		}
	} else {
		;
	}
	wrefresh(menu_win(lib->menu[lib->depth-1]));
	if (lib->select) {
		delwin(lib->visual);
		create_visual_window(lib);
	}
}

void meta_path_find(struct vmn_config *cfg, struct vmn_library *lib, const char *name, int index) {
	char *cur = malloc(4096*sizeof(char));
	char **split;
	char *home = getenv("HOME"); 
	const char *cfg_path = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg_path) + 1);
	strcpy(path, home);
	strcat(path, cfg_path);
	FILE *cache = fopen(path, "r");
	char *name_dup = strdup(name);
	for (int i = 0; i < lib->length; ++i) {
		fgets(cur, 4096, cache);
		split = line_split(cur, "\t");
		int len = 0;
		for (int i = 0; i < strlen(cur); ++i) {
			if (cur[i] == '\t') {
				++len;
			}
		}
		++len;
		int prev;
		if (lib->depth-1) {
			prev = check_vmn_cache(lib, cur, cfg->tags);
		}
		if (prev || lib->depth == 1) {
			int known = is_known(cfg->tags[lib->depth-1], cur);
			if (!known) {
				if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
					char *filename = strrchr(split[0], '/');
					if (strcmp(filename+1, name) == 0) {
						mpv_queue(lib->ctx, split[0]);
					}
				} else {
					mpv_queue(lib->ctx, split[0]);
				}
			} else {
				if (strcmp(cfg->tags[lib->depth-1], "title") == 0) {
					int track_known = is_known(cfg->tags[lib->depth-1], cur);
					if (track_known) {
						int track_number = read_vmn_cache_int(cur, "track");
						if (track_number != index+1) {
							continue;
						}
					}
				}
				char *file = get_vmn_cache_path(lib, cur, name_dup, cfg->tags[lib->depth-1]);
				if (!(strcmp(file, "") == 0)) {
					mpv_queue(lib->ctx, file);
				}
				free(file);
			}
		}
		for (int j = 0; j < len; ++j) {
			free(split[j]);
		}
		free(split);
	}
	fclose(cache);
	free(cur);
	free(name_dup);
	free(path);
}

int move_menu_meta_backward(struct vmn_library *lib) {
	if (lib->depth == 1) {
		return 0;
	}
	destroy_last_menu_meta(lib);
	--lib->depth;
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*(lib->depth));
	lib->menu = (MENU **)realloc(lib->menu, sizeof(MENU *)*(lib->depth));
	int maxx = getmaxx(stdscr);
	for (int i = 1; i < lib->depth; ++i) {
		unpost_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
		mvwin(menu_win(lib->menu[i]), 0, (int)(maxx*i)/(lib->depth));
		post_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
	}
	wrefresh(menu_win(lib->menu[lib->depth-1]));
	return 0;
}

int move_menu_path_backward(struct vmn_library *lib) {
	if (lib->depth == 1) {
		return 0;
	}
	destroy_last_menu_path(lib);
	--lib->depth;
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*(lib->depth));
	lib->menu = (MENU **)realloc(lib->menu, sizeof(MENU *)*(lib->depth));
	int maxx = getmaxx(stdscr);
	for (int i = 1; i < lib->depth; ++i) {
		unpost_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
		mvwin(menu_win(lib->menu[i]), 0, (int)(maxx*i)/(lib->depth));
		post_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
	}
	wrefresh(menu_win(lib->menu[lib->depth-1]));
	return 0;
}

int move_menu_meta_forward(struct vmn_config *cfg, struct vmn_library *lib) {
	++lib->depth;
	if (lib->depth > cfg->tags_len) {
		--lib->depth;
		return 0;
	}
	lib->entries = (char ****)realloc(lib->entries, sizeof(char ***)*(lib->depth));
	lib->entries[lib->depth-1] = get_metadata(cfg, lib);
	int maxx = getmaxx(stdscr);
	for (int i = 1; i < lib->depth-1; ++i) {
		mvwin(menu_win(lib->menu[i]), 0, (int)(maxx*i)/(lib->depth));
		wrefresh(menu_win(lib->menu[i]));
	}
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*(lib->depth));
	lib->items[lib->depth-1] = create_meta_items(lib->entries[lib->depth-1]);
	lib->menu = (MENU **)realloc(lib->menu, sizeof(MENU *)*(lib->depth));
	lib->menu[lib->depth-1] = new_menu((ITEM **)lib->items[lib->depth-1]);
	set_menu_format(lib->menu[lib->depth-1], LINES, 0);
	menu_opts_off(lib->menu[lib->depth-1], O_ONEVALUE);
	menu_opts_off(lib->menu[lib->depth-1], O_SHOWDESC);
	WINDOW *win = newwin(0, 0, 0, (int)((maxx*(lib->depth-1))/lib->depth));
	set_menu_win(lib->menu[lib->depth-1], win);
	set_menu_sub(lib->menu[lib->depth-1], win);
	post_menu(lib->menu[lib->depth-1]);
	wmove(win, 0, 0);
	wrefresh(win);
	return 0;
}

int move_menu_path_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib) {
	char *ext = get_file_ext(path);
	if (ext_valid(ext)) {
		return 0;
	}
	++lib->depth;
	lib->entries = (char ****)realloc(lib->entries, sizeof(char ***)*(lib->depth));
	lib->entries[lib->depth-1] = get_lib_dir(path, lib);
	if (!lib->entries[lib->depth-1]) {
		--lib->depth;
		lib->entries = (char ****)realloc(lib->entries, sizeof(char ***)*(lib->depth));
		return 0;
	}
	int maxx = getmaxx(stdscr);
	for (int i = 1; i < lib->depth-1; ++i) {
		mvwin(menu_win(lib->menu[i]), 0, (int)(maxx*i)/(lib->depth));
		wrefresh(menu_win(lib->menu[i]));
	}
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*(lib->depth));
	lib->items[lib->depth-1] = create_path_items(lib->entries[lib->depth-1]);
	lib->menu = (MENU **)realloc(lib->menu, sizeof(MENU *)*(lib->depth));
	lib->menu[lib->depth-1] = new_menu((ITEM **)lib->items[lib->depth-1]);
	set_menu_format(lib->menu[lib->depth-1], LINES, 0);
	menu_opts_off(lib->menu[lib->depth-1], O_ONEVALUE);
	menu_opts_off(lib->menu[lib->depth-1], O_SHOWDESC);
	WINDOW *win = newwin(0, 0, 0, (int)((maxx*(lib->depth-1))/lib->depth));
	set_menu_win(lib->menu[lib->depth-1], win);
	set_menu_sub(lib->menu[lib->depth-1], win);
	post_menu(lib->menu[lib->depth-1]);
	wmove(win, 0, 0);
	wrefresh(win);
	return 0;
}

mpv_handle *mpv_generate(struct vmn_config *cfg) {
	mpv_handle *ctx = mpv_create();
	mpv_set_opts(ctx, cfg);
	return ctx;
}

void mpv_queue(mpv_handle *ctx, const char *audio) {
	const char *cmd[] = {"loadfile", audio, "append-play", NULL};
	mpv_command(ctx, cmd);
}

int path_in_lib(char *path, struct vmn_library *lib) {
	int len = strlen(path);
	for (int i = 0; i < lib->length; ++i) {
		if (strncmp(path, lib->files[i], len) == 0) {
			return 1;
		}
	}
	return 0;
}

void resize_detected() {
	resize = 1;
}

void sort_select(struct vmn_config *cfg, struct vmn_library *lib, char ***metadata, int len) {
	if (cfg->sort[lib->depth-1] == S_DATA) {
		qsort(metadata[0], len, sizeof(char *), qstrcmp);
	} else if (cfg->sort[lib->depth-1] == S_FILE) {
	} else if (cfg->sort[lib->depth-1] == S_NONE) {
		;
	} else if (cfg->sort[lib->depth-1] == S_NUMB) {
		if (strcasecmp(cfg->tags[lib->depth-1], "title") == 0) {
			int **order = trackorder(cfg, lib, metadata, len);
			tracksort(metadata, order, len);
			for (int i = 0; i < len; ++i) {
				free(order[i]);
			}
			free(order);
		}
	} else if (cfg->sort[lib->depth-1] == S_RAND) {
	}
}

int **trackorder(struct vmn_config *cfg, struct vmn_library *lib, char ***metadata, int len) {
	char *cur = malloc(4096*sizeof(char));
	char *home = getenv("HOME"); 
	const char *cfg_path = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg_path) + 1);
	strcpy(path, home);
	strcat(path, cfg_path);
	FILE *cache = fopen(path, "r");
	int **positions = (int **)malloc(sizeof(int *)*len);
	for (int i = 0; i < len; ++i) {
		positions[i] = (int *)malloc(sizeof(int)*2);
	}
	int n = 0;
	for (int i = 0; i < lib->length; ++i) {
		fgets(cur, 4096, cache);
		int match = is_known(metadata[0][n], cur);
		if (match) {
			int prev = check_vmn_cache(lib, cur, cfg->tags);
			if (prev) {
				int disc_known = is_known("disc", cur);
				int track_known = is_known("track", cur);
				if (disc_known) {
					char *disc = read_vmn_cache(cur, "disc");
					positions[n][0] = atoi(disc);
					free(disc);
				} else {
					positions[n][0] = -1;
				}
				if (track_known) {
					char *track = read_vmn_cache(cur, "track");
					positions[n][1] = atoi(track);
					free(track);
				} else {
					positions[n][1] = -1;
				}
				++n;
				if (n == len) {
					break;
				}
			}
		}
	}
	fclose(cache);
	free(cur);
	free(path);
	return positions;
}

void tracksort(char ***metadata, int **order, int len) {
	int swap = 1;
	while (swap) {
		for (int i = 0; i < len; ++i) {
			if (i == 0) {
				swap = 0;
			}
			if (i == (len - 1)) {
				break;
			}
			if (order[i][0] > order[i+1][0]) {
				int disc_tmp = order[i][0];
				int track_tmp = order[i][1];
				order[i][0] = order[i+1][0];
				order[i][1] = order[i+1][1];
				order[i+1][0] = disc_tmp;
				order[i+1][1] = track_tmp;
				char **tmp = malloc(sizeof(char *)*2);
				tmp[0] = malloc(sizeof(char)*(strlen(metadata[0][i])+1));
				tmp[1] = malloc(sizeof(char)*(strlen(metadata[1][i])+1));
				strcpy(tmp[0], metadata[0][i]);
				strcpy(tmp[1], metadata[1][i]);
				metadata[0][i] = (char *)realloc(metadata[0][i], sizeof(char)*(strlen(metadata[0][i+1])+1));
				strcpy(metadata[0][i], metadata[0][i+1]);
				metadata[1][i] = (char *)realloc(metadata[1][i], sizeof(char)*(strlen(metadata[1][i+1])+1));
				strcpy(metadata[1][i], metadata[1][i+1]);
				metadata[0][i+1] = (char *)realloc(metadata[0][i+1], sizeof(char)*(strlen(tmp[0])+1));
				metadata[1][i+1] = (char *)realloc(metadata[1][i+1], sizeof(char)*(strlen(tmp[1])+1));
				strcpy(metadata[0][i+1], tmp[0]);
				strcpy(metadata[1][i+1], tmp[1]);
				free(tmp[0]);
				free(tmp[1]);
				free(tmp);
				swap = 1;
				continue;
			}
			if (order[i][1] > order[i+1][1]) {
				if (order[i][0] != order[i+1][0]) {
					continue;
				}
				int disc_tmp = order[i][0];
				int track_tmp = order[i][1];
				order[i][0] = order[i+1][0];
				order[i][1] = order[i+1][1];
				order[i+1][0] = disc_tmp;
				order[i+1][1] = track_tmp;
				char **tmp = malloc(sizeof(char *)*2);
				tmp[0] = malloc(sizeof(char)*(strlen(metadata[0][i])+1));
				tmp[1] = malloc(sizeof(char)*(strlen(metadata[1][i])+1));
				strcpy(tmp[0], metadata[0][i]);
				strcpy(tmp[1], metadata[1][i]);
				metadata[0][i] = (char *)realloc(metadata[0][i], sizeof(char)*(strlen(metadata[0][i+1])+1));
				strcpy(metadata[0][i], metadata[0][i+1]);
				metadata[1][i] = (char *)realloc(metadata[1][i], sizeof(char)*(strlen(metadata[1][i+1])+1));
				strcpy(metadata[1][i], metadata[1][i+1]);
				metadata[0][i+1] = (char *)realloc(metadata[0][i+1], sizeof(char)*(strlen(tmp[0])+1));
				metadata[1][i+1] = (char *)realloc(metadata[1][i+1], sizeof(char)*(strlen(tmp[1])+1));
				strcpy(metadata[0][i+1], tmp[0]);
				strcpy(metadata[1][i+1], tmp[1]);
				free(tmp[0]);
				free(tmp[1]);
				free(tmp);
				swap = 1;
				continue;
			}
		}
	}
}
