#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h> // kzalloc
#include <linux/list.h> // linked list API
#include <linux/spinlock.h>
#include <linux/proc_fs.h> // proc_create API
#include <linux/seq_file.h> // struct seq_operations
#include <linux/kstrtox.h> // kstrtol

struct my_seq_struct {
    struct list_head node;
    long val;
};

static LIST_HEAD(seqfile_list);

static long long seqfile_list_len = 1;

DEFINE_SPINLOCK(seqfile_list_spinlock);

static struct proc_dir_entry *pentry_dir;
static struct proc_dir_entry *pentry_sample;

static int my_seq_insert(long val)
{
    struct my_seq_struct *obj = kzalloc(sizeof(struct my_seq_struct), GFP_KERNEL);
    if (!obj) {
        pr_info("failed to alloc...\n");
        return -ENOMEM;
    }

    obj->val = val;
    list_add_tail(&obj->node, &seqfile_list);
    seqfile_list_len++;
    return 0;
}

static void *seqop_start(struct seq_file *s, loff_t *pos)
{
    struct list_head **cur;

    if (!s->private)
        return NULL;

    pr_info("seqop_start, pos: %lld, ptr: %px\n", *pos, s->private);

    cur = (struct list_head **)s->private;
    *cur = ((struct list_head *)&seqfile_list)->next;

    if (*pos) {
        int offset = (*pos) % seqfile_list_len;
        for (; offset > 0; offset--)
            *cur = (*cur)->next;
    }

    if (*cur == &seqfile_list) {
        *pos = 0;
        *cur = (*cur)->next;
    }
    return *cur;
    /* return (*cur == &seqfile_list) ? NULL : *cur; */
}

static void *seqop_next(struct seq_file *s, void *v, loff_t *pos)
{
    struct list_head *cur = v;

    pr_info("seqop_next..., pos: %lld\n", *pos);
    (*pos)++;
    cur = cur->next;

    return (cur == &seqfile_list) ? NULL : cur;
}

static void seqop_stop(struct seq_file *s, void *v)
{
    pr_info("seqop_stop..., v: %px\n", v);
}

static int seqop_show(struct seq_file *s, void *v)
{
    struct list_head *cur;
    struct my_seq_struct *obj;

    pr_info("seqop_show..., v: %px\n", v);

    if (!v)
        return -1;

    cur = v;
    if (cur == &seqfile_list)
        return -ENODATA;

    obj = container_of(cur, struct my_seq_struct, node);
    seq_printf(s, "%d\n", (int)obj->val);
    return 0;
}

static const struct seq_operations seq_ops = {
    .start = seqop_start,
    .next  = seqop_next,
    .stop  = seqop_stop,
    .show  = seqop_show
};

static int seqfile_open(struct inode *inode, struct file *file)
{
    pr_info("seq file opened...\n");
    return seq_open_private(file, &seq_ops, sizeof(struct list_head *));
}


static ssize_t seqfile_write(struct file * file, const char __user *buf, size_t count, loff_t *offset)
{
    char tmp[32] = {0};
    long res;
    int rc;

    if (!count || count > sizeof(tmp))
        return -EIO;

    if (copy_from_user(tmp, buf, count)) {
        pr_warn("copy_from_user failed...\n");
        return -EIO;
    }

    if ((rc = kstrtol(tmp, 10, &res))) {
        pr_warn("kstrtol failed...\n");
        return rc;
    }

    if ((rc = my_seq_insert(res)))
        return rc;

    return count;
}

static const struct proc_ops seq_file_ops = {
    .proc_open    = seqfile_open,
    .proc_read    = seq_read,
    .proc_write   = seqfile_write,
    .proc_lseek   = seq_lseek,
    .proc_release = seq_release
};

static bool procfs_init(void)
{
    pentry_dir = proc_mkdir("seqfilelab", NULL);
    if (!pentry_dir) {
        return false;
    }

    pentry_sample = proc_create("sample", 0, pentry_dir, &seq_file_ops);
    if (!pentry_sample) {
        proc_remove(pentry_dir);
        return false;
    }

    return true;
}

static int __init seqfile_init(void)
{
    pr_info("module init...\n");

    if (!procfs_init()) {
        pr_warn("failed to init procfs entry...\n");
        return -1;
    }

    INIT_LIST_HEAD(&seqfile_list);

    return 0;
}

static void __exit seqfile_exit(void)
{
    struct my_seq_struct *cur = NULL, *tmp;

    spin_lock(&seqfile_list_spinlock);
    list_for_each_entry_safe(cur, tmp, &seqfile_list, node) {
        kfree(cur);
    }
    spin_unlock(&seqfile_list_spinlock);

    proc_remove(pentry_sample);
    proc_remove(pentry_dir);
    pr_info("module exit...\n");
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("simple lab for seq_file");
MODULE_AUTHOR("Jacky Yin");

module_init(seqfile_init);
module_exit(seqfile_exit);

