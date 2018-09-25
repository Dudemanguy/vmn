#include <dirent.h>
#include <menu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"

struct vmn_library lib_init() {
	struct vmn_library lib;
	int entries_allocated = 1000;
	lib.files = (char **)malloc(sizeof(char*)*entries_allocated);
	lib.mem_size = 1000;
	lib.length = 0;
	lib.depth = 0;
	lib.mpv_active = 0;
	return lib;
}

void vmn_library_add(struct vmn_library *lib, char *entry) {
	int max_line_len = 4096;
	int length = lib->length;
	if (lib->length >= lib->mem_size) {
		int new_size = lib->mem_size*2;
		lib->files = (char **)realloc(lib->files,sizeof(char*)*new_size);
		lib->mem_size = new_size;
	}
	lib->files[length] = malloc(max_line_len);
	strcpy(lib->files[length], entry);
	lib->length = length + 1;
}


void vmn_library_destroy(struct vmn_library *lib) {
	for (int i = 0; i < lib->length; ++i) {
		free(lib->files[i]);
	}
	free(lib->files);
	if (lib->length) {
		for (int i = 0; i < (lib->depth + 1); ++i) {
			int j = 0;
			while (lib->entries[i][0][j]) {
				free(lib->entries[i][0][j]);
				free(lib->entries[i][1][j]);
				++j;
			}
			free(lib->entries[i][0]);
			free(lib->entries[i][1]);
			free(lib->entries[i]);
		}
		free(lib->entries);
	}
	if (lib->length) {
		for (int i = 0; i < lib->depth + 1; ++i) {
			unpost_menu(lib->menu[i]);
			free_menu(lib->menu[i]);
		}
		free(lib->menu);
	}
	if (lib->length) {
		for (int i = 0; i < lib->depth + 1; ++i) {
			int j = 0;
			while(lib->items[i][j]) {
				free_item(lib->items[i][j]);
				++j;
			}
			free(lib->items[i]);
		}
		free(lib->items);
	}
}
