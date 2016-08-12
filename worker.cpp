#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/epoll.h>
#include <wait.h>
#include "worker.h"
#include "../pub/pub.h"

int Worker::worker_process = 0;
int *Worker::pids = NULL;
int Worker::process_status = START_UP;
pid_t Worker::master_pid = 0;
int Worker::socket_fd = 0;
int *Worker::epfds = NULL;

Worker::Worker(int port, int backlog, const char *cmd) {
	if (port <= 0 || backlog <=0 || cmd == NULL) {
		write_log("the parameters of constructor is error!");
		exit(0);
	}
	worker_count = 0;
	memset(command, 0, sizeof(command));
	strcpy(command, cmd);
	parseCommand();
	socket_fd = socket_create(port, backlog);
	if (socket_fd == -1) {
		exit(0);
	}
	socket_setnonblock(socket_fd);
}

void Worker::parseCommand() {
	int ret = access(glb_conf->master_pid_path, F_OK);
	if (!strcmp(command, "start")) {
		if (!ret){
			write_log("the process is running !");
			exit(0);
		}
	} else if (!strcmp(command, "stop")) {
		if (ret == -1) {
			write_log("the process is not running !");
			exit(0);
		}
		stopProc();
		exit(0);
	} else if(!strcmp(command, "restart")) {
		if (!ret) {
			stopProc();
		}
		write_log("the process is restarting....!");
	}
}

void Worker::stopProc() {
	FILE *fp = fopen(glb_conf->master_pid_path, "r");
	if (fp == NULL) {
		write_log(strerror(errno));
		exit(0);
	}
	char buf[12];
	memset(buf, 0, sizeof(buf));
	if (fread(buf, 1, sizeof(buf), fp) <= 0) {
		write_log(strerror(errno));
		exit(0);
	}
	fclose(fp);
	pid_t tmp_pid = atoi(buf);
	write_log("the process is stoping.....!");
	if (kill(tmp_pid, SIGINT) == -1) {
		write_log(strerror(errno));
	}
}

void Worker::setDeamon() {

	pid_t pid = fork();

	if (pid < 0) {
		write_log(strerror(errno));
		return;
	} else if (pid > 0) {
		exit(0);
	} else {
		if (setsid() == -1) {
			write_log(strerror(errno));
			exit(0);
		}
		chdir("/");
		umask(0);
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
}

void Worker::savePid2File() {
	create_dir(glb_conf->master_pid_path);
	FILE *fp = fopen(glb_conf->master_pid_path, "w");
	if (fp == NULL) {
		write_log(strerror(errno));
		return;
	}
	master_pid = getpid();
	char buf[10];
	sprintf(buf, "%d", master_pid);
	fwrite(buf, 1, strlen(buf), fp);
	fclose(fp);
}

void Worker::run() {
	int ret;
	struct epoll_event event, events[MAX_EPOLL_SIZE];
	int epfd = epoll_create(MAX_EPOLL_SIZE);
	if (epfd == -1) {
		write_log(strerror(errno));
		return;
	}
	for(int i=0; i<worker_process; ++i) {
		if (epfds[i] == -1) {
			epfds[i] = epfd;
			break;
		}
	}
	memset(&event, 0, sizeof(event));
	event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	event.data.fd = socket_fd;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);
	if (ret == -1) {
		write_log(strerror(errno));
		exit(0);
	}

	int active_num = 0, i = 0;

	for(int i=0; i<MAX_EPOLL_SIZE; ++i) {
		socket_client[i] = 0;
	}

	while(1) {
		active_num = epoll_wait(epfd, events, MAX_EPOLL_SIZE, -1);
		for(i=0; i<active_num; ++i) {
			if (events[i].data.fd == socket_fd) {
				struct sockaddr_in client_addr;
				memset(&client_addr, 0, sizeof(client_addr));
				socklen_t addr_len = sizeof(client_addr);
				int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_len);
				if (client_fd == -1) {
					write_log(strerror(errno));
					exit(0);
				}
				socket_setnonblock(client_fd);
				event.data.fd = client_fd;
				event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
				if (ret == -1) {
					write_log(strerror(errno));
					/*if epoll ctl failed, then send the SIGINT signal to master process and close all resources*/
					kill(master_pid, SIGINT);
					exit(0);
				}
				continue;
			} else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
				events[i].data.fd = -1;
				continue;
			} else {
				int tmp_fd = events[i].data.fd;
				ret = recvMsg(tmp_fd);
				/*if recvMsg error, then return the uid and use this uid delete the reflect fd*/
				if(ret <= 0) {
					for (int i=0; i<MAX_EPOLL_SIZE; ++i) {
						if (socket_client[i] == tmp_fd) {
							close(tmp_fd);
							events[i].data.fd = -1;
							socket_client[i] = 0;
						}
					}
				}
			}
		}
	}
}

int Worker::recvMsg(int fd) {
	struct _msg msg;
	memset(&msg, 0, sizeof(msg));
	ssize_t size = recv(fd, (char *)&msg, sizeof(msg), 0);
	if(size <= 0) {
		return -1;
	}
	switch(msg.header[0]) {
		case 0:  //user just connected
			socket_client[msg.header[1]] = fd;
			break;
		case 1:  //user have connected
			msgSingleHandler(msg, size, socket_client[msg.header[2]]);
			break;
		case 2:  //system msg to user
			systemMsgHandler(socket_client[msg.header[1]]);
			break;
		case 3:  //msg can send to all user
			msgAllHandler(msg, size, socket_client);
			break;
		case 4:  //user function
			selfDefineHandler(msg, size);
			break;
	}
	return size;
}

void Worker::setWorkerProcess(int num) {
	if (num <= 0) {
		write_log("work process must great than one!");
		exit(0);
	}
	worker_process = num;
	pids = new int[worker_process];
	for(int i=0; i<worker_process; ++i) {
		pids[i] = -1;
	}
	epfds = new int[worker_process];
	for(int i=0; i<worker_process; ++i) {
		epfds[i] = -1;
	}
}

void Worker::setSignal() {
	signal(SIGINT, Worker::signalHandler);
	signal(SIGUSR1, Worker::signalHandler);
	signal(SIGUSR2, Worker::signalHandler);
}

void Worker::forkWorker() {
	int i = worker_count;
	for(; i<worker_process; ++i) {
		pid_t pid = fork();
		if (pid < 0) {
			write_log(strerror(errno));
			exit(0);
		} else if (pid > 0) {
			for (int j=0; j<worker_process; ++j) {
				if (pids[j] == -1) {
					pids[j] = pid;
					break;
				}
			}
			worker_count++;
		} else {
			run();	
			exit(-1);
		}
	}
}

void Worker::monitorWorker() {
	int status, s;
	while (1) {
		pid_t pid = wait(&status);
		s = WEXITSTATUS(status);
		if (s == 0) {
			for(int i=0; i<worker_count; ++i) {
				if (pids[i] == pid) {
					pids[i] = -1;
					epfds[i] = -1;
					break;
				}
			}
		}
		worker_count--;
		if (process_status == START_UP) {
			forkWorker();
		}
	}
}

void Worker::signalHandler(int signo) {
	switch(signo) {
		case SIGINT:
		{
			if (master_pid == getpid()) {
				for(int i=0; i<worker_process; ++i) {
					if (pids[i] != -1) {
						close(epfds[i]);
						kill(pids[i], SIGKILL);
					}
				}
				process_status = SHUT_DOWN;
				if (unlink(glb_conf->master_pid_path) == -1) {
					write_log(strerror(errno));
				}
				delete [] pids;
				if (socket_fd > 0) {
					close(socket_fd);
				}
				delete glb_conf;
				kill(master_pid, SIGKILL);
			}
		}
		break;
		case SIGUSR1:
		break;
		case SIGUSR2:
		break;
	}
}

void Worker::runAll(Worker &worker) {
	worker.setDeamon();
	worker.savePid2File();
	worker.setSignal();
	worker.forkWorker();
	worker.monitorWorker();
}

