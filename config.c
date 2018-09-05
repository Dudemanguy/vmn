#include <libconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"

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

char *get_cfg_lib() {
	char *home = getenv("HOME"); 
	const char *lib = "/.config/vmn/lib";
	char *path = malloc(strlen(home) + strlen(lib) + 1);
	strcpy(path, home);
	strcat(path, lib);
	return path;
}

int get_default_int(struct vmn_config *cfg, const char *opt) {
	int output;
	if (strcmp(opt, "select") == 0) {
		output = cfg->select;
	}
	if (strcmp(opt, "select_pos") == 0) {
		output = cfg->select_pos;
	}
	return output;
}

char *get_default_lib() {
	char *home = getenv("HOME"); 
	const char *library = "/Music";
	char *path = malloc(strlen(home) + strlen(library) + 1);
	strcpy(path, home);
	strcat(path, library);
	return path;
}

char *get_default_str(struct vmn_config *cfg, const char *opt) {
	char *output;
	if (strcmp(opt, "library") == 0) {
		char *home = getenv("HOME");
		const char *library = "/Music";
		output = malloc(strlen(home) + strlen(library) + 1);
		strcpy(output, home);
		strcat(output, library);
	}
	if (strcmp(opt, "mpv_cfg_dir") == 0) {
		char *output;
		char *home = getenv("HOME");
		const char *cfgdir = "/.config/vmn";
		output = malloc(strlen(home) + strlen(cfgdir) + 1);
		strcpy(output, home);
		strcat(output, cfgdir);
	}
	if (strcmp(opt, "mpv_cfg") == 0) {
		char *output;
		const char *load_config = "yes";
		output = malloc(strlen(load_config) + 1);
		strcpy(output, load_config);
	}
	return output;
}

int read_cfg_int(struct vmn_config *vmn, char *file, const char *opt) {
	config_t cfg;
	config_init(&cfg);
	config_read_file(&cfg, file);
	int output;
	config_lookup_int(&cfg, opt, &output);
	if (!output) {
		output = get_default_int(vmn, opt);
	}
	return output;
}

const char *read_cfg_str(struct vmn_config *vmn, char *file, const char *opt) {
	config_t cfg;
	config_init(&cfg);
	config_read_file(&cfg, file);
	const char *output;
	config_lookup_string(&cfg, opt, &output);
	return output;
}

struct vmn_config cfg_init() {
	struct vmn_config vmn;
	config_t cfg;
	config_init(&cfg);
	char *cfg_file = get_cfg();
	config_read_file(&cfg, cfg_file);

	const char *library;
	config_lookup_string(&cfg, "library", &library);
	if (!library) {
		library = get_default_lib();
	}
	vmn.select = 0;
	vmn.select_pos = 0;
	vmn.library = library;
	vmn.mpv_cfg_dir = get_cfg_dir();
	vmn.mpv_cfg = "yes";
	return vmn;
}
