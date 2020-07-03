struct vmn_config cfg_init(int argc, char *argv[]);
void vmn_config_destroy(struct vmn_config *cfg);
void mpv_cfg_add(struct vmn_config *cfg, char *opt, char *value);
void mpv_set_opts(mpv_handle *ctx, struct vmn_config *cfg);

struct vmn_key {
	int beginning;
	int command;
	int end;
	int escape;
	int move_backward;
	int move_down;
	int move_forward;
	int move_up;
	int mpv_kill;
	int mute;
	int page_down;
	int page_up;
	int playnext;
	int playpause;
	int playprev;
	int queue;
	int queue_all;
	int queue_clear;
	int search;
	int search_next;
	int search_prev;
	int start;
	int visual;
	int vmn_quit;
	int vmn_refresh;
	int voldown;
	int volup;
};

enum vmn_config_sort {
	S_DATA,
	S_FILE,
	S_NONE,
	S_NUMB,
	S_RAND,
};

enum vmn_config_view {
	V_PATH,
	V_DATA,
	V_SONG,
};

struct vmn_config {
	char *cfg_file;
	int err;
	char *input_mode;
	char *lib_dir;
	char **mpv_opts;
	int mpv_opts_len;
	struct vmn_key key;
	char **tags;
	int tags_len;
	enum vmn_config_sort *sort;
	enum vmn_config_view view;
};

