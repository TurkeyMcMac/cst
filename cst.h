#ifndef _CST_H

#define _CST_H

#include <pthread.h>

struct cst_node {
	struct cst_node *next;
	union cst_interface {
		struct cst_common *input;
		void *result;
	} iface;
	struct sockaddr *addr;
	pthread_t id;
};

typedef void *(*cst_req_handler_f)(struct cst_node *data);

struct cst_common {
	cst_req_handler_f handler;
	int _status;
};

#define CST_COMMON(handler) {handler}

int cst_pool(struct cst_common *common,
	const pthread_attr_t *attr,
	struct cst_node *threads);

#endif /* Header guard */
