#include <dirent.h>
#include <libconfig.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"

int check_arg(struct vmn_config *cfg, char *arg) {
	char *valid[5] = {"", "--library=", "--mpv-cfg=", "--mpv-cfg-dir=", "--view="};
	int i = 0;
	int status;
	for (i = 0; i < 5; ++i) {
		regex_t regex;
		regcomp(&regex, valid[i], 0);
		status = regexec(&regex, arg, 0, NULL, 0);
		if (status == 0) {
			if (i == 0) {
				regfree(&regex);
			}
			if (i == 1) {
				regfree(&regex);
				return i;
				break;
			}
			if (i == 2) {
				regfree(&regex);
				return i;
				break;
			}
			if (i == 3) {
				regfree(&regex);
				return i;
				break;
			}
			if (i == 4) {
				regfree(&regex);
				return i;
				break;
			}
		}
	}
	return 0;
}

int check_cfg(char *cfg_file) {
	config_t cfg;
	config_init(&cfg);

	if (!config_read_file(&cfg, cfg_file)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}
	return 0;
}

void check_dir() {
	char *cfgdir = get_cfg_dir();
	struct stat st = {0};
	if (stat(cfgdir, &st) == -1) {
		mkdir(cfgdir, 0777);
	}
}

char *get_cfg() {
	char *home = getenv("HOME"); 
	const char *cfg = "/.config/vmn/config";
	char *path = malloc(strlen(home) + strlen(cfg) + 1);
	strcpy(path, home);
	strcat(path, cfg);
	return path;
}

char *get_cfg_dir() {
	char *home = getenv("HOME");
	const char *cfgdir = "/.config/vmn";
	char *path = malloc(strlen(home) + strlen(cfgdir) + 1);
	strcpy(path, home);
	strcat(path, cfgdir);
	return path;
}

char *get_default_lib() {
	char *home = getenv("HOME"); 
	const char *library = "/Music";
	char *path = malloc(strlen(home) + strlen(library) + 1);
	strcpy(path, home);
	strcat(path, library);
	return path;
}

char *read_arg(char *arg) {
	char *sep = "=";
	char *out = strtok(arg, sep);
	out = strtok(NULL, "");
	if (out) {
		return out;
	} else {
		return "";
	}
}

char *read_cfg_str(config_t *libcfg, const char *opt) {
	const char *output;
	if (!config_lookup_string(libcfg, opt, &output)) {
		return "";
	}
	char *out = strdup(output);
	return out;
}

struct vmn_config cfg_init(int argc, char *argv[]) {
	config_t libcfg;
	config_init(&libcfg);
	struct vmn_config cfg;
	char *cfg_file = get_cfg();
	config_read_file(&libcfg, cfg_file);
	key_init(&cfg, &libcfg);
	const char *library;
	const char *mpv_cfg;
	const char *mpv_cfg_dir;
	const char *viewcfg;
	int pos[4] = {0, 0, 0, 0};
	int lib_arg = 0;
	int mpv_arg = 0;
	int mpv_dir_arg = 0;
	int view_arg = 0;
	
	//check for any command line arguments
	//these take priority over any config file options
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			pos[i] = check_arg(&cfg, argv[i]);
		}
		for (int i = 0; i < argc; ++i) {
			if (pos[i] == 1) {
				lib_arg = 1;
				library = read_arg(argv[i]);
				DIR *dir = opendir(library);
				if (dir) {
					cfg.lib_dir = strdup(library);
				} else {
					cfg.lib_dir = get_default_lib();
					printf("Library directory not found. Falling back to default.\n");
				}
				closedir(dir);
			}
			if (pos[i] == 2) {
				mpv_arg = 1;
				mpv_cfg = read_arg(argv[i]);
				if ((strcmp(mpv_cfg, "no") == 0) || (strcmp(mpv_cfg, "no") == 0)) {
					cfg.mpv_cfg = strdup(mpv_cfg);
				} else {
					cfg.mpv_cfg = "yes";
				}
			}
			if (pos[i] == 3) {
				mpv_dir_arg = 1;
				mpv_cfg_dir = read_arg(argv[i]);
				DIR *dir = opendir(mpv_cfg_dir);
				if (dir) {
					cfg.mpv_cfg_dir = strdup(mpv_cfg_dir);
				} else {
					cfg.mpv_cfg_dir = get_cfg_dir();
					printf("Mpv config directory not found. Falling back to default.\n");
				}
				closedir(dir);
			}
			if (pos[i] == 4) {
				view_arg = 1;
				viewcfg = read_arg(argv[i]);
				if (strcmp(viewcfg, "file-path") == 0) {
					cfg.view = F_PATH;
				} else if (strcmp(viewcfg, "song-only") == 0) {
					cfg.view = S_ONLY;
				} else {
					cfg.view = F_PATH;
					printf("Invalid view specified. Falling back to default.\n");
				}
			}
		}
	}

	if (!lib_arg) {
		cfg.lib_dir = read_cfg_str(&libcfg, "library");
		if (strcmp(cfg.lib_dir, "") == 0) {
			cfg.lib_dir = get_default_lib();
		} else {
			DIR *dir = opendir(cfg.lib_dir);
			if (!dir) {
				cfg.lib_dir = get_default_lib();
				printf("Library directory not found. Falling back to default.\n");
			}
			closedir(dir);
		}
	}

	if (!mpv_arg) {
		cfg.mpv_cfg = read_cfg_str(&libcfg, "mpv-cfg");
		if ((!strcmp(cfg.mpv_cfg, "yes") == 0) && (!strcmp(cfg.mpv_cfg, "no") == 0) &&
				(!strcmp(cfg.mpv_cfg, "") == 0)) {
			printf("mpv-cfg can only be set to 'yes' or 'no'\n");
			cfg.mpv_cfg = "yes";
		}
	}

	if (!mpv_dir_arg) {
		cfg.mpv_cfg_dir = read_cfg_str(&libcfg, "mpv-cfg-dir");
		if (strcmp(cfg.mpv_cfg_dir, "") == 0) {
			cfg.mpv_cfg_dir = get_cfg_dir();
		} else {
			DIR *dir = opendir(cfg.mpv_cfg_dir);
			if (!dir) {
				cfg.mpv_cfg_dir = get_cfg_dir();
				printf("mpv config directory not found. Falling back to default.\n");
			}
			closedir(dir);
		}
	}

	if (!view_arg) {
		if (!config_lookup_string(&libcfg, "view", &viewcfg)) {
			cfg.view = F_PATH;
		} else {
			if (strcmp(viewcfg, "file-path") == 0) {
				cfg.view = F_PATH;
			} else if (strcmp(viewcfg, "song-only") == 0) {
				cfg.view = S_ONLY;
			} else {
				cfg.view = F_PATH;
				printf("Invalid view specified. Falling back to default.\n");
			}
		}
	}

	cfg.select = 0;
	cfg.select_pos = 0;

	free(cfg_file);
	config_destroy(&libcfg);
	return cfg;
}

void vmn_config_destroy(struct vmn_config *cfg) {
	free(cfg->lib_dir);
	free(cfg->mpv_cfg_dir);
}
