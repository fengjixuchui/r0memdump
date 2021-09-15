#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/memory.h>
#include <linux/seq_file.h>
#include <linux/pagemap.h>

#define PROCFS_NAME "r0memdump"
#define __PAGESIZE 4096

MODULE_LICENSE("GPL");
MODULE_AUTHOR("null333");
MODULE_DESCRIPTION("ring 0 memory dumper, get mogged jimmy");
MODULE_VERSION("1.0");

// TODO: replace with seqfile, on seqfile open, map all pages into kernel mem, on close unmap

char zero_page[__PAGESIZE] = {0};

static struct proc_dir_entry *proc_dir;
static int procfile_open(struct inode *inode,struct file *file);
static ssize_t procfile_read(struct file *file, char __user *buffer, size_t count, loff_t *offset);
static struct proc_ops procfs_ops =
{
    .proc_open = procfile_open,
    .proc_read = procfile_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release
};
// static char procfs_buffer[PROCFS_MAX_SIZE];
// static unsigned long procfs_buffer_size = 0;


struct pmemdump_info
{
    // vmalloc this for 2x sizes for process (realloc if it grows, pretty much a ghetto vector)
    pid_t pid;
    char name[6]; // PID_MAX + null
    struct proc_dir_entry *proc_entry;
    struct pmemdump_info *prev;
    struct pmemdump_info *next;
};
static struct pmemdump_info *pmd_info_list = NULL;

static void update_pids(struct work_struct *w);
static struct workqueue_struct *wq = 0;
// static DECLARE_DELAYED_WORK(update_pids_work, update_pids);
static DECLARE_WORK(update_pids_work, update_pids);

static void update_pids(struct work_struct *w)
{
    // iterate through every process and generate a proc entry if it doesnt exist already
    struct task_struct *task_list;
    for_each_process (task_list)
    {
        int in_list = 0;
        struct pmemdump_info *cpmd_info = pmd_info_list;
        if (cpmd_info)
        {
            while (cpmd_info->next != NULL)
            {
                if (cpmd_info->pid == task_list->pid)
                {
                    in_list = 1;
                    break;
                }
                cpmd_info = cpmd_info->next;
            }
        }
        else
        {
            // init pmd_info_list
            cpmd_info = (struct pmemdump_info *) vmalloc(sizeof(struct pmemdump_info));
            cpmd_info->next = NULL;
            cpmd_info->prev = NULL;
            cpmd_info->pid = task_list->pid;
            sprintf(cpmd_info->name, "%d", cpmd_info->pid);
            cpmd_info->proc_entry = proc_create(cpmd_info->name, 0777, proc_dir, &procfs_ops);
            pmd_info_list = cpmd_info;
            in_list = 1;
        }
        if (in_list == 0)
        {
            struct pmemdump_info *tmp = cpmd_info;
            cpmd_info->next = (struct pmemdump_info *) vmalloc(sizeof(struct pmemdump_info));
            cpmd_info = cpmd_info->next; // FIX; null pointer deref here ?
            cpmd_info->prev = tmp;
            cpmd_info->pid = task_list->pid;
            sprintf(cpmd_info->name, "%d", cpmd_info->pid);
            cpmd_info->proc_entry = proc_create(cpmd_info->name, 0666, proc_dir, &procfs_ops);
        }
    }

    // purge dead pids
    struct pmemdump_info *cpmd_info = pmd_info_list;
    if (cpmd_info)
    {
        while (cpmd_info->next != NULL)
        {
            struct pmemdump_info *tmp = cpmd_info->next;
            if (find_vpid(cpmd_info->pid) == 0)
            {
                proc_remove(cpmd_info->proc_entry);
                cpmd_info->prev->next = cpmd_info->next;
                vfree(cpmd_info);
            }
            cpmd_info = tmp;
        }
    }
}

static char *nullstr = NULL;
static int procfile_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%s\n", nullstr);
    return 0;
}

static int procfile_open(struct inode *inode, struct file *file)
{
    return single_open(file, procfile_show, NULL);
}

static ssize_t procfile_read(struct file *file, char __user *buffer, size_t count, loff_t *offset)
{
    // TODO: replace with seqfile
    struct pmemdump_info *cpmd_info = pmd_info_list;
    if (cpmd_info)
    {
        while (cpmd_info->next != NULL)
        {
            if (strcmp(cpmd_info->name, file->f_path.dentry->d_name.name) == 0)
            {
                // // TODO: first copy from user with pid of requested proc to tmp buffer
                // procfs_buffer_size = 0;
                // memset(procfs_buffer, 0, PROCFS_MAX_SIZE);
                // memcpy(procfs_buffer, cpmd_info->name, strlen(cpmd_info->name));
                // procfs_buffer_size = strlen(cpmd_info->name);
                // printk("r0memdump DEBUG -- mem ops passed");
                //
                // if (*offset > 0 || count < PROCFS_MAX_SIZE)
                // {
                //     printk("r0memdump DEBUG -- returned");
                //     return 0;
                // }
                // printk("r0memdump DEBUG -- procfs_buffer contents: %s", procfs_buffer);
                // copy_to_user(buffer, procfs_buffer, procfs_buffer_size);
                // *offset = procfs_buffer_size;
                // return procfs_buffer_size;


                // TODO: remove procfs_buffer
                // TODO: loop copying from task to tmp buffer, then tmp to buffer
                // TODO: get stack offset from /proc/self to get bottom of stack
                struct task_struct *from_ts = find_task_by_vpid(cpmd_info->pid);
                // stackoverflow post never allocates this struct ??
                // https://stackoverflow.com/questions/36337942/how-does-get-user-pages-work-for-linux-driver
                struct page *from_page; // = vmalloc(sizeof(struct page));

                uintptr_t c_addr;
                for (c_addr = 0; c_addr < TASK_SIZE; c_addr += __PAGESIZE)
                {
                    // down_read(&from_ts->mm->mmap_sem);
                    // get_user_pages(from_ts, from_ts->mm, c_addr, 1, 0, 0, &from_page, NULL);
                    get_user_pages_remote(from_ts->mm, c_addr, 0, &from_page, NULL, NULL);
                    if (from_page->mapping == NULL)
                    {
                        void *kaddr = kmap(from_page);
                        // copy directly from mapped page without tmp buffer
                        copy_to_user((void *) ((char *) &buffer[0] + c_addr), kaddr, __PAGESIZE);
                    }
                    else
                    {
                        // if page isnt mapped zero it
                        copy_to_user((void *) ((char *) &buffer[0] + c_addr), zero_page, __PAGESIZE);
                    }

                    vfree(from_page);
                    page_cache_release(from_page);
                    // up_read(&from_ts->mm->mmap_sem);
                }
            }
            cpmd_info = cpmd_info->next;
        }
    }
    return 0;
}

static int __init r0memdump_init(void)
{
    proc_dir = proc_mkdir(PROCFS_NAME, NULL);

    if (!wq)
    {
        wq = create_singlethread_workqueue("update_pids_wq");
    }
    if (wq)
    {
        queue_work(wq, &update_pids_work);
    }

	printk(KERN_INFO "loaded r0memdump\n");
	return 0;
}

static void __exit r0memdump_exit(void)
{
    cancel_work_sync(&update_pids_work);
    destroy_workqueue(wq);

    proc_remove(proc_dir);
    struct pmemdump_info *cpmd_info = pmd_info_list;
    if (cpmd_info)
    {
        while (cpmd_info->next != NULL)
        {
            struct pmemdump_info *tmp = cpmd_info->next;
            proc_remove(cpmd_info->proc_entry);
            vfree(cpmd_info);
            cpmd_info = tmp;
        }
    }

    printk(KERN_INFO "unloaded r0memdump\n");
}

module_init(r0memdump_init);
module_exit(r0memdump_exit);
