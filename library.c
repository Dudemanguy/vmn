#include <dirent.h>
#include <menu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"

struct vmn_library lib_init() {
	struct vmn_library lib;
	lib.depth = 0;
	lib.files = (char **)malloc(sizeof(char *)*(1));
	lib.length = 0;
	lib.mpv_active = 0;
	return lib;
}

void vmn_library_add(struct vmn_library *lib, char *entry) {
	lib->files = (char **)realloc(lib->files,sizeof(char*)*(lib->length + 1));
	lib->files[lib->length] = malloc(strlen(entry) + 1);
	strcpy(lib->files[lib->length], entry);
	++lib->length;
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
