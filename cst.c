#include "cst.h"

enum {
	CST_BEGIN_WAIT,
	CST_BEGIN_ERROR,
	CST_BEGIN_SUCCESS
};

static void *cst_worker(void *_node)
{
	struct cst_node *node = _node;
	struct cst_common *common = node->iface.input;

	do {
		switch (__atomic_load_n(&common->_status, __ATOMIC_SEQ_CST)) {
		case CST_BEGIN_WAIT:
			continue;
		case CST_BEGIN_ERROR:
			return NULL;
		case CST_BEGIN_SUCCESS:
			break;
		}
	} while (0);

	return common->handler(node);
}

int cst_pool(struct cst_common *common,
	const pthread_attr_t *attr,
	struct cst_node *threads)
{
	int err;
	struct cst_node *t = threads;

	common->_status = CST_BEGIN_WAIT;
	while (t) {
		t->iface.input = common;
		err = pthread_create(&t->id, attr, cst_worker, t);
		if (err) {
			__atomic_store_n(&common->_status, CST_BEGIN_ERROR,
				__ATOMIC_SEQ_CST);
			while (threads != t) {
				pthread_join(threads->id, NULL);
				threads = threads->next;
			}
			return err;
		}
		t = t->next;
	}
	__atomic_store_n(&common->_status, CST_BEGIN_SUCCESS, __ATOMIC_SEQ_CST);
	return 0;
}
