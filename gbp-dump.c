#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/gbp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Marchi");
MODULE_DESCRIPTION("A module to debug global breakpoints.");

static void *my_start(struct seq_file *s, loff_t *pos) {
	struct list_head *it;
	int i = 0;

	printk("my_start\n");

	list_for_each(it, &gbp_sessions) {
		printk("boo %p\n", it);
		if (*pos == i) {
			break;
		}

		i++;
	}

	// *pos is too big
	if (it == &gbp_sessions) {
		printk("done\n");
		return NULL;
	}

	printk("After start: %p\n", it);

	return it;
}

static void *my_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct list_head *it = v;

	printk("my_next\n");

	it = it->next;

	if (it == &gbp_sessions) {
		return NULL;
	}

	(*pos)++;

	printk("Afte next: %p\n", it);

	return it;
}

static void my_stop(struct seq_file *s, void *v)
{
	printk("my_stop\n");
}

static int my_show(struct seq_file *s, void *v)
{
	struct list_head *it = v;
	struct gbp_session *sess = list_entry(it, struct gbp_session, node);
	int i;
	int printed_blocked_header = 0;

	printk("my_show\n");

	if (sess) {
		struct gbp_bp *bp;
		seq_printf(s, "Session %p\n", v);
		if (!list_empty(&sess->bp_list)) {
			seq_printf(s, "  breakpoints:\n");
			list_for_each_entry(bp, &sess->bp_list, node) {
				seq_printf(s, "    #%d at 0x%lx\n", bp->bp_id, bp->offset);
			}
		}

		for (i = 0; i < GBP_MAX_ENTRY_SLOTS; i++) {
			if (sess->bp_slots & (1UL << i)) {
				struct task_struct *blocked = sess->blocked_task[i];

				if (!printed_blocked_header) {
					seq_printf(s, "  blocked tasks:\n");
					printed_blocked_header = 1;
				}
				seq_printf(s, "    pid=%d ptrace=%d\n", blocked->pid, blocked->ptrace);
			}
		}
	}
	return 0;
}

static struct seq_operations my_seq_ops = {
	.start = my_start,
	.next = my_next,
	.stop = my_stop,
	.show = my_show,
};


static int my_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &my_seq_ops);
}

struct file_operations gbp_proc_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
struct proc_dir_entry *proc_entry = NULL;

static int __init gbp_init(void)
{
	printk(KERN_INFO "Initializing gbp.\n");

	proc_entry = proc_create_data("gbp", 0, NULL, &gbp_proc_fops, NULL);

	if (!proc_entry)
		goto fail;

	return 0;
fail:
	return 1;
}

static void __exit gbp_cleanup(void)
{
	printk(KERN_INFO "Cleaning up module.\n");
	proc_remove(proc_entry);
}

module_init(gbp_init);
module_exit(gbp_cleanup);
