int check_cfg(char *cfg_file);
void check_dir();
struct vmn_config cfg_init();
char *get_cfg();
char *get_cfg_dir();
char *get_cfg_lib();
int read_cfg_int(char *file, const char *opt);
const char *read_cfg_string(struct vmn_config *vmn, char *file, const char *opt);

struct vmn_config {
	int select;
	int select_pos;
	int mpv_config;
	const char *library;
	const char *mpv_config_dir;
};
