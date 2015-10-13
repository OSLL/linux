### `elevator.c`
Elevator is an I/O scheduler that helps to make hard disk requests fast. Best performance is reached by operations with requests called merging  and sorting.

Each request is very expensive for the system so the main idea is to minimize their number. So, let's explain the main idea of an algorithm. 

We have an queue of requests and we have one fresh request. At first, we must check whether it is possible to merge it to one of old requests (it must follow or precede in physical memory). If it is possible - great! We can merge it and there is no need to do it separately. If not - put it to the end of an queue.

### Elements of realization
In fact, queue is a RB tree. Main functions:

`int elevator_init(struct request_queue *q, char *name)` initializes queue (returns code of error)

`void elevator_exit(struct elevator_queue *e)` ends all processes and clean memory

`void elv_add_request(struct request_queue *q, struct request *rq, int where)`

`void elv_merge_requests(struct request_queue *, struct request *, struct request *)`

`int elevator_change(struct request_queue *q, const char *name)` helps to switch to another scheduler
