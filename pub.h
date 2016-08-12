#ifndef PUB_H_

#define PUB_H_

struct _msg {
	char header[8];
	char body[1024];
};

struct _conf {
	int max_err_msg_len;
	char err_log_path[64];
	int msg_header_max_len;
	int msg_body_max_size;
	int max_epoll_size;
	char master_pid_path[64];
	int command_max_len;
};

extern struct _conf *glb_conf;

void write_log(const char *);

void create_dir(const char *);

int socket_create(int port, int backlog=20);

void socket_setnonblock(int);

struct _conf * init_conf();

#endif
