extern wait_queue_head_t thrott_wq;
void epoch_handler(struct timer_list *t);
void core_cleanup(void);
int core_setup(void);