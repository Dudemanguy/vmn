#include <dirent.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
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

ITEM **create_meta_items(char **metadata);
ITEM **create_path_items(char ***files);
void input_mode(struct vmn_config *cfg);
char **get_base_metadata(struct vmn_config *cfg, struct vmn_library *lib);
char ***get_lib_dir(const char *library, struct vmn_library *lib);
int get_music_files(const char *library, struct vmn_library *lib);
void key_event(int c, MENU *menu, ITEM **items, struct vmn_config *cfg, struct vmn_library *lib);
void meta_path_find_multiple(struct vmn_config *cfg, struct vmn_library *lib, char **names, int len);
void meta_path_find_single(struct vmn_config *cfg, struct vmn_library *lib, const char *name);
int move_menu_meta_backward(struct vmn_library *lib);
int move_menu_path_backward(struct vmn_library *lib);
int move_menu_meta_forward(struct vmn_config *cfg, struct vmn_library *lib);
int move_menu_path_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib);
mpv_handle *mpv_generate(struct vmn_config *cfg);
void mpv_queue(mpv_handle *ctx, const char *audio);
int path_in_lib(char *path, struct vmn_library *lib);
int qstrcmp(const void *a, const void *b);
char *remove_char(char *str);
void sort_select(struct vmn_config *cfg, char **metadata, int len, int depth);

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");
	struct vmn_config cfg = cfg_init(argc, argv);
	struct vmn_library lib = lib_init();
	if (strcmp(cfg.input_mode, "yes") == 0) {
		input_mode(&cfg);
	}
	int invalid = get_music_files(cfg.lib_dir, &lib);
	qsort(lib.files, lib.length, sizeof(char *), qstrcmp);
	if (invalid) {
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
		lib.entries[0] = (char ***)calloc(1, sizeof(char **));
		vmn_library_metadata(&lib);
		lib.selections = (char **)calloc(cfg.tags_len, sizeof(char *));
		lib.entries[0][0] = get_base_metadata(&cfg, &lib);
		lib.items = (ITEM ***)calloc(1, sizeof(ITEM **));
		lib.items[0] = create_meta_items(lib.entries[0][0]);
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
		mpv_event *event = mpv_wait_event(lib.ctx, 0);
		if (event->event_id == MPV_EVENT_SHUTDOWN) {
			mpv_terminate_destroy(lib.ctx);
			lib.mpv_active = 0;
			lib.ctx = mpv_generate(&cfg);
		}
		if (event->event_id == MPV_EVENT_END_FILE) {
			char *idle = mpv_get_property_string(lib.ctx, "idle-active");
			if (strcmp(idle, "yes") == 0) {
				mpv_terminate_destroy(lib.ctx);
				lib.mpv_active = 0;
				lib.ctx = mpv_generate(&cfg);
			}
		}
		MENU *menu = lib.menu[lib.depth];
		ITEM **items = lib.items[lib.depth];
		c = wgetch(win);
		key_event(c, menu, items, &cfg, &lib);
		if (cfg.view == V_DATA) {
			ITEM *cur = current_item(lib.menu[lib.depth]);
			const char *name = item_name(cur);
			vmn_library_selections_add(&lib, name);
		}
		if (lib.mpv_kill) {
			mpv_terminate_destroy(lib.ctx);
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
		vmn_library_destroy_path(&lib);
	}
	if (cfg.view == V_DATA) {
		vmn_library_destroy_meta(&lib);
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

char *append_char(char *str, char c) {
	int len = strlen(str);
	char *append = malloc(len + 2);
	strcpy(append, str);
	append[len] = c;
	append[len + 1] = '\0';
	return append;
}

ITEM **create_meta_items(char **metadata) {
	ITEM **items;
	int n = 0;
	while (metadata[n]) {
		++n;
	}
	items = (ITEM **)calloc(n+1, sizeof(ITEM *));
	for (int i = 0; i < n; ++i) {
		items[i] = new_item(metadata[i], metadata[i]);
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
	int n = item_count(lib->menu[lib->depth]);
	unpost_menu(lib->menu[lib->depth]);
	wrefresh(menu_win(lib->menu[lib->depth]));
	delwin(menu_win(lib->menu[lib->depth]));
	free_menu(lib->menu[lib->depth]);
	for (int i = 0; i < n; ++i) {
		free_item(lib->items[lib->depth][i]);
		free(lib->entries[0][lib->depth][i]);
	}
	free(lib->items[lib->depth]);
	free(lib->entries[0][lib->depth]);
}

void destroy_last_menu_path(struct vmn_library *lib) {
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

char **get_base_metadata(struct vmn_config *cfg, struct vmn_library *lib) {
	int len = 0;
	int match = 0;
	int unknown = 0;
	char **metadata = (char **)calloc(lib->length + 1, sizeof(char *));
	for (int i = 0; i < lib->length; ++i) {
		AVDictionaryEntry *tag = NULL;
		tag = av_dict_get(lib->dict[i], cfg->tags[0], tag, 0);
		if (!tag) {
			if (!unknown && (!(strcmp(cfg->tags[0], "title") == 0))) {
				metadata[len] = (char *)calloc(strlen(cfg->tags[0]) + strlen("Unknown ") + 1, sizeof(char));
				strcpy(metadata[len], "Unknown ");
				strcat(metadata[len], cfg->tags[0]);
				unknown = 1;
				++len;
			}
			if (!unknown && (strcmp(cfg->tags[0], "title") == 0)) {
				char *filename = strrchr(lib->files[i], '/');
				metadata[len] = (char *)calloc(strlen(filename+1) + 1, sizeof(char));
				strcpy(metadata[len], filename+1);
				++len;
			}
			continue;
		}
		for (int j = 0; j < len; ++j) {
			if (strcmp(tag->value, metadata[j]) == 0) {
				match = 1;
				break;
			}
		}
		if (match) {
			match = 0;
			continue;
		} else {
			metadata[len] = malloc(sizeof(char *)*(strlen(tag->value) + 1));
			strcpy(metadata[len], tag->value);
			++len;
		}
	}
	metadata[len] = '\0';
	metadata = (char **)realloc(metadata,sizeof(char *)*(len+1));
	sort_select(cfg, metadata, len, lib->depth);
	return metadata;
}

char **get_next_metadata(struct vmn_config *cfg, struct vmn_library *lib) {
	int *index = (int *)calloc(lib->length + 1, sizeof(int));
	for (int i = 0; i < lib->length; ++i) {
		for (int j = 0; j < lib->depth; ++j) {
			AVDictionaryEntry *tag = NULL;
			tag = av_dict_get(lib->dict[i], cfg->tags[j], tag, 0);
			if (j == 0) {
				if (!tag) {
					char *unknown_tag = (char *)calloc(strlen(cfg->tags[0]) + strlen("Unknown ") + 1, sizeof(char));
					strcpy(unknown_tag, "Unknown ");
					strcat(unknown_tag, cfg->tags[0]);
					if (strcmp(unknown_tag, lib->selections[0]) == 0) {
						index[i] = 1;
					} else {
						index[i] = 0;
					}
					free(unknown_tag);
					continue;
				}
				if ((strcasecmp(tag->key, cfg->tags[j]) == 0) && (strcmp(tag->value, lib->selections[j]) == 0)) {
					index[i] = 1;
				} else {
					index[i] = 0;
				}
			} else {
				if (index[i]) {
					if (!tag) {
						char *unknown_tag = (char *)calloc(strlen(cfg->tags[j]) + strlen("Unknown ") + 1, sizeof(char));
						strcpy(unknown_tag, "Unknown ");
						strcat(unknown_tag, cfg->tags[j]);
						if (strcmp(unknown_tag, lib->selections[j]) == 0) {
							index[i] = 1;
						} else {
							index[i] = 0;
						}
						free(unknown_tag);
						continue;
					}
					if ((strcasecmp(tag->key, cfg->tags[j]) == 0) && (strcmp(tag->value, lib->selections[j]) == 0)) {
						index[i] = 1;
					} else {
						index[i] = 0;
					}
				}
			}
		}
	}
	int len = 0;
	int match = 0;
	char **metadata = (char **)calloc(lib->length + 1, sizeof(char *));
	for (int i = 0; i < lib->length; ++i ) {
		AVDictionaryEntry *tag = NULL;
		tag = av_dict_get(lib->dict[i], cfg->tags[lib->depth], tag, 0);
		if (index[i]) {
			if (!tag) {
				if (strcmp(cfg->tags[lib->depth], "title") == 0) {
					char *filename = strrchr(lib->files[i], '/');
					metadata[len] = (char *)calloc(strlen(filename+1) + 1, sizeof(char));
					strcpy(metadata[len], filename+1);
					++len;
					continue;
				} else {
					metadata[len] = (char *)calloc(strlen(cfg->tags[lib->depth]) + strlen("Unknown ") + 1, sizeof(char));
					strcpy(metadata[len], "Unknown ");
					strcat(metadata[len], cfg->tags[lib->depth]);
					++len;
					continue;
				}
				continue;
			}
			for (int j = 0; j < len; ++j) {
				if (strcasecmp(tag->value, metadata[j]) == 0) {
					match = 1;
					break;
				}
			}
			if (match) {
				match = 0;
				continue;
			} else {
				metadata[len] = (char *)calloc(strlen(tag->value) + 1, sizeof(char));
				strcpy(metadata[len], tag->value);
				++len;
			}
		}
	}
	metadata[len] = '\0';
	metadata = (char **)realloc(metadata, sizeof(char *)*(len+1));
	sort_select(cfg, metadata, len, lib->depth);
	free(index);
	return metadata;
	return 0;
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

void input_mode(struct vmn_config *cfg) {
		int key;
		initscr();
		noecho();
		keypad(stdscr, TRUE);
		while (1) {
			mvprintw(0, 0, "Input mode is enabled. Keycodes will be returned on the screen. Use quit to exit.\n");
			key = getch();
			mvprintw(1, 0, "Key = %d\n", key);
			if (key == cfg->key.quit) {
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
	} else if (c == cfg->key.end) {
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
	} else if (c == cfg->key.move_backward) {
		if (cfg->view == V_PATH) {
			cur = current_item(menu);
			path = item_description(cur);
			move_menu_path_backward(lib);
		} else if (cfg->view == V_DATA) {
			move_menu_meta_backward(lib);
		}
	} else if (c == cfg->key.move_down) {
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
	} else if (c == cfg->key.move_forward) {
		if (cfg->view == V_PATH) {
			cur = current_item(menu);
			path = item_description(cur);
			move_menu_path_forward(path, cfg, lib);
		} else if (cfg->view == V_DATA) {
			move_menu_meta_forward(cfg, lib);
		}
	} else if (c == cfg->key.move_up) {
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
		if (cfg->select) {
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
	} else if (c == cfg->key.page_up) {
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
		for (int i = 0; i < item_count(menu); ++i) {
			if (!item_value(items[i])) {
				set_item_value(items[i], true);
			}
		}
	} else if (c == cfg->key.queue_clear || c == cfg->key.quit) {
		for (int i = 0; i < item_count(menu); ++i) {
			if (item_value(items[i])) {
				set_item_value(items[i], false);
			}
		}
		cfg->select = 0;
	} else if (c == cfg->key.search) {
		menu_driver(menu, REQ_CLEAR_PATTERN);
		WINDOW *wins = newwin(1, 0, LINES - 1, 0);
		char *search = "";
		while (1) {
			char key = wgetch(wins);
			if (key == 127) {
				menu_driver(menu, REQ_BACK_PATTERN);
				search = remove_char(search);
				menu_driver(menu, REQ_PREV_MATCH);
			} else {
				menu_driver(menu, key);
				menu_driver(menu, REQ_NEXT_MATCH);
				search = append_char(search, key);
			}
			wrefresh(menu_win(lib->menu[lib->depth]));
			if (key == cfg->key.quit || key == 10) {
				free(search);
				delwin(wins);
				for (int i = 0; i <= lib->depth; ++i) {
					unpost_menu(lib->menu[i]);
					post_menu(lib->menu[i]);
					wrefresh(menu_win(lib->menu[i]));
				}
				break;
			}
			mvwprintw(wins, 0, 0, "%s\n", search);
			wrefresh(wins);
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
			char **names = (char **)calloc(item_count(menu), sizeof(char*));
			for (int i = 0; i < item_count(menu); ++i) {
				if (item_value(items[i])) {
					name = item_name(items[i]);
					char *name_dup = strdup(name);
					names[n] = (char *)calloc(strlen(name) + 1, sizeof(char));
					strcpy(names[n], name_dup);
					free(name_dup);
					++n;
				}
				names[n] = '\0';
			}
			if (n) {
				meta_path_find_multiple(cfg, lib, names, n);
			} else {
				ITEM *cur = current_item(menu);
				name = item_name(cur);
				meta_path_find_single(cfg, lib, name);
			}
			for (int i = 0; i < item_count(menu); ++i) {
				free(names[i]);
			}
			free(names);
		}
	} else if (c == cfg->key.visual) {
		if (cfg->select) {
			cfg->select = 0;
			cfg->select_pos = 0;
		} else {
			cfg->select = 1;
			cur = current_item(menu);
			set_item_value(cur, true);
			cfg->select_pos = item_index(cur);
		}
	} else if (c == cfg->key.vmn_quit) {
		++lib->vmn_quit;
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
	wrefresh(menu_win(lib->menu[lib->depth]));
}

void meta_path_find_multiple(struct vmn_config *cfg, struct vmn_library *lib, char **names, int len) {
	int *index = (int *)calloc(lib->length + 1, sizeof(int));
	for (int i = 0; i < lib->length; ++i) {
		for (int j = 0; j < lib->depth; ++j) {
			if (j == 0) {
				index[i] = 1;
			}
			AVDictionaryEntry *tag = NULL;
			tag = av_dict_get(lib->dict[i], cfg->tags[j], tag, 0);
			if (!tag) {
				index[i] = 1;
				continue;
			}
			if ((strcasecmp(tag->key, cfg->tags[j]) == 0) && (strcmp(tag->value, lib->selections[j]) == 0)) {
				if (index[i]) {
					index[i] = 1;
				}
			} else {
				index[i] = 0;
			}
		} 
		if (index[i]) {
			for (int j = 0; j < len; ++j) {
				AVDictionaryEntry *tag = NULL;
				tag = av_dict_get(lib->dict[i], cfg->tags[lib->depth], tag, 0);
				if (!tag) {
					regex_t regex;
					int status;
					regcomp(&regex, names[j], 0);
					status = regexec(&regex, lib->files[i], 0, NULL, 0);
					if (status == 0) {
						mpv_queue(lib->ctx, lib->files[i]);
					}
					regfree(&regex);
					continue;
				}
				if ((strcasecmp(tag->key, cfg->tags[lib->depth]) == 0) && (strcmp(tag->value, names[j]) == 0)) {
					mpv_queue(lib->ctx, lib->files[i]);
				}
			}
		}
	}
	free(index);
}

void meta_path_find_single(struct vmn_config *cfg, struct vmn_library *lib, const char *name) {
	int *index = (int *)calloc(lib->length + 1, sizeof(int));
	for (int i = 0; i < lib->length; ++i) {
		for (int j = 0; j < (lib->depth+1); ++j) {
			if (j == 0) {
				index[i] = 1;
			}
			AVDictionaryEntry *tag = NULL;
			tag = av_dict_get(lib->dict[i], cfg->tags[j], tag, 0);
			if (!tag) {
				index[i] = 1;
				continue;
			}
			if ((strcasecmp(tag->key, cfg->tags[j]) == 0) && (strcmp(tag->value, lib->selections[j]) == 0)) {
				if (index[i]) {
					index[i] = 1;
				}
			} else {
				index[i] = 0;
			}
		}
		if (index[i]) {
			AVDictionaryEntry *tag = NULL;
			tag = av_dict_get(lib->dict[i], cfg->tags[lib->depth], tag, 0);
			if (!tag) {
				regex_t regex;
				int status;
				regcomp(&regex, name, 0);
				status = regexec(&regex, lib->files[i], 0, NULL, 0);
				if (status == 0) {
					mpv_queue(lib->ctx, lib->files[i]);
				}
				regfree(&regex);
				continue;
			}
			if ((strcasecmp(tag->key, cfg->tags[lib->depth]) == 0) && (strcmp(tag->value, name) == 0)) {
				mpv_queue(lib->ctx, lib->files[i]);
			}
		}
	}
	free(index);
}

int move_menu_meta_backward(struct vmn_library *lib) {
	if (lib->depth == 0) {
		return 0;
	}
	destroy_last_menu_meta(lib);
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
		wrefresh(menu_win(lib->menu[i]));
	}
	wrefresh(menu_win(lib->menu[lib->depth]));
	return 0;
}

int move_menu_path_backward(struct vmn_library *lib) {
	if (lib->depth == 0) {
		return 0;
	}
	destroy_last_menu_path(lib);
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
		wrefresh(menu_win(lib->menu[i]));
	}
	wrefresh(menu_win(lib->menu[lib->depth]));
	return 0;
}

int move_menu_meta_forward(struct vmn_config *cfg, struct vmn_library *lib) {
	++lib->depth;
	if (lib->depth == cfg->tags_len) {
		--lib->depth;
		return 0;
	}
	lib->entries[0] = (char ***)realloc(lib->entries[0], sizeof(char **)*(lib->depth+1));
	lib->entries[0][lib->depth] = get_next_metadata(cfg, lib);
	double startx = getmaxx(stdscr);
	for (int i = 1; i < lib->depth; ++i) {
		wresize(menu_win(lib->menu[i]), 0, (startx*i)/(lib->depth+1));
		mvwin(menu_win(lib->menu[i]), 0, (startx*i)/(lib->depth+1));
		wrefresh(menu_win(lib->menu[i]));
	}
	lib->items = (ITEM ***)realloc(lib->items, sizeof(ITEM **)*(lib->depth+1));
	lib->items[lib->depth] = create_meta_items(lib->entries[0][lib->depth]);
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

int move_menu_path_forward(const char *path, struct vmn_config *cfg, struct vmn_library *lib) {
	char *ext = get_file_ext(path);
	if (ext_valid(ext)) {
		return 0;
	}
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
	lib->items[lib->depth] = create_path_items(lib->entries[lib->depth]);
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

char *remove_char(char *str) {
	int len = strlen(str);
	if (!len) {
		return "";
	}
	char *remove;
	remove = malloc(len - 1);
	memcpy(remove, str, sizeof(char)*(len - 1));
	remove[len - 1] = '\0';
	return remove;
}

void sort_select(struct vmn_config *cfg, char **metadata, int len, int depth) {
	if (cfg->sort[depth] == S_DATA) {
		qsort(metadata, len, sizeof(char *), qstrcmp);
	} else if (cfg->sort[depth] == S_FILE) {
	} else if (cfg->sort[depth] == S_NONE) {
		;
	} else if (cfg->sort[depth] == S_NUMB) {
	} else if (cfg->sort[depth] == S_RAND) {
	}
}
