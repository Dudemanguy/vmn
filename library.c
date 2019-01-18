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

char **line_split(char *str) {
	char *str_dup = strdup(str);
	char *token = strtok(str_dup, "\t");
	int len = 20;
	char **arr = (char **)calloc(len, sizeof(char*));
	int i = 0;
	while (token != NULL) {
		if (i == len) {
			len = len*2;
			arr = (char **)realloc(arr,sizeof(char*)*len);
		}
		arr[i] = malloc(strlen(token) + 1);
		strcpy(arr[i], token);
		token = strtok(NULL, "\t");
		++i;
	}
	arr = (char **)realloc(arr, sizeof(char*)*i);
	free(str_dup);
	return arr;
}

int check_vmn_cache(struct vmn_library *lib, char *str, char **tags) {
	char **split = line_split(str);
	int len = 0;
	for (int i = 0; i < strlen(str); ++i) {
		if (str[i] == '\t') {
			++len;
		}
	}
	++len;
	int match = 0;
	for (int i = 0; i < lib->depth; ++i) {
		if (lib->unknown[i]) {
			continue;
		}
		for (int j = 0; j < len; ++j) {
			match = 0;
			if ((strcmp(lib->selections[i], split[j]) == 0) && (strcasecmp(tags[i], split[j-1]) == 0)) {
				j = 0;
				match = 1;
				break;
			}
		}
		if (!match) {
			break;
		}
	}
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	if (!match) {
		return 0;
	} else {
		return 1;
	}
}

char *get_vmn_cache_path(struct vmn_library *lib, char *line, char *name) {
	char *out = (char *)calloc(1, sizeof(char));
	char **split = line_split(line);
	int len = 0;
	for (int i = 0; i < strlen(line); ++i) {
		if (line[i] == '\t') {
			++len;
		}
	}
	++len;
	for (int i = 0; i < len; ++i) {
		if (strcmp(split[i], name) == 0) {
			out = (char *)realloc(out,sizeof(char)*(strlen(split[0])+1));
			strcpy(out, split[0]);
		}
	}
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	return out;
}

int qstrcmp(const void *a, const void *b) {
	const char *aa = *(const char**)a;
	const char *bb = *(const char**)b;
	return strcasecmp(aa, bb);
}

char *read_vmn_cache(char *str, char *match) {
		char *out = (char *)calloc(1, sizeof(char));
		char **split = line_split(str);
		int len = 0;
		for (int i = 0; i < strlen(str); ++i) {
			if (str[i] == '\t') {
				++len;
			}
		}
		++len;
		for (int i = 0; i < len; ++i) {
			if (strcasecmp(split[i], match) == 0) {
				out = (char *)realloc(out,sizeof(char)*(strlen(split[i+1])+1));
				strcpy(out, split[i+1]);
				break;
			}
		}
		for (int i = 0; i < len; ++i) {
			free(split[i]);
		}
		free(split);
		return out;
}

struct vmn_library lib_init() {
	struct vmn_library lib;
	lib.depth = 0;
	lib.files = NULL;
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

char **vmn_library_check(struct vmn_library *lib) {
	char *home = getenv("HOME"); 
	const char *cfg = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg) + 1);
	strcpy(path, home);
	strcat(path, cfg);
	FILE *cache = fopen(path, "r");
	char **new;
	if (!cache) {
		new = (char **)calloc(lib->length+1, sizeof(char *));
		for (int i = 0; i < lib->length; ++i) {
			new[i] = (char *)malloc(sizeof(char)*(strlen(lib->files[i])+1));
			strcpy(new[i], lib->files[i]);
		}
		return new;
	}
	char c;
	int file_len = 0;
	while ((c = fgetc(cache)) != EOF) {
		if (c == '\n') {
			++file_len;
		}
	}
	rewind(cache);
	char *cur = (char *)calloc(4096, sizeof(char));
	char **files = (char **)calloc(file_len, sizeof(char *));
	for (int i = 0; i < file_len; ++i) {
		fgets(cur, 4096, cache);
		int j = 0;
		while(cur[j] != '\t') { 
			++j;
		}
		++j;
		files[i] = (char *)malloc(sizeof(char)*j);
		memcpy(files[i], cur, sizeof(char)*(j-1));
		files[i][j-1] = '\0';
	}
	fclose(cache);
	free(path);
	free(cur);
	if (!files[0]) {
		new = (char **)calloc(lib->length+1, sizeof(char *));
		for (int i = 0; i < lib->length; ++i) {
			new[i] = (char *)malloc(sizeof(char)*(strlen(lib->files[i])+1));
			strcpy(new[i], lib->files[i]);
		}
		return new;
	}
	int new_pos = 0;
	int new_len = 120;
	new = (char **)malloc(sizeof(char *)*new_len);
	for (int i = 0; i < lib->length; ++i) {
		for (int j = 0; j < file_len; ++j) {
			if (strcmp(files[j], lib->files[i]) == 0) {
				break;
			}
			if (j == file_len-1 && (strcmp(files[j], lib->files[i]) != 0)) {
				if (new_pos == new_len) {
					new_len = new_len*2;
					new = (char **)realloc(new, sizeof(char *)*new_len);
				}
				new[new_pos] = (char *)malloc(sizeof(char)*(strlen(lib->files[i])+1));
				strcpy(new[new_pos], lib->files[i]);
				++new_pos;
			}
		}
	}
	for (int i = 0; i < file_len; ++i) {
		free(files[i]);
	}
	free(files);
	new[new_pos] = '\0';
	return new;
}

void vmn_library_destroy_meta(struct vmn_library *lib) {
	for (int i = 0; i < lib->length; ++i) {
		free(lib->files[i]);
	}
	free(lib->files);
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
				free(lib->entries[0][i][j]);
				++j;
			}
			free(lib->entries[0][i]);
			free(lib->items[i]);
		}
		free(lib->items);
		free(lib->entries[0]);
		free(lib->entries);
	}
}

void vmn_library_destroy_path(struct vmn_library *lib) {
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
	char *home = getenv("HOME"); 
	const char *cfg = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg) + 1);
	strcpy(path, home);
	strcat(path, cfg);
	char **new = vmn_library_check(lib);
	FILE *cache = fopen(path, "a");
	av_log_set_level(AV_LOG_QUIET);
	AVFormatContext *fmt_ctx = NULL;
	AVInputFormat *format = NULL;
	uint8_t *buffer = NULL;
	size_t buffer_size;
	int i = 0;
	while (new[i]) {
		struct stat st;
		stat(new[i], &st);
		buffer_size = st.st_size;
		format = get_input_format(new[i]);
		int ret = av_file_map(new[i], &buffer, &buffer_size, 0, NULL);
		if (ret) {
			continue;
		}
		av_file_unmap(buffer, buffer_size);
		fprintf(cache, "%s\t", new[i]);
		AVDictionaryEntry *tag = NULL;
		fmt_ctx = avformat_alloc_context();
		avformat_open_input(&fmt_ctx, new[i], format, NULL);
		if (strcmp(format->name, "ogg") == 0) {
			while ((tag = av_dict_get(fmt_ctx->streams[0]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
				int invalid = 0;
				for (int j = 0; j < strlen(tag->value); ++j) {
					if (tag->value[j] == '\n') {
						invalid = 1;
						break;
					}
				}
				if (strcmp(tag->value, "") == 0) {
					invalid = 1;
				}
				if (tag && !invalid) {
					fprintf(cache, "%s\t%s\t", tag->key, tag->value);
				}
			}
		} else {
			while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
				int invalid = 0;
				for (int j = 0; j < strlen(tag->value); ++j) {
					if (tag->value[j] == '\n') {
						invalid = 1;
						break;
					}
				}
				if (strcmp(tag->value, "") == 0) {
					invalid = 1;
				}
				if (tag && !invalid) {
					fprintf(cache, "%s\t%s\t", tag->key, tag->value);
				}
			}
		}
		fprintf(cache, "\n");
		avformat_close_input(&fmt_ctx);
		++i;
	}
	fclose(cache);
	free(path);
	avformat_free_context(fmt_ctx);
	i = 0;
	while (new[i]) {
		free(new[i]);
		++i;
	}
	free(new);
}

void vmn_library_refresh(struct vmn_library *lib, char *tag) {
	char *home = getenv("HOME"); 
	const char *cfg = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg) + 1);
	strcpy(path, home);
	strcat(path, cfg);
	FILE *cache = fopen(path, "r");
	char *cur = (char *)calloc(4096, sizeof(char));
	char **files = (char **)calloc(lib->length, sizeof(char *));
	int n = 0;
	int cache_len = 0;
	for (int i = 0; i < lib->length; ++i) {
		fgets(cur, 4096, cache);
		char **split = line_split(cur);
		int len = 0;
		for (int j = 0; j < strlen(cur); ++j) {
			if (cur[j] == '\t') {
				++len;
			}
		}
		++len;
		int skip = 0;
		for (int j = 0; j < len; ++j) {
			if ((strcasecmp(split[j], tag) == 0) && (strcmp(split[j+1], lib->selections[lib->depth]) == 0)) {
				skip = 1;
				++n;
				break;
			}
		}
		if (!skip) {
			files[cache_len] = (char *)malloc(sizeof(char)*(strlen(cur)+1));
			strcpy(files[cache_len], cur);
			++cache_len;
		}
		for (int i = 0; i < len; ++i) {
			free(split[i]);
		}
		free(split);
	}
	fclose(cache);
	const char *temp = "/.config/vmn/cache_tmp";
	char *temp_path = malloc(strlen(home) + strlen(temp) + 1);
	strcpy(temp_path, home);
	strcat(temp_path, temp);
	FILE *cache_temp = fopen(temp_path, "a");
	for (int i = 0; i < cache_len; ++i) {
		fprintf(cache_temp, "%s", files[i]);
	}
	fclose(cache_temp);
	remove(path);
	rename(temp_path, path);
	free(cur);
	free(path);
	free(temp_path);
	for (int i = 0; i < lib->length; ++i) {
		free(files[i]);
	}
	free(files);
	vmn_library_metadata(lib);
}

void vmn_library_sort(struct vmn_library *lib) {
	char *home = getenv("HOME"); 
	const char *cfg = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg) + 1);
	strcpy(path, home);
	strcat(path, cfg);
	FILE *cache = fopen(path, "r");
	char *cur = (char *)calloc(4096, sizeof(char));
	char **files = (char **)calloc(lib->length, sizeof(char *));
	for (int i = 0; i < lib->length; ++i) {
		fgets(cur, 4096, cache);
		files[i] = (char *)malloc(sizeof(char)*(strlen(cur)+1));
		strcpy(files[i], cur);
	}
	fclose(cache);
	qsort(files, lib->length, sizeof(char *), qstrcmp);
	const char *temp = "/.config/vmn/cache_tmp";
	char *temp_path = malloc(strlen(home) + strlen(temp) + 1);
	strcpy(temp_path, home);
	strcat(temp_path, temp);
	FILE *cache_temp = fopen(temp_path, "a");
	for (int i = 0; i < lib->length; ++i) {
		fprintf(cache_temp, "%s", files[i]);
	}
	fclose(cache_temp);
	remove(path);
	rename(temp_path, path);
	free(cur);
	free(path);
	free(temp_path);
	for (int i = 0; i < lib->length; ++i) {
		free(files[i]);
	}
	free(files);
}

void vmn_library_selections_add(struct vmn_library *lib, const char *entry) {
	lib->selections[lib->depth] = (char *)realloc(lib->selections[lib->depth], sizeof(char)*(strlen(entry) + 1));
	strcpy(lib->selections[lib->depth], entry);
}	
