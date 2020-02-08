void append_char(char *str, char c);
int char_count(char *str, char c);
char *get_file_ext(const char *file);
int ext_valid(char *ext);
char **cfg_split(char *str);
char **line_split(char *str, char *delim);
int qstrcmp(const void *a, const void *b);
void remove_char(char *str);
char *remove_spaces(char *str);
