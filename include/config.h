int check_cfg(char *cfg_file);
void check_dir();
struct vmn_config cfg_init(int argc, char *argv[]);
char *get_cfg();
char *get_cfg_dir();
char *get_cfg_lib();
char **parse_tags(char *tags);
char *remove_spaces(char *str);
void vmn_config_destroy(struct vmn_config *cfg);

struct vmn_key {
	int beginning;
	int end;
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
	int quit;
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
	int select;
	int select_pos;
	char *input_mode;
	char *lib_dir;
	char *mpv_cfg;
	char *mpv_cfg_dir;
	struct vmn_key key;
	char **tags;
	int tags_len;
	enum vmn_config_sort *sort;
	enum vmn_config_view view;
	int sort_len;
};

