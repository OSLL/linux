/*
 * Blk Group scheduler
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

#include "meta-iosched.h"

struct blkgrp_data {
	struct list_head queue;
	struct request_queue *q;
};

static void blkgrp_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int blkgrp_dispatch(struct request_queue *q, int force)
{
	struct request *rq;
	struct blkgrp_data *nd = q->elevator->elevator_data;
	update_stats(q);

	if (!list_empty(&nd->queue)) {
		int time = calc_time_to_sleep(q);
		if (time != 0) {
			blk_delay_queue(q, time);
			return 0;
		}
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;
}

static void blkgrp_add_request(struct request_queue *q, struct request *rq)
{
	struct blkgrp_data *nd = q->elevator->elevator_data;
	list_add_tail(&rq->queuelist, &nd->queue);
}

static struct request *
blkgrp_former_request(struct request_queue *q, struct request *rq)
{
	struct blkgrp_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
blkgrp_latter_request(struct request_queue *q, struct request *rq)
{
	struct blkgrp_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

bool first_time = true;

static int blkgrp_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct blkgrp_data *nd;
	struct elevator_queue *eq;
	if (first_time) {
		first_time = false;
		kobj_init();
		init_my_timer();
		init_my_switch_timer();
	}

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	nd->q = q;
	add_queue(q);

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void blkgrp_exit_queue(struct elevator_queue *e)
{
	struct blkgrp_data *nd = e->elevator_data;

	del_queue(nd->q);

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_blkgrp = {
	.ops = {
		.elevator_merge_req_fn		= blkgrp_merged_requests,
		.elevator_dispatch_fn		= blkgrp_dispatch,
		.elevator_add_req_fn		= blkgrp_add_request,
		.elevator_former_req_fn		= blkgrp_former_request,
		.elevator_latter_req_fn		= blkgrp_latter_request,
		.elevator_init_fn		= blkgrp_init_queue,
		.elevator_exit_fn		= blkgrp_exit_queue,
	},
	.elevator_name = "blkgrp",
	.elevator_owner = THIS_MODULE,
};

static int __init blkgrp_init(void)
{
	return elv_register(&elevator_blkgrp);
}

static void __exit blkgrp_exit(void)
{
	elv_unregister(&elevator_blkgrp);
}

module_init(blkgrp_init);
module_exit(blkgrp_exit);


MODULE_AUTHOR("CSC");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Blk Group I/O scheduler");
