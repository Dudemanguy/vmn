#include <stdlib.h>
#include <string.h>

char *append_char(char *str, char c) {
	int len = strlen(str);
	char *append = malloc(len + 2);
	strcpy(append, str);
	append[len] = c;
	append[len + 1] = '\0';
	return append;
}

char *remove_char(char *str) {
	int len = strlen(str);
	if (!len) {
		return "";
	}
	char *remove;
	remove = malloc(len - 1);
	memcpy(remove, str, sizeof(char)*(len - 1));
	remove[len - 1] = '\0';
	return remove;
}
