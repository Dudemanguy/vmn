struct vmn_library lib_init();
void vmn_library_add(struct vmn_library *lib, char *entry);

struct vmn_library {
	char **files;
	int mem_size;
	int length;
};
