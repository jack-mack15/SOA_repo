//api
int hack_syscall(int sys_num);
int dishack_syscall(int sys_num);
long throttling_wrapper(const struct pt_regs *);
int cleanup_rcu(void);
int register_system_call(const int syscall_numb);
int deregister_system_call(const int syscall_numb);
int register_user_id(const uid_t user_id);
int deregister_user_id(const uid_t user_id);
int register_prog_name(const char *prog_name);
int deregister_prog_name(const char *prog_name);
int switch_on_monitor(void);
int switch_off_monitor(void);
int set_max_syscall(int new_max);
struct thread_stats_cr_struct *get_thread_stats(void);
struct syscall_cr_struct *get_syscall_stats(int sys_num);
int check_syscall(const int syscall_numb);
int check_uid(const uid_t user_id);
int check_progname(const char *prog_name);