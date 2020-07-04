struct vmn_config cfg_init(int argc, char *argv[]);
void vmn_config_destroy(struct vmn_config *cfg);
void mpv_cfg_add(struct vmn_config *cfg, char *opt, char *value);
void mpv_set_opts(mpv_handle *ctx, struct vmn_config *cfg);

enum vmn_key {
	BEGINNING,
	COMMAND,
	END,
	ESCAPE,
	MOVE_BACKWARD,
	MOVE_DOWN,
	MOVE_FORWARD,
	MOVE_UP,
	MPV_KILL,
	MUTE,
	PAGE_DOWN,
	PAGE_UP,
	PLAYNEXT,
	PLAYPAUSE,
	PLAYPREV,
	QUEUE,
	QUEUE_ALL,
	QUEUE_CLEAR,
	SEARCH,
	SEARCH_NEXT,
	SEARCH_PREV,
	START,
	VISUAL,
	VMN_QUIT,
	VMN_REFRESH,
	VOLDOWN,
	VOLUP,
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
	int *keys;
	int key_count;
	char *lib_dir;
	char **mpv_opts;
	int mpv_opts_len;
	enum vmn_config_sort *sort;
	char **tags;
	int tags_len;
	enum vmn_config_view view;
};

