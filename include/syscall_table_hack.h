int init_system_call_table(void);
void cleanup_system_call_table(void);
void begin_syscall_table_hack(void);
void end_syscall_table_hack(void);
extern unsigned long **hacked_syscall_tbl;