#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>

#define procfs_name "r0memdump"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("null333");
MODULE_DESCRIPTION("ring 0 memory dumper, get mogged jimmy");
MODULE_VERSION("1.0");

struct pdumpcache
{
    // vmalloc this for 2x sizes for process (realloc if it grows, pretty much a ghetto vector)
    char* data, stack, heap, mmap;
    int64_t data_sz, stack_sz, heap_dz, mmap_sz;
};

static void intrpt_routine(void *);
static struct workqueue_struct *wq;
static struct work_struct Task;
static DECLARE_WORK(Task, intrpt_routine, NULL);

static void intrpt_routine(void *junk)
{
    
}

static int __init r0memdump_init(void)
{
	printk(KERN_INFO "loaded r0memdump\n");
	return 0;
}

static void __exit r0memdump_exit(void)
{
	printk(KERN_INFO "unloaded r0memdump\n");
}

module_init(r0memdump_init);
module_exit(r0memdump_exit);
