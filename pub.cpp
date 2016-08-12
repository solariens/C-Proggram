#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <libgen.h>
#include "pub.h"

struct _conf *glb_conf = init_conf();

void write_log(const char *errmsg) {

	if (errmsg == NULL) {
		return;
	}

	time_t t;
	if (time(&t) == -1) {
		printf("%s\n", strerror(errno));
		return;
	}

	struct tm *date = localtime(&t);
	if (date == NULL) {
		printf("%s\n", strerror(errno));
		return;
	}

	int year = date->tm_year;
	int mon = date->tm_mon;
	int day = date->tm_mday;
	int hour = date->tm_hour;
	int min = date->tm_min;
	int sec = date->tm_sec;

	char buf[glb_conf->max_err_msg_len];
	sprintf(buf, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n", year, mon, day, hour, min, sec, errmsg);
	create_dir(glb_conf->err_log_path);
	FILE *fp = fopen(glb_conf->err_log_path, "a");
	if (fp == NULL) {
		printf("%s\n", strerror(errno));
		return;
	}
	fwrite(buf, 1, strlen(buf), fp);
	fclose(fp);
	return;
}

void create_dir(const char *dirpath) {
	char *dirc = strdup(dirpath);
	char *dname = dirname(dirc);
	if (access(dname, F_OK) == -1) {
		mkdir(dname, 0666);
	}
}

int socket_create(int port, int backlog) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (server_fd == -1) {
	    write_log(strerror(errno));
	    return -1; 
	}   
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
	    write_log(strerror(errno));
	    return -1; 
	}   
    if (listen(server_fd, backlog) == -1) {
	    write_log(strerror(errno));
	    return -1; 
	}   
    return server_fd;
}

void socket_setnonblock(int fd) {
	int flag = fcntl(fd, F_GETFL);
	if (flag == -1) {
		write_log(strerror(errno));
		return;
	}   
	flag |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flag) == -1) {
		write_log(strerror(errno));
		return;
	}   
}

struct _conf *init_conf() {
	char cur_path[64];
	memset(cur_path, 0, sizeof(cur_path));
	if (getcwd(cur_path, sizeof(cur_path)) == NULL) {
		printf("get the current path failed\n");
		exit(0);
	}
	strcat(cur_path, "/pub/pub.conf");
	struct _conf *conf = new _conf;
	FILE *fp = fopen(cur_path, "r");
	char buf[64], key[32];
	char *tmp_str;
	int len, tmp_len;
	memset(buf, 0, sizeof(buf));
	memset(key, 0, sizeof(key));
	while (fgets(buf, sizeof(buf), fp)) {
		len = strlen(buf);
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
		}   
		tmp_str = strchr(buf, ':');
		if (tmp_str == NULL) {
			continue;
		}   
		tmp_len = strlen(tmp_str);
		memcpy(key, buf, len - tmp_len - 1); 
		if (!strcmp(key, "MAX_ERR_MSG_LEN")) {
			conf->max_err_msg_len = atoi(++tmp_str);
		} else if (!strcmp(key, "ERR_LOG_PATH")) {
			strcpy(conf->err_log_path, ++tmp_str);
		} else if (!strcmp(key, "MSG_HEADER_MAX_LEN")) {
			conf->msg_header_max_len = atoi(++tmp_str);
		} else if (!strcmp(key, "MSG_BODY_MAX_SIZE")) {
			conf->msg_body_max_size = atoi(++tmp_str);
		} else if (!strcmp(key, "MAX_EPOLL_SIZE")) {
			conf->max_epoll_size = atoi(++tmp_str);
		} else if (!strcmp(key, "MASTER_PID_PATH")) {
			strcpy(conf->master_pid_path, ++tmp_str);
		} else if (!strcmp(key, "COMMAND_MAX_LEN")) {
			conf->command_max_len = atoi(++tmp_str);
		}
		memset(buf, 0, sizeof(buf));
		memset(key, 0, sizeof(key));
	} 
	fclose(fp);
	return conf;
}

