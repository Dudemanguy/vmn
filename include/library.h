struct vmn_library lib_init();
int check_vmn_cache(struct vmn_library *lib, char *str, char **tags);
int check_vmn_lib(struct vmn_library *lib, char *line, char *lib_dir);
char *get_vmn_cache_path(struct vmn_library *lib, char *line, char *name, char *tag);
int is_known(char *tag, char *line);
int is_sel(char *sel, char *line);
char *read_vmn_cache(char *str, char *match);
int read_vmn_cache_int(char *str, char *match);
struct vmn_entry create_entry(struct vmn_library *lib, char *line, char *lib_dir, char **tags);
void entry_destroy(struct vmn_entry *entry);
void vmn_library_add(struct vmn_library *lib, char *entry);
void vmn_library_destroy(struct vmn_library *lib);
void vmn_library_metadata(struct vmn_library *lib);
void vmn_library_refresh(struct vmn_library *lib, char *tag);
void vmn_library_selections_add(struct vmn_library *lib, const char *entry);
void vmn_library_sort(struct vmn_library *lib, char *lib_dir);

struct vmn_entry {
	char *filename;
	int *known;
	int in_lib;
	char *path;
	int *selected;
};

struct vmn_library {
	mpv_handle *ctx;
	WINDOW *command;
	int depth;
	char **files;
	char ****entries;
	ITEM ***items;
	int length;
	MENU **menu;
	int mpv_active;
	int mpv_kill;
	int select;
	int select_pos;
	char **selections;
	WINDOW *search;
	int **unknown;
	WINDOW *visual;
	int vmn_quit;
};
