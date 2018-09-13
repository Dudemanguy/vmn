#include <dirent.h>
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
	free(lib->files);
}
