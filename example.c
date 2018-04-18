#include <arpa/inet.h>
#include "cst.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>


#define CONTAINER_OF(ptr, container, member) \
	((container *)((char *)(ptr) - offsetof(container, member)))

struct serv_data {
	struct cst_common common;
	int sockfd;
};

void error(int errnum, const char *msg)
{
	fprintf(stderr, "ERROR: %d: %s\n", errnum, msg);
	exit(errnum);
}

void *handle_req(struct cst_node *data)
{
	static const char page[] = 
		"HTTP/1.1 200 OK\r\n\n"
		"<!DOCTYPE html>"
		"<html>"
			"<head>"
				"<title>Hello</title>"
			"</head>"
			"<body>"
				"<h1>Hello</h1>"
				"<p>Hello</p>"
			"</body>"
		"</html>";

	int sockfd = CONTAINER_OF(data->iface.input, struct serv_data, common)
		->sockfd;
	char buf[256];
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int amount;
	int client_fd;
	while (1) {
		client_fd = accept(sockfd, (struct sockaddr *) &client_addr,
			&client_len);
		if (client_fd < 0) 
			error(errno, "accept() failed");
		printf("incoming:\n");
		do {
			amount = read(client_fd, buf, sizeof(buf) - 1);
			if (amount < 0)
				error(errno, "read() failed");
			buf[amount] = '\0';
			printf("%s", buf);
		} while (amount == sizeof(buf) - 1);
		amount = write(client_fd, page, sizeof(page));
		if (amount < 0)
			error(errno, "write() failed");
		close(client_fd);
	}
	return NULL;
}

#define N_THREADS 4

void init_threads(struct cst_node *threads, size_t n_threads)
{
	size_t i;
	for (i = 0; i < n_threads - 1; ++i) {
		threads[i].next = &threads[i + 1];
	}
	threads[i].next = NULL;
}

#define PORTNO 8080
#define BACKLOG 32

void init_serv_data(struct serv_data *sd)
{
	struct sockaddr_in serv_addr;
	int yes = 1;

	sd->common.handler = handle_req;
	sd->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd->sockfd < 0)
		error(errno, "socket() failed");
	if (setsockopt(sd->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		error(errno, "setsockopt() failed");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORTNO);
	if (bind(sd->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))
		< 0)
		error(errno, "bind() failed");
	if (listen(sd->sockfd, BACKLOG))
		error(errno, "listen() failed");
}

int main(void)
{
	struct cst_node threads[N_THREADS];
	struct serv_data sd;

	init_threads(threads, N_THREADS);
	init_serv_data(&sd);
	sd.common.handler = handle_req,
	cst_pool(&sd.common, NULL, threads);
	printf("done\n");
	getchar();
	return 0; 
}
