#include <menu.h>
#include <mpv/client.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "library.h"
#include "utils.h"

void destroy_command_window(struct vmn_library *lib) {
	delwin(lib->command);
	for (int i = 0; i <= lib->depth; ++i) {
		unpost_menu(lib->menu[i]);
		post_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
	}
}

void execute_command(struct vmn_library *lib, char **parse_arr, char *entry) {
	int len = 0;
	for (int i = 0; i < strlen(entry); ++i) {
		if ((entry[i] == ' ') && (entry[i-1] != ' ')) {
			++len;
		}
	}
	++len;
	if (strcmp(parse_arr[0], "mpv") == 0) {
		if (strcmp(parse_arr[1], "cmd") == 0) {
			const char *cmd[len-1];
			for (int i = 0; i < len - 2; ++i) {
				cmd[i] = parse_arr[i+2];
			}
			cmd[len-2] = NULL;
			mpv_command(lib->ctx, cmd);
		} else if (strcmp(parse_arr[1], "set") == 0) {
			mpv_set_option_string(lib->ctx, parse_arr[2], parse_arr[3]);
		}
	}
	for (int i = 0; i < len; ++i) {
		free(parse_arr[i]);
	}
	free(parse_arr);
}

char **parse_command(char *entry) {
	char **split = line_split(entry, " ");
	int len = 0;
	for (int i = 0; i < strlen(entry); ++i) {
		if ((entry[i] == ' ') && (entry[i-1] != ' ')) {
			++len;
		}
	}
	++len;
	char **parse_arr = (char **)calloc(len, sizeof(char*));
	for (int i = 0; i < len; ++i) {
		parse_arr[i] = malloc(strlen(split[i]) + 1);
		parse_arr[i] = remove_spaces(split[i]);
	}
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	return parse_arr;
}

void init_command_mode(struct vmn_config *cfg, struct vmn_library *lib) {
	lib->command = newwin(1, 0, LINES - 1, 0);
	char *entry = "";
	while (1) {
		mvwprintw(lib->command, 0, 0, ":%s\n", entry);
		char key = wgetch(lib->command);
		if (key == 127) {
			entry = remove_char(entry);
		} else if (key == 10) {
			if (strlen(entry) && (entry[0] != ' ')) {
				char **parse_arr = parse_command(entry);
				execute_command(lib, parse_arr, entry);
				free(entry);
			}
			destroy_command_window(lib);
			break;
		} else if (key == cfg->key.escape || key == 27) {
			if (strlen(entry)) {
				free(entry);
			}
			destroy_command_window(lib);
			break;
		} else {
			entry = append_char(entry, key);
		}
	}
}
