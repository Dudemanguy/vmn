#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void append_char(char *str, char c) {
	while(*str) {
		++str;
	}
	*str = c;
	++str;
	*str = '\0';
	//*str++ = c;
	//*str++ = '\0';
	/*char *append = malloc(sizeof(char)*(len+3));
	if (len > 0) {
		append = strdup(str);
	}
	append[len] = c;
	append[len + 1] = '\0';
	if (len) {
		free(str);
	}
	return append;*/
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

char **line_split(char *str, char *delim) {
	char *str_dup = strdup(str);
	char *token = strtok(str_dup, delim);
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
		token = strtok(NULL, delim);
		++i;
	}
	arr = (char **)realloc(arr, sizeof(char*)*i);
	free(str_dup);
	return arr;
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
	trim = (char *)realloc(trim, sizeof(char)*(i+1));
	return trim;
}
