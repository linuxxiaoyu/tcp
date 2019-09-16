// server.c

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#define SERV_PORT 	43211
#define LISTENQ		1024
#define MAXLINE		512
#define MAX_PATH	512

char* trim(char*);
char* trim_left(char*);
char* trim_right(char*);
char* exec(char*, char *);

int main(int argc, char **argv) {
	int listenfd = tcp_server();
	char default_dir[MAX_PATH] = {0};
	exec("pwd", default_dir);
	trim(default_dir);

	while (1) {
		int connfd;
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);

		connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);
		if (connfd < 0) {
			error(1, errno, "accept failed");
		}
		
		chdir(default_dir);
		
		while (1) {
			char buf[MAXLINE];
			int n = read(connfd, buf, sizeof(buf)-1);
			if (n < 0) {
				error(1, errno, "read failed");
			} else if (n == 0) {
				printf("client closed\n");
				break;
			}

			buf[n] = '\0';
			trim(buf);

			int i = 0;
			for (i = 0; i < strlen(buf); i++) {
				buf[i] = tolower(buf[i]);
			}

			char send_line[4096] = {0};
			if (strcmp(buf, "quit") == 0) {
				close(connfd);
				printf("client quit\n");
				break;
			} else if (strcmp(buf, "pwd") == 0) {
				exec(buf, send_line);
			} else if (strncmp(buf, "ls", 2) == 0) {
				exec(buf, send_line);
			} else if (strncmp(buf, "cd ", 3) == 0 || strcmp(buf, "cd") == 0) {
				// exec(buf, NULL);
				chdir(trim(buf+2));
			}

			if (strlen(send_line) > 0) {
				n = write(connfd, send_line, strlen(send_line));
				if (n != strlen(send_line)) {
					error(1, errno, "write failed");
				}
			}
		}
	}
}

char* exec(char* cmd, char* output) {
	if (cmd == NULL)
		return NULL;

	FILE *fstream = NULL;
	char buff[1024] = {0};

	if ((fstream = popen(cmd, "r")) == NULL) {
		error(1, errno, "popen failed");
	}

	if (output == NULL) {
		return NULL;
	}

	while (fgets(buff, sizeof(buff), fstream) != NULL) {
		strcat(output, buff);
	}
	pclose(fstream);
	return output;
}

char* trim(char *buf) {
	return trim_left(trim_right(buf));
}

int is_space(char ch) {
	switch (ch) {
		case ' ':	// fallthrough
		case '\t':	// fallthrough
		case '\r':	// fallthrough
		case '\n':
			return 1;
		default:
			return 0;
	}
}

char* trim_right(char *buf) {
	if (buf == NULL) {
		return buf;
	}

	int len = strlen(buf);
	int i = 0;
	for (i = len-1; i>=0; i--) {
		if (is_space(buf[i])) {
			buf[i] = '\0';
		} else {
			break;
		} 
	}

	return buf;
}

char* trim_left(char *buf) {
	if (buf == 0)
		return buf;

	int len = strlen(buf);
	int i = 0;
	for (i = 0; i < len; i++) {
		if (is_space(buf[i])) {
			buf++;
		} else {
			break;
		}
	}
	return buf;
}

int tcp_server() {
	int listenfd;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		error(1, errno, "socket failed");
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	signal(SIGPIPE, SIG_IGN);

	int on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	int rt = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rt < 0) {
		error(1, errno, "bind failed");
	}

	rt = listen(listenfd, LISTENQ);
	if (rt < 0) {
		error(1, errno, "listen failed");
	}

	return listenfd;
}
