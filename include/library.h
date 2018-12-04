struct vmn_library lib_init();
int ext_valid(char *ext);
char *get_file_ext(const char *file);
void vmn_library_add(struct vmn_library *lib, char *entry);
void vmn_library_destroy_meta(struct vmn_library *lib);
void vmn_library_destroy_path(struct vmn_library *lib);
void vmn_library_metadata(struct vmn_library *lib);
void vmn_library_selections_add(struct vmn_library *lib, const char *entry);

struct vmn_library {
	mpv_handle *ctx;
	AVDictionary **dict;
	int depth;
	char **files;
	char ****entries;
	ITEM ***items;
	int length;
	MENU **menu;
	int mpv_active;
	int mpv_kill;
	char **selections;
	int vmn_quit;
};
