int check_cfg(char *cfg_file);
void check_dir();
struct vmn_config cfg_init();
char *get_cfg();
char *get_cfg_dir();
char *get_cfg_lib();
int read_cfg_int(struct vmn_config *cfg, char *file, const char *opt);
const char *read_cfg_str(struct vmn_config *cfg, char *file, const char *opt);
void vmn_config_destroy(struct vmn_config *cfg);

enum vmn_config_view {
	F_PATH,
	S_ONLY,
};

struct vmn_config {
	int select;
	int select_pos;
	char *lib_dir;
	char *mpv_cfg_dir;
	char *mpv_cfg;
	enum vmn_config_view view;
};
