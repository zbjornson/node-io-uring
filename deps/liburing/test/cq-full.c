/*
 * Description: test CQ ring overflow
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "../src/liburing.h"

static int queue_n_nops(struct io_uring *ring, int n)
{
	struct io_uring_sqe *sqe;
	int i, ret;

	for (i = 0; i < n; i++) {
		sqe = io_uring_get_sqe(ring);
		if (!sqe) {
			printf("get sqe failed\n");
			goto err;
		}

		io_uring_prep_nop(sqe);
	}

	ret = io_uring_submit(ring);
	if (ret < n) {
		printf("Submitted only %d\n", ret);
		goto err;
	} else if (ret < 0) {
		printf("sqe submit failed: %d\n", ret);
		goto err;
	}

	return 0;
err:
	return 1;
}

int main(int argc, char *argv[])
{
	struct io_uring_cqe *cqe;
	struct io_uring ring;
	int i, ret;

	ret = io_uring_queue_init(4, &ring, 0);
	if (ret) {
		printf("ring setup failed\n");
		return 1;

	}

	if (queue_n_nops(&ring, 4))
		goto err;
	if (queue_n_nops(&ring, 4))
		goto err;
	if (queue_n_nops(&ring, 4))
		goto err;

	i = 0;
	do {
		ret = io_uring_peek_cqe(&ring, &cqe);
		if (ret < 0) {
			printf("wait completion %d\n", ret);
			goto err;
		}
		io_uring_cqe_seen(&ring, cqe);
		if (!cqe)
			break;
		i++;
	} while (1);

	if (i != 8 || *ring.cq.koverflow != 4) {
		printf("CQ overflow fail: %d completions, %u overflow\n", i,
				*ring.cq.koverflow);
		goto err;
	}

	io_uring_queue_exit(&ring);
	return 0;
err:
	io_uring_queue_exit(&ring);
	return 1;
}
