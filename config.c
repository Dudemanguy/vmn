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

struct vmn_config cfg_init() {
	struct vmn_config cfg = {0, 0};
	return cfg;
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

const char *cfg_defaults(const char *opt) {
	char *path;
	if (strcmp(opt, "library") == 0) {
		char *home = getenv("HOME");
		const char *library = "/Music";
		path = malloc(strlen(home) + strlen(library) + 1);
		strcpy(path, home);
		strcat(path, library);
	}
	return path;
}

const char *read_cfg(char *file, const char *opt) {
	config_t cfg;
	config_init(&cfg);
	config_read_file(&cfg, file);
	const char *output;
	if (config_lookup_string(&cfg, opt, &output)) {
		return output;
	} else {
		output = cfg_defaults(opt);
		return output;
	}
}
