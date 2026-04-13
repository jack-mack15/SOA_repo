#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x0040afbe, "param_ops_ulong" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0xd272d446, "__rcu_read_lock" },
	{ 0xd710adbf, "__kmalloc_noprof" },
	{ 0xd272d446, "__SCT__preempt_schedule" },
	{ 0xc87f4bab, "finish_wait" },
	{ 0xa1dacb42, "class_destroy" },
	{ 0x0040afbe, "param_array_ops" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0x0db8d68d, "prepare_to_wait_event" },
	{ 0x2352b148, "timer_delete_sync" },
	{ 0x16ab4215, "__wake_up" },
	{ 0xe1e1f979, "_raw_spin_lock_irqsave" },
	{ 0xd272d446, "__fentry__" },
	{ 0x5a844b26, "__x86_indirect_thunk_rax" },
	{ 0xe8213e80, "_printk" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0xd272d446, "schedule" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x9479a1e8, "strnlen" },
	{ 0xd272d446, "__put_user_4" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xbd03ed67, "page_offset_base" },
	{ 0xd70733be, "sized_strscpy" },
	{ 0x7a5ffe84, "init_wait_entry" },
	{ 0xd272d446, "synchronize_rcu" },
	{ 0xd272d446, "__rcu_read_unlock" },
	{ 0x5a844b26, "__x86_indirect_thunk_r14" },
	{ 0x32feeafc, "mod_timer" },
	{ 0xe486c4b7, "device_create" },
	{ 0x653aa194, "class_create" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0x2435d559, "strncmp" },
	{ 0x2719b9fa, "const_current_task" },
	{ 0xc609ff70, "strncpy" },
	{ 0xe54e0a6b, "__fortify_panic" },
	{ 0xe199f25f, "jiffies_to_msecs" },
	{ 0x81a1a811, "_raw_spin_unlock_irqrestore" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x058c185a, "jiffies" },
	{ 0x6f8082dd, "pv_ops" },
	{ 0xee139a2f, "strncpy_from_user" },
	{ 0x7ec472ba, "__preempt_count" },
	{ 0x37031a65, "__register_chrdev" },
	{ 0x7851be11, "__get_user_4" },
	{ 0x1595e410, "device_destroy" },
	{ 0xc064623f, "__kmalloc_cache_noprof" },
	{ 0x546c19d9, "validate_usercopy_range" },
	{ 0x1c489eb6, "register_kprobe" },
	{ 0x02f9bbf0, "timer_init_key" },
	{ 0x7a8e92c6, "unregister_kprobe" },
	{ 0x0040afbe, "param_ops_int" },
	{ 0xd272d446, "BUG_func" },
	{ 0x7851be11, "__SCT__might_resched" },
	{ 0xfaabfe5e, "kmalloc_caches" },
	{ 0x52b15b3b, "__unregister_chrdev" },
	{ 0xbebe66ff, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xa61fd7aa,
	0x0040afbe,
	0x092a35a2,
	0xd272d446,
	0xd710adbf,
	0xd272d446,
	0xc87f4bab,
	0xa1dacb42,
	0x0040afbe,
	0xcb8b6ec6,
	0x0db8d68d,
	0x2352b148,
	0x16ab4215,
	0xe1e1f979,
	0xd272d446,
	0x5a844b26,
	0xe8213e80,
	0xbd03ed67,
	0xd272d446,
	0xd272d446,
	0x9479a1e8,
	0xd272d446,
	0x90a48d82,
	0xbd03ed67,
	0xd70733be,
	0x7a5ffe84,
	0xd272d446,
	0xd272d446,
	0x5a844b26,
	0x32feeafc,
	0xe486c4b7,
	0x653aa194,
	0xbd03ed67,
	0x2435d559,
	0x2719b9fa,
	0xc609ff70,
	0xe54e0a6b,
	0xe199f25f,
	0x81a1a811,
	0xd272d446,
	0x092a35a2,
	0x058c185a,
	0x6f8082dd,
	0xee139a2f,
	0x7ec472ba,
	0x37031a65,
	0x7851be11,
	0x1595e410,
	0xc064623f,
	0x546c19d9,
	0x1c489eb6,
	0x02f9bbf0,
	0x7a8e92c6,
	0x0040afbe,
	0xd272d446,
	0x7851be11,
	0xfaabfe5e,
	0x52b15b3b,
	0xbebe66ff,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"__check_object_size\0"
	"param_ops_ulong\0"
	"_copy_from_user\0"
	"__rcu_read_lock\0"
	"__kmalloc_noprof\0"
	"__SCT__preempt_schedule\0"
	"finish_wait\0"
	"class_destroy\0"
	"param_array_ops\0"
	"kfree\0"
	"prepare_to_wait_event\0"
	"timer_delete_sync\0"
	"__wake_up\0"
	"_raw_spin_lock_irqsave\0"
	"__fentry__\0"
	"__x86_indirect_thunk_rax\0"
	"_printk\0"
	"__ref_stack_chk_guard\0"
	"schedule\0"
	"__stack_chk_fail\0"
	"strnlen\0"
	"__put_user_4\0"
	"__ubsan_handle_out_of_bounds\0"
	"page_offset_base\0"
	"sized_strscpy\0"
	"init_wait_entry\0"
	"synchronize_rcu\0"
	"__rcu_read_unlock\0"
	"__x86_indirect_thunk_r14\0"
	"mod_timer\0"
	"device_create\0"
	"class_create\0"
	"random_kmalloc_seed\0"
	"strncmp\0"
	"const_current_task\0"
	"strncpy\0"
	"__fortify_panic\0"
	"jiffies_to_msecs\0"
	"_raw_spin_unlock_irqrestore\0"
	"__x86_return_thunk\0"
	"_copy_to_user\0"
	"jiffies\0"
	"pv_ops\0"
	"strncpy_from_user\0"
	"__preempt_count\0"
	"__register_chrdev\0"
	"__get_user_4\0"
	"device_destroy\0"
	"__kmalloc_cache_noprof\0"
	"validate_usercopy_range\0"
	"register_kprobe\0"
	"timer_init_key\0"
	"unregister_kprobe\0"
	"param_ops_int\0"
	"BUG_func\0"
	"__SCT__might_resched\0"
	"kmalloc_caches\0"
	"__unregister_chrdev\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "31B7A1EDB8544302CE55B92");
