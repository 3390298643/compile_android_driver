#include <linux/module.h>
#include <linux/tty.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "comm.h"
#include "memory.h"
#include "process.h"

long handle_ioctl(unsigned int fd, unsigned int const cmd, unsigned long const arg)
{
	static COPY_MEMORY cm;
	static MODULE_BASE mb;
	static char name[0x100] = {0};
	
	switch (cmd) {
		case OP_READ_MEM:
			{
				if (copy_from_user(&cm, (void __user*)arg, sizeof(cm)) != 0) {
					return -1;
				}
				if (read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false) {
					return -1;
				}
			}
			break;
		case OP_WRITE_MEM:
			{
				if (copy_from_user(&cm, (void __user*)arg, sizeof(cm)) != 0) {
					return -1;
				}
				if (write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false) {
					return -1;
				}
			}
			break;
		case OP_MODULE_BASE:
			{
				if (copy_from_user(&mb, (void __user*)arg, sizeof(mb)) != 0 
				|| copy_from_user(name, (void __user*)mb.name, sizeof(name)-1) !=0) {
					return -1;
				}
				mb.base = get_module_base(mb.pid, name);
				if (copy_to_user((void __user*)arg, &mb, sizeof(mb)) !=0) {
					return -1;
				}
			}
			break;
		default:
			break;
	}
	return 0;
}

static int handler_ioctl_pre(struct kprobe *p, struct pt_regs *kregs)
{
    unsigned int fd = (unsigned int)kregs->regs[0];
    unsigned int cmd = (unsigned int)kregs->regs[1];
    unsigned long arg = (unsigned long)kregs->regs[2];
    if (fd==-1 && cmd >= OP_INIT_KEY && cmd <= OP_MODULE_BASE)
    {
        handle_ioctl(fd, cmd, arg);
        return 1;
    }

    return 0;
}

static struct kprobe kp_ioctl = {
    .symbol_name = "inet_ioctl",
    .pre_handler = handler_ioctl_pre,
};

static int __init my_module_init(void) {

    int ret = register_kprobe(&kp_ioctl);
    printk("[Thook] kprobe ret:%d\n", ret);
    
    if (!IS_ERR(filp_open("/proc/sched_debug", O_RDONLY, 0))) {
		remove_proc_subtree("sched_debug", NULL); // /proc/sched_debug。
	}
	if (!IS_ERR(filp_open("/proc/uevents_records", O_RDONLY, 0))) {
		remove_proc_entry("uevents_records", NULL); // /proc/uevents_records。
	}
	
	list_del(&THIS_MODULE->list); //lsmod,/proc/modules
    kobject_del(&THIS_MODULE->mkobj.kobj); // /sys/modules
    list_del(&THIS_MODULE->mkobj.kobj.entry); // kobj struct list_head entry

	printk("[Thook] init\n");
    return 0;
}

static void __exit my_module_exit(void) {
    unregister_kprobe(&kp_ioctl);
    printk("[Thook] exit\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Custom syscall module using kprobes");
MODULE_AUTHOR("万载");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0))
	MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver); 
#endif
