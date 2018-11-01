#include <dirent.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/file.h>
#include <menu.h>
#include <mpv/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "library.h"

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

AVInputFormat *get_input_format(const char *file) {
	char *ext = get_file_ext(file);
	AVInputFormat *format = NULL;
	if (strcmp(ext, "opus") == 0) {
		format = av_find_input_format("ogg");
		return format;
	} else {
		format = av_find_input_format(ext);
		return format;
	}
}

struct vmn_library lib_init() {
	struct vmn_library lib;
	lib.depth = 0;
	lib.length = 0;
	lib.mpv_active = 0;
	lib.mpv_kill = 0;
	lib.vmn_quit = 0;
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
		for (int i = 0; i < (lib->depth + 1); ++i) {
			unpost_menu(lib->menu[i]);
			free_menu(lib->menu[i]);
		}
		free(lib->menu);
	}
	if (lib->length) {
		for (int i = 0; i < (lib->depth + 1); ++i) {
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

void vmn_library_metadata(struct vmn_library *lib) {
	av_log_set_level(AV_LOG_QUIET);
	AVFormatContext *fmt_ctx = NULL;
	AVInputFormat *format = NULL;
	lib->dict = malloc(10*lib->length); //TODO: figure out the right way to allocate memory for this
	uint8_t *buffer = NULL;
	size_t buffer_size;
	for (int i = 0; i < lib->length; ++i) {
		struct stat st;
		stat(lib->files[i], &st);
		buffer_size = st.st_size;
		format = get_input_format(lib->files[i]);
		int ret = av_file_map(lib->files[i], &buffer, &buffer_size, 0, NULL);
		if (ret) {
			continue;
		}
		fmt_ctx = avformat_alloc_context();
		avformat_open_input(&fmt_ctx, lib->files[i], format, NULL);
		av_dict_copy(&lib->dict[i], fmt_ctx->streams[0]->metadata, 0);
		avformat_close_input(&fmt_ctx);
		av_file_unmap(buffer, buffer_size);
	}
}
