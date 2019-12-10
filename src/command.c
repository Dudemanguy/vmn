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
	for (int i = 0; i < lib->depth; ++i) {
		unpost_menu(lib->menu[i]);
		post_menu(lib->menu[i]);
		wrefresh(menu_win(lib->menu[i]));
	}
}

const char *mpv_err_msg(int err_index) {
	const char *err_msg;
	switch(err_index) {
		case -1:
			err_msg = "Error: The event ringbuffer is full.";
			break;
		case -2:
			err_msg = "Error: Memory allocation failed.";
			break;
		case -3:
			err_msg = "Error: The mpv core is uninitalized.";
			break;
		case -4:
			err_msg = "Error: An invalid parameter was used.";
			break;
		case -5:
			err_msg = "Error: Trying to set an option that doesn't exist.";
			break;
		case -6:
			err_msg = "Error: Trying to set an option using an unsupported MPV_FORMAT.";
			break;
		case -7:
			err_msg = "Error: Setting the option failed.";
			break;
		case -8:
			err_msg = "Error: The accessed property doesn't exist.";
			break;
		case -9:
			err_msg = "Error: Trying to set or get a property using an unsupported MPV_FORMAT.";
			break;
		case -10:
			err_msg = "Error: This property exists, but it is not available.";
			break;
		case -11:
			err_msg = "Error: Cannot set or get property.";
			break;
		case -12:
			err_msg = "Error: Problem when trying to run mpv_command.";
			break;
		case -13:
			err_msg = "Error: Problem when trying to load.";
			break;
		case -14:
			err_msg = "Error: Initializing the audio output failed.";
			break;
		case -15:
			err_msg = "Error: Initializing the video output failed.";
			break;
		case -16:
			err_msg = "Error: No audio or video data to play.";
			break;
		case -17:
			err_msg = "Error: File format is unknown.";
			break;
		case -18:
			err_msg = "Error: Certain system requirements are not fulfilled.";
			break;
		case -19:
			err_msg = "Error: This API function is only a stub.";
			break;
		case -20:
			err_msg = "Error: Unspecified.";
			break;
	}
	return err_msg;
}

const char *execute_command(struct vmn_config *cfg, struct vmn_library *lib, char **parse_arr, int len) {
	if (strcmp(parse_arr[0], "mpv") == 0) {
		int mpv_err = 0;
		if (strcmp(parse_arr[1], "cmd") == 0) {
			const char *cmd[len-1];
			for (int i = 0; i < len - 2; ++i) {
				cmd[i] = parse_arr[i+2];
			}
			cmd[len-2] = NULL;
			mpv_err = mpv_command(lib->ctx, cmd);
		} else if (strcmp(parse_arr[1], "set") == 0) {
			mpv_err = mpv_set_option_string(lib->ctx, parse_arr[2], parse_arr[3]);
			if (!mpv_err) {
				mpv_cfg_add(cfg, parse_arr[2], parse_arr[3]);
			}
		}
		if (mpv_err < 0) {
			const char *err_msg = mpv_err_msg(mpv_err);
			return err_msg;
		}
	}
	return "";
}

char **parse_command(char *entry, int len) {
	char **split = line_split(entry, " ");
	char **parse_arr = (char **)calloc(len, sizeof(char*));
	for (int i = 0; i < len; ++i) {
		char *tmp = remove_spaces(split[i]);
		parse_arr[i] = malloc(sizeof(char)*(strlen(split[i])+1));
		strcpy(parse_arr[i], tmp);
		free(tmp);
	}
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	return parse_arr;
}

void init_command_mode(struct vmn_config *cfg, struct vmn_library *lib) {
	lib->command = newwin(1, 0, LINES - 1, 0);
	char *entry = strdup("");
	int pos = 0;
	const char *err_msg = "";
	while (1) {
		mvwprintw(lib->command, 0, 0, ":%s\n", entry);
		char key = wgetch(lib->command);
		if (key == 127) {
			if (pos) {
				--pos;
				remove_char(entry);
				entry = realloc(entry, sizeof(char)*(pos+1));
			}
		} else if (key == 10) {
			if (strlen(entry) && (entry[0] != ' ')) {
				int len = 0;
				for (int i = 0; i < strlen(entry); ++i) {
					if ((entry[i] == ' ') && (entry[i-1] != ' ')) {
						++len;
					}
				}
				++len;
				char **parse_arr = parse_command(entry, len);
				err_msg = execute_command(cfg, lib, parse_arr, len);
				free(entry);
				for (int i = 0; i < len; ++i) {
					free(parse_arr[i]);
				}
				free(parse_arr);
				if (strlen(err_msg)) {
					destroy_command_window(lib);
					lib->command = newwin(1, 0, LINES - 1, 0);
					mvwprintw(lib->command, 0, 0, err_msg);
					wrefresh(lib->command);
					break;
				}
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
			entry = realloc(entry, sizeof(char)*(pos+2));
			append_char(entry, key);
			++pos;
		}
	}
	if (!(strcmp(err_msg, "") == 0)) {
		while (1) {
			char key = wgetch(lib->command);
			if (key) {
				destroy_command_window(lib);
				break;
			}
		}
	}
}
