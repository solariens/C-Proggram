#ifndef WORKER_H_

#define WORKER_H_

#define MAX_EPOLL_SIZE 1024

#define SHUT_DOWN 0

#define START_UP 1

#define COMMAND_MAX_LEN 16

class Worker{
	private:
		static int socket_fd;
		static int worker_process;
		static int *pids;
		static int process_status;
		static pid_t master_pid;
		int worker_count;
		char command[COMMAND_MAX_LEN];
		void stopProc();
		int socket_client[MAX_EPOLL_SIZE];
		int recvMsg(int);
		static int *epfds;
		void parseCommand();
		void run();
	public:
		Worker(int, int, const char *);
		void setDeamon();
		void setSignal();
		static void signalHandler(int);
		void savePid2File();
		void setWorkerProcess(int);
		void forkWorker();
		void monitorWorker();
		void (*msgSingleHandler)(struct _msg, int, int);
		void (*systemMsgHandler)(int);
		void (*msgAllHandler)(struct _msg, int, int *);
		void (*selfDefineHandler)(struct _msg, int);
		static void runAll(Worker &);
};

#endif
