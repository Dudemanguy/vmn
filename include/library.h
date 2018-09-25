struct vmn_library lib_init();
void vmn_library_add(struct vmn_library *lib, char *entry);
void vmn_library_destroy(struct vmn_library *lib);

struct vmn_library {
	int depth;
	char **files;
	char ****entries;
	ITEM ***items;
	int length;
	MENU **menu;
	int mpv_active;
};
