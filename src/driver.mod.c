#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xeeab4c1e, "module_layout" },
	{ 0x34aea5de, "usb_deregister" },
	{ 0x411643fc, "usb_register_driver" },
	{ 0xb4b0eb14, "input_free_device" },
	{ 0xc5850110, "printk" },
	{ 0x2fbefc53, "wake_up_process" },
	{ 0xe554678a, "kthread_create_on_node" },
	{ 0x3e18778e, "input_register_device" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xf9c0b663, "strlcat" },
	{ 0x754d539c, "strlen" },
	{ 0x698667c4, "usb_alloc_urb" },
	{ 0x32a6fe24, "usb_alloc_coherent" },
	{ 0x13876db5, "input_allocate_device" },
	{ 0xd60e87c0, "kmem_cache_alloc_trace" },
	{ 0x6aa9e858, "kmalloc_caches" },
	{ 0x529e559b, "_dev_err" },
	{ 0x41656a26, "input_event" },
	{ 0x3a4c34f7, "usb_submit_urb" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x12a38747, "usleep_range" },
	{ 0xa7eedcc4, "call_usermodehelper" },
	{ 0x37a0cba, "kfree" },
	{ 0xe2c75aa7, "usb_free_coherent" },
	{ 0xa07e7bbf, "usb_free_urb" },
	{ 0xf007d19d, "input_unregister_device" },
	{ 0xf437babb, "kthread_stop" },
	{ 0xb9173a09, "usb_kill_urb" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic03isc01ip02in*");

MODULE_INFO(srcversion, "799A4513BBD9A4079EEBC75");
