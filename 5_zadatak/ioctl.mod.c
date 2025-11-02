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
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x92997ed8, "_printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x3ef97a02, "cdev_init" },
	{ 0xc25d5652, "cdev_add" },
	{ 0xf795a77a, "class_create" },
	{ 0x1c8ac990, "device_create" },
	{ 0x7057799f, "class_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x6ff257d6, "device_destroy" },
	{ 0xa4cf93df, "cdev_del" },
	{ 0x5f754e5a, "memset" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "CE823BF9986A67C53A3447E");
