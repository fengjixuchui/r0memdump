#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>

#define PID_MAX 32768
#define procfs_name "r0memdump"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("null333");
MODULE_DESCRIPTION("ring 0 memory dumper, get mogged jimmy");
MODULE_VERSION("1.0");

static void purge_dead_pids(void);

static struct proc_dir_entry *proc_dir;

struct pmemdump_info
{
    // vmalloc this for 2x sizes for process (realloc if it grows, pretty much a ghetto vector)
    pid_t pid;
    char name[5];
    char* data, stack, heap, mmap;
    int64_t data_sz, stack_sz, heap_dz, mmap_sz;
    int cached;
    struct proc_dir_entry *proc_entry;
};

// replace with doubly linked list to save memory
static struct pmemdump_info pmemdumpcache[PID_MAX];

static void update_pids(void *);
static int die = 0;
static struct workqueue_struct *wq;
static struct work_struct Task;
static DECLARE_WORK(Task, update_pids, NULL);

static void update_pids(void *junk)
{
    // iterate through every process and generate proc dir for it, cache if not cached
    struct task_struct *task_list;
    for_each_process (task_list)
    {
        struct pmemdump_info cpmd_info = pmemdumpcache[task_list->pid];
        if(cpmd_info.cached == 0)
        {
            cpmd_info.pid = task_list->pid;
            sprintf(cpmd_info.name, "%d", cpmd_info.pid);
            cpmd_info.proc_entry = create_proc_entry(cpmd_info.name, 0666, proc_dir);
            // TODO: cache it lmfao
        }
    }

    purge_dead_pids();
}

static int __init r0memdump_init(void)
{
    proc_dir = proc_mkdir(procfs_name, NULL);

	printk(KERN_INFO "loaded r0memdump\n");
	return 0;
}

static void __exit r0memdump_exit(void)
{
    die = 1;
    cancel_delayed_work(&Task);
    flush_workqueue(wq);
    destroy_workqueue(wq);

    purge_dead_pids();
    remove_proc_entry(procfs_name, NULL);

    printk(KERN_INFO "unloaded r0memdump\n");
}

static void purge_dead_pids(void)
{
    pid_t i;
    for (i = 0; i < PID_MAX; i++)
    {
        if (pmemdumpcache[i].cached == 1 && find_task_by_vpid(i) == 0)
        {
            remove_proc_entry(pmemdumpcache[i].name, proc_dir);
        }
    }
}

module_init(r0memdump_init);
module_exit(r0memdump_exit);
