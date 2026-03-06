#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x0040afbe, "param_ops_ulong" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0xd272d446, "__SCT__preempt_schedule" },
	{ 0xa1dacb42, "class_destroy" },
	{ 0x0040afbe, "param_array_ops" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xde338d9a, "_raw_spin_lock" },
	{ 0xd272d446, "__fentry__" },
	{ 0x5a844b26, "__x86_indirect_thunk_rax" },
	{ 0xe8213e80, "_printk" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xbd03ed67, "page_offset_base" },
	{ 0xd272d446, "synchronize_rcu" },
	{ 0xe486c4b7, "device_create" },
	{ 0x653aa194, "class_create" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0x2435d559, "strncmp" },
	{ 0x2719b9fa, "const_current_task" },
	{ 0xc609ff70, "strncpy" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x6f8082dd, "pv_ops" },
	{ 0x7ec472ba, "__preempt_count" },
	{ 0x37031a65, "__register_chrdev" },
	{ 0x1595e410, "device_destroy" },
	{ 0xc064623f, "__kmalloc_cache_noprof" },
	{ 0x1c489eb6, "register_kprobe" },
	{ 0x7a8e92c6, "unregister_kprobe" },
	{ 0xe4de56b4, "__ubsan_handle_load_invalid_value" },
	{ 0x0040afbe, "param_ops_int" },
	{ 0xde338d9a, "_raw_spin_unlock" },
	{ 0xd272d446, "BUG_func" },
	{ 0xfaabfe5e, "kmalloc_caches" },
	{ 0x52b15b3b, "__unregister_chrdev" },
	{ 0xbebe66ff, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x0040afbe,
	0x092a35a2,
	0xd272d446,
	0xa1dacb42,
	0x0040afbe,
	0xcb8b6ec6,
	0xde338d9a,
	0xd272d446,
	0x5a844b26,
	0xe8213e80,
	0xbd03ed67,
	0xd272d446,
	0xbd03ed67,
	0xd272d446,
	0xe486c4b7,
	0x653aa194,
	0xbd03ed67,
	0x2435d559,
	0x2719b9fa,
	0xc609ff70,
	0xd272d446,
	0x092a35a2,
	0x6f8082dd,
	0x7ec472ba,
	0x37031a65,
	0x1595e410,
	0xc064623f,
	0x1c489eb6,
	0x7a8e92c6,
	0xe4de56b4,
	0x0040afbe,
	0xde338d9a,
	0xd272d446,
	0xfaabfe5e,
	0x52b15b3b,
	0xbebe66ff,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"param_ops_ulong\0"
	"_copy_from_user\0"
	"__SCT__preempt_schedule\0"
	"class_destroy\0"
	"param_array_ops\0"
	"kfree\0"
	"_raw_spin_lock\0"
	"__fentry__\0"
	"__x86_indirect_thunk_rax\0"
	"_printk\0"
	"__ref_stack_chk_guard\0"
	"__stack_chk_fail\0"
	"page_offset_base\0"
	"synchronize_rcu\0"
	"device_create\0"
	"class_create\0"
	"random_kmalloc_seed\0"
	"strncmp\0"
	"const_current_task\0"
	"strncpy\0"
	"__x86_return_thunk\0"
	"_copy_to_user\0"
	"pv_ops\0"
	"__preempt_count\0"
	"__register_chrdev\0"
	"device_destroy\0"
	"__kmalloc_cache_noprof\0"
	"register_kprobe\0"
	"unregister_kprobe\0"
	"__ubsan_handle_load_invalid_value\0"
	"param_ops_int\0"
	"_raw_spin_unlock\0"
	"BUG_func\0"
	"kmalloc_caches\0"
	"__unregister_chrdev\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "3C03BAE96C160553593E679");
