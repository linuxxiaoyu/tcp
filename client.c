// client.c

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

#define MAXLINE 4096

int tcp_client(const char*, short);

int main(int argc, char **argv) {
	if (argc != 3) {
		error(1, 0, "usage: client <IPaddress> <Port>");
	}

	short port = atoi(argv[2]);
	int fd = tcp_client(argv[1], port);

	char send_line[MAXLINE], recv_line[MAXLINE];

	fd_set readmask, allreads;
	FD_ZERO(&allreads);
	FD_SET(0, &allreads);
	FD_SET(fd, &allreads);

	for ( ; ; ) {
		readmask = allreads;
		int rc = select(fd+1, &readmask, NULL, NULL, NULL);
		if (rc < 0) {
			error(1, errno, "select failed");
		}
		if (FD_ISSET(0, &readmask)) {
			if (fgets(send_line, sizeof(send_line), stdin) != NULL) {
				bzero(recv_line, sizeof(recv_line));

				int len = strlen(send_line);
				int n = write(fd, send_line, len);
				if (n != len) {
					error(1, errno, "write failed");
				}
			}
			continue;
		}
		if (FD_ISSET(fd, &readmask)) {
			int n = read(fd, recv_line, MAXLINE-1);
			if (n < 0) {
				error(1, errno, "read failed");
			} else if (n == 0) {
				error(1, 0, "server closed");
			}
			fprintf(stdout, "%s", recv_line);
		}
	}
}

int tcp_client(const char* addr, short port) {
	int fd;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		error(1, errno, "socket failed");
	}

	struct sockaddr_in server_addr;
	socklen_t server_len = sizeof(server_addr);
	bzero(&server_addr, server_len);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	inet_pton(AF_INET, addr, &server_addr.sin_addr);

	int rt = connect(fd, (struct sockaddr *)&server_addr, server_len);
	if (rt < 0) {
		error(1, errno, "connect failed");
	}

	return fd;
}
