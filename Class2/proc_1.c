#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEFAULT_COUNT 10

static int len = 0;
static int temp = 0;
static char *msg = NULL;

ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp) {
    if (temp <= 0) {
        return 0;
    }
    if (count > temp) {
        count = temp;
    }
    if (copy_to_user(buf, msg + (len - temp), count)) {
        return -EFAULT;
    }
    temp -= count;
    return count;
}

ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp) {
    if (count > DEFAULT_COUNT) {
        count = DEFAULT_COUNT;
    }
    if (copy_from_user(msg, buf, count)) {
        return -EFAULT;
    }
    len = count;
    temp = len;
    return count;
}

static const struct proc_ops proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

void create_new_proc_entry(void) {
    msg = kmalloc(DEFAULT_COUNT * sizeof(char), GFP_KERNEL);
    if (!msg) {
        pr_err("Failed to allocate memory for msg\n");
        return;
    }
    proc_create("Class2", 0, NULL, &proc_fops);
}

int proc_init(void) {
    create_new_proc_entry();
    return 0;
}

void proc_cleanup(void) {
    remove_proc_entry("Class2", NULL);
    kfree(msg);
}

MODULE_LICENSE("COOKIE");
MODULE_AUTHOR("Sofico");
module_init(proc_init);
module_exit(proc_cleanup);