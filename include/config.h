int check_cfg(char *cfg_file);
void check_dir();
struct vmn_config cfg_init(int argc, char *argv[]);
char *get_cfg();
char *get_cfg_dir();
char *get_cfg_lib();
void vmn_config_destroy(struct vmn_config *cfg);

struct vmn_key {
	int beginning;
	int end;
	int move_backward;
	int move_down;
	int move_forward;
	int move_up;
	int mpv_kill;
	int page_down;
	int page_up;
	int playback;
	int queue;
	int queue_all;
	int queue_clear;
	int visual;
	int vmn_quit;
};

enum vmn_config_view {
	F_PATH,
	S_ONLY,
};

struct vmn_config {
	int select;
	int select_pos;
	char *input_mode;
	char *lib_dir;
	char *mpv_cfg;
	char *mpv_cfg_dir;
	struct vmn_key key;
	enum vmn_config_view view;
};

