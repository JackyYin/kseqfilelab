#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h> // kzalloc
#include <linux/list.h> // linked list API
#include <linux/spinlock.h>
#include <linux/proc_fs.h> // proc_create API
#include <linux/seq_file.h> // struct seq_operations

struct my_seq_struct {
    struct list_head node;
    int val;
};

static LIST_HEAD(seqfile_list);

static long long seqfile_list_len = 0;

DEFINE_SPINLOCK(seqfile_list_spinlock);

static struct proc_dir_entry *pentry;

static void *seqop_start(struct seq_file *s, loff_t *pos)
{
    struct list_head *cur;

    pr_info("seqop_start, pos: %lld\n", *pos);
    cur = kmalloc(sizeof(struct list_head), GFP_KERNEL);
    if (!cur)
        return NULL;

    cur = ((struct list_head *)&seqfile_list)->next;

    if (*pos) {
        int offset = (*pos) % seqfile_list_len;

        for (; offset > 0; offset--)
            cur = cur->next;
    }

    return (cur == &seqfile_list) ? NULL : cur;
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
    pr_info("seqop_stop...\n");
    kfree(v);
}

static int seqop_show(struct seq_file *s, void *v)
{
    struct list_head *cur;
    struct my_seq_struct *obj;

    pr_info("seqop_show..., v: %px\n", v);

    if (!v)
        return -1;

    cur = v;
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
    return seq_open(file, &seq_ops);
}

static const struct proc_ops seq_file_ops = {
    .proc_open    = seqfile_open,
    .proc_read    = seq_read,
    .proc_lseek  = seq_lseek,
    .proc_release = seq_release
};

static int __init seqfile_init(void)
{
    pr_info("module init...\n");

    pentry = proc_create("seqsample", 0, NULL, &seq_file_ops);
    INIT_LIST_HEAD(&seqfile_list);

    for (; seqfile_list_len < 10; seqfile_list_len++) {
        struct my_seq_struct *obj = kzalloc(sizeof(struct my_seq_struct), GFP_KERNEL);
        if (!obj) {
            pr_info("failed to alloc...\n");
            return -1;
        }

        obj->val = seqfile_list_len;
        list_add_tail(&obj->node, &seqfile_list);
    }

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

    proc_remove(pentry);
    pr_info("module exit...\n");
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("simple lab for seq_file");
MODULE_AUTHOR("Jacky Yin");

module_init(seqfile_init);
module_exit(seqfile_exit);

