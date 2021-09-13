#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>


#define procfs_name "r0memdump"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("null333");
MODULE_DESCRIPTION("ring 0 memory dumper, get mogged jimmy");
MODULE_VERSION("1.0");

static struct proc_dir_entry *proc_dir;

struct pmemdump_info
{
    // vmalloc this for 2x sizes for process (realloc if it grows, pretty much a ghetto vector)
    pid_t pid;
    char name[5];
    struct proc_dir_entry *proc_entry;
    struct pmemdump_info *prev;
    struct pmemdump_info *next;
};

// replace with doubly linked list to save memory
static struct pmemdump_info *pmd_info_list;

static struct file_operations proc_ops =
{
	.owner = THIS_MODULE
};

static void update_pids(void *);
static int die = 0;
static struct workqueue_struct *wq;
static struct work_struct Task;
static DECLARE_WORK(Task, update_pids, NULL);

static void update_pids(void *junk)
{
    // iterate through every process and generate a proc entry if it doesnt exist already
    struct task_struct *task_list;
    for_each_process (task_list)
    {
        int in_list = 0;
        struct pmemdump_info *cpmd_info = pmd_info_list;
        while (cpmd_info->next != NULL)
        {
            if (cpmd_info->pid == task_list->pid)
            {
                in_list = 1;
                break;
            }
        }
        if (in_list == 0)
        {
            struct pmemdump_info *tmp = cpmd_info;
            cpmd_info->next = (struct pmemdump_info *) vmalloc(sizeof(struct pmemdump_info));
            cpmd_info = cpmd_info->next;
            cpmd_info->prev = tmp;
            cpmd_info->pid = task_list->pid;
            sprintf(cpmd_info->name, "%d", cpmd_info->pid);
            cpmd_info->proc_entry = proc_create(cpmd_info->name, 0666, proc_dir, &proc_ops);
        }
    }

    // purge dead pids
    struct pmemdump_info *cpmd_info = pmd_info_list;
    while (cpmd_info->next != NULL)
    {
        if (find_task_by_vpid(cpmd_info->pid) == 0)
        {
            proc_remove(cpmd_info->proc_entry);
            cpmd_info->prev->next = cpmd_info->next;
            vfree(cpmd_info);
        }
    }
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

    proc_remove(proc_dir);
    struct pmemdump_info *cpmd_info = pmd_info_list;
    while (cpmd_info->next != NULL)
    {
        struct pmemdump_info *tmp = cpmd_info->next;
        proc_remove(cpmd_info->proc_entry);
        vfree(cpmd_info);
        cpmd_info = tmp;
    }

    printk(KERN_INFO "unloaded r0memdump\n");
}

module_init(r0memdump_init);
module_exit(r0memdump_exit);
