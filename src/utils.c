#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void append_char(char *str, char c) {
	while(*str) {
		++str;
	}
	*str = c;
	++str;
	*str = '\0';
}

int char_count(char *str, char c) {
	int count = 0;
	for (int i = 0; i < strlen(str); ++i) {
		if (str[i] == c) {
			++count;
		}
	}
	return count;
}

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

char **cfg_split(char *str) {
	char *str_dup = strdup(str);
	char *token = strtok(str_dup, "=");
	char **arr = (char **)calloc(2, sizeof(char*));
	arr[0] = malloc(strlen(token) + 1);
	strcpy(arr[0], token);
	token = strtok(NULL, "=");
	arr[1] = malloc(strlen(token) + 1);
	strcpy(arr[1], token);
	arr[1][strcspn(arr[1], "\n")] = 0;
	free(str_dup);
	return arr;
}

struct char_split line_split(char *str, char *delim) {
	char *str_dup = strdup(str);
	int len = 0;
	for (int i = 0; i < strlen(str_dup); ++i) {
		if (str_dup[i] == '\t') {
			++len;
		}
	}
	++len;
	char *token = strtok(str_dup, delim);
	struct char_split split;
	split.arr = (char **)malloc(len*sizeof(char*));
	int i = 0;
	while (token != NULL) {
		split.arr[i] = (char *)malloc((strlen(token) + 1)*sizeof(char));
		strcpy(split.arr[i], token);
		token = strtok(NULL, delim);
		++i;
	}
	split.len = i;
	free(str_dup);
	return split;
}

void char_split_destroy(struct char_split *split) {
	for (int i = 0; i < split->len; ++i) {
		free(split->arr[i]);
	}
	free(split->arr);
}

int qstrcmp(const void *a, const void *b) {
	const char *aa = *(const char**)a;
	const char *bb = *(const char**)b;
	return strcasecmp(aa, bb);
}

void remove_char(char *str) {
	while(*str) {
		++str;
	}
	--str;
	*str = '\0';
}

char *remove_spaces(char *str) {
	char *trim = (char *)calloc(strlen(str) + 1, sizeof(char));
	int i = 0;
	while (*str != '\0') {
		if (*str != ' ') {
			trim[i] = *str;
			++i;
		}
		++str;
	}
	trim[i] = '\0';
	return trim;
}
