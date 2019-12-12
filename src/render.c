#include <menu.h>
#include <mpv/client.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "library.h"

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

void create_visual_window(struct vmn_library *lib) {
	lib->visual = newwin(1, 0, LINES - 1, 0);
	mvwprintw(lib->visual, 0, 0, "-- VISUAL --\n");
	wrefresh(lib->visual);
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
