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
#include "utils.h"

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

int check_vmn_cache(struct vmn_library *lib, char *str, char **tags) {
	char **split = line_split(str, "\t");
	int len = 0;
	for (int i = 0; i < strlen(str); ++i) {
		if (str[i] == '\t') {
			++len;
		}
	}
	++len;
	int match;
	int known;
	for (int i = 0; i < lib->depth-1; ++i) {
		match = 0;
		known = is_known(tags[i], str);
		if (!known) {
			match = 1;
			continue;
		}
		for (int j = 0; j < len; ++j) {
			if ((strcmp(lib->selections[i], split[j]) == 0) && (strcasecmp(tags[i], split[j-1]) == 0)) {
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
	if (match) {
		return 1;
	} else {
		return 0;
	}
}

int check_vmn_lib(struct vmn_library *lib, char *line, char *lib_dir) {
	char **split = line_split(line, "\t");
	int len = 0;
	for (int i = 0; i < strlen(line); ++i) {
		if (line[i] == '\t') {
			++len;
		}
	}
	++len;
	int check = 0;
	int lib_dir_len = strlen(lib_dir);
	if (strncmp(lib_dir, split[0], lib_dir_len) == 0) {
		check = 1;
	}
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	return check;
}

struct vmn_entry create_entry(struct vmn_library *lib, char *line, char *lib_dir, char **tags) {
	struct vmn_entry entry;
	char **split = line_split(line, "\t");
	int len = 0;
	for (int i = 0; i < strlen(line); ++i) {
		if (line[i] == '\t') {
			++len;
		}
	}
	++len;
	int lib_dir_len = strlen(lib_dir);
	if (strncmp(lib_dir, split[0], lib_dir_len) == 0) {
		entry.in_lib = 1;
	} else {
		entry.in_lib = 0;
	}
	if (!entry.in_lib) {
		for (int i = 0; i < len; ++i) {
			free(split[i]);
		}
		free(split);
		return entry;
	}
	entry.known = (int *)malloc(lib->depth*sizeof(int));
	entry.selected = (int *)malloc(lib->depth*sizeof(int));
	for (int i = 0; i < lib->depth; ++i) {
		for (int j = 0; j < len; ++j) {
			if (strcasecmp(tags[i], split[j]) == 0) {
				entry.known[i] = 1;
				break;
			}
			entry.known[i] = 0;
		}
	}
	for (int i = 0; i < lib->depth; ++i) {
		for (int j = 0; j < len; ++j) {
			if (lib->selections[i]) {
				if (strcmp(lib->selections[i], split[j]) == 0) {
					entry.selected[i] = 1;
					break;
				}
			}
			entry.selected[i] = 0;
		}
	}
	entry.filename = strdup(strrchr(split[0], '/'));
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	return entry;
}

void entry_destroy(struct vmn_entry *entry) {
	free(entry->filename);
	free(entry->known);
	free(entry->selected);
}

char *get_vmn_cache_path(struct vmn_library *lib, char *line, char *name, char *tag) {
	char *out = (char *)calloc(1, sizeof(char));
	char **split = line_split(line, "\t");
	int len = 0;
	for (int i = 0; i < strlen(line); ++i) {
		if (line[i] == '\t') {
			++len;
		}
	}
	++len;
	for (int i = 0; i < len; ++i) {
		if ((strcmp(split[i], name) == 0) && (strcasecmp(tag, split[i-1]) == 0)) {
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

int is_known(char *tag, char *line) {
	char **split = line_split(line, "\t");
	int known = 0;
	int len = 0;
	for (int i = 0; i < strlen(line); ++i) {
		if (line[i] == '\t') {
			++len;
		}
	}
	++len;
	for (int i = 0; i < len; ++i) {
		if (strcasecmp(split[i], tag) == 0) {
			known = 1;
			break;
		}
	}
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	if (known) {
		return 1;
	} else {
		return 0;
	}
}

int is_sel(char *sel, char *line) {
	char **split = line_split(line, "\t");
	int valid = 0;
	int len = 0;
	for (int i = 0; i < strlen(line); ++i) {
		if (line[i] == '\t') {
			++len;
		}
	}
	++len;
	for (int i = 0; i < len; ++i) {
		if (strcmp(split[i], sel) == 0) {
			valid = 1;
			break;
		}
	}
	for (int i = 0; i < len; ++i) {
		free(split[i]);
	}
	free(split);
	if (valid) {
		return 1;
	} else {
		return 0;
	}
}

char *read_vmn_cache(char *str, char *match) {
	char *out = (char *)calloc(1, sizeof(char));
	char **split = line_split(str, "\t");
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

int read_vmn_cache_int(char *str, char *match) {
	int out;
	char **split = line_split(str, "\t");
	int len = 0;
	for (int i = 0; i < strlen(str); ++i) {
		if (str[i] == '\t') {
			++len;
		}
	}
	++len;
	for (int i = 0; i < len; ++i) {
		if (strcasecmp(split[i], match) == 0) {
			out = atoi(split[i+1]);
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
	lib.depth = 1;
	lib.files = NULL;
	lib.length = 0;
	lib.mpv_active = 0;
	lib.mpv_kill = 0;
	lib.select = 0;
	lib.select_pos = 0;
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

void vmn_library_destroy(struct vmn_library *lib) {
	for (int i = 0; i < lib->length; ++i) {
		free(lib->files[i]);
	}
	free(lib->files);
	if (lib->length) {
		for (int i = 0; i < lib->depth; ++i) {
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
		for (int i = 0; i < lib->depth; ++i) {
			unpost_menu(lib->menu[i]);
			free_menu(lib->menu[i]);
		}
		free(lib->menu);
	}
	if (lib->length) {
		for (int i = 0; i < lib->depth; ++i) {
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
		if (i == 1000) {
			printf("Storing a large amount of metadata into cache. This may take some time.\n");
		}
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
	int n = 0;
	int cache_len = 0;
	int file_len = 0;
	char c;
	while ((c = fgetc(cache)) != EOF) {
		if (c == '\n') {
			++file_len;
		}
	}
	char **files = (char **)calloc(file_len, sizeof(char *));
	rewind(cache);
	for (int i = 0; i < file_len; ++i) {
		fgets(cur, 4096, cache);
		char **split = line_split(cur, "\t");
		int len = 0;
		for (int j = 0; j < strlen(cur); ++j) {
			if (cur[j] == '\t') {
				++len;
			}
		}
		++len;
		int skip = 0;
		for (int j = 0; j < len; ++j) {
			if ((strcasecmp(split[j], tag) == 0) && (strcmp(split[j+1], lib->selections[lib->depth-1]) == 0)) {
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
	for (int i = 0; i < file_len; ++i) {
		free(files[i]);
	}
	free(files);
	vmn_library_metadata(lib);
}

void vmn_library_sort(struct vmn_library *lib, char *lib_dir) {
	char *home = getenv("HOME"); 
	const char *cfg = "/.config/vmn/cache";
	char *path = malloc(strlen(home) + strlen(cfg) + 1);
	strcpy(path, home);
	strcat(path, cfg);
	FILE *cache = fopen(path, "r");
	if (!cache) {
		free(path);
		return;
	}
	char *cur = (char *)calloc(4096, sizeof(char));
	int file_len = 0;
	int file_newlen = 0;
	char c;
	while ((c = fgetc(cache)) != EOF) {
		if (c == '\n') {
			++file_len;
		}
	}
	rewind(cache);
	char **files = (char **)calloc(file_len, sizeof(char *));
	for (int i = 0; i < file_len; ++i) {
		fgets(cur, 4096, cache);
		char **split = line_split(cur, "\t");
		int len = 0;
		for (int j = 0; j < strlen(cur); ++j) {
			if (cur[j] == '\t') {
				++len;
			}
		}
		++len;
		int match = 0;
		int in_lib = check_vmn_lib(lib, cur, lib_dir);
		if (in_lib) {
			for (int j = 0; j < lib->length; ++j) {
				if (strcmp(split[0], lib->files[j]) == 0) {
					match = 1;
					break;
				}
			}
			if (!match) {
				continue;
			}
		}
		files[file_newlen] = (char *)malloc(sizeof(char)*(strlen(cur)+1));
		strcpy(files[file_newlen], cur);
		++file_newlen;
		for (int j = 0; j < len; ++j) {
			free(split[j]);
		}
		free(split);
	}
	fclose(cache);
	qsort(files, file_newlen, sizeof(char *), qstrcmp);
	const char *temp = "/.config/vmn/cache_tmp";
	char *temp_path = malloc(strlen(home) + strlen(temp) + 1);
	strcpy(temp_path, home);
	strcat(temp_path, temp);
	FILE *cache_temp = fopen(temp_path, "a");
	for (int i = 0; i < file_newlen; ++i) {
		fprintf(cache_temp, "%s", files[i]);
	}
	fclose(cache_temp);
	remove(path);
	rename(temp_path, path);
	free(cur);
	free(path);
	free(temp_path);
	for (int i = 0; i < file_len; ++i) {
		free(files[i]);
	}
	free(files);
}

void vmn_library_selections_add(struct vmn_library *lib, const char *entry) {
	lib->selections[lib->depth-1] = (char *)realloc(lib->selections[lib->depth-1], sizeof(char)*(strlen(entry) + 1));
	strcpy(lib->selections[lib->depth-1], entry);
}	
