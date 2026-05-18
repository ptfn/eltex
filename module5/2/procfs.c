#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_NAME "test"
#define BUF_SIZE 256

static struct proc_dir_entry *proc_entry;
static char proc_buf[256];
static unsigned long proc_buf_len;

static int proc_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int proc_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t proc_read(struct file *file, char __user *user_buf,
                         size_t len, loff_t *off)
{
    return simple_read_from_buffer(user_buf, len, off, proc_buf, proc_buf_len);
}

static ssize_t proc_write(struct file *file, const char __user *user_buf,
                          size_t len, loff_t *off)
{
    if (len > 255)
        len = 255;

    if (copy_from_user(proc_buf, user_buf, len))
        return -EFAULT;

    proc_buf_len = len;
    return len;
}

static struct proc_ops proc_fops = {
    .proc_open    = proc_open,
    .proc_release = proc_release,
    .proc_read    = proc_read,
    .proc_write   = proc_write,
};

static int __init procfs_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0666, NULL, &proc_fops);
    if (!proc_entry) {
        pr_err("Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    pr_info("Created /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit procfs_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("Removed /proc/%s\n", PROC_NAME);
}

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ptfn");
MODULE_DESCRIPTION("eltex module5 task2: procfs example (old API)");
