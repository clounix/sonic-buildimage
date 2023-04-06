#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#define MAX_NAME_LEN (PAGE_SIZE/2)
#define MIN_PATH_LEN (16)
#define SYSFS_ROOT "/sys/"
#define CLOUNIX_DIR_NAME "switch"

#define PATH_FORMAT "Format:\r\nFor create: target_node path link_node_path.\r\nFor del: del link_node_path\r\nThe path format must be full path like \"/sys/bus/...\"\r\n"

DEFINE_SPINLOCK(sysfs_rw_lock);

static struct kobject *clounix_switch;

static char *skip_root(char *path)
{
	if (memcmp(path, SYSFS_ROOT, 5) == 0) {
		path += 5;
		return path;
	}

	return NULL;
}

static void traverse_sysfs_by_rbtree(struct kernfs_node *parent)
{
	struct rb_node *node;
	struct kernfs_node *sd;

	if (parent == NULL)
		return;

	node = rb_first(&(parent->dir.children));

	while (node != NULL) {
		sd = container_of(node, struct kernfs_node, rb);
		printk(KERN_ALERT "node name: %s \t type: %x parent dir %s\r\n", sd->name ? sd->name : "null", sd->flags, sd->parent->name);
		node = rb_next(node);
	}
}

static struct kernfs_node *search_sysfs_node_by_name(struct kernfs_node *root, char *name)
{
	struct rb_node *node;
	struct kernfs_node *sd;

	if (root == NULL)
		return NULL;
	
	if ((root->flags & KERNFS_LINK) != 0) {
		root = root->symlink.target_kn;
	}

	node = rb_first(&(root->dir.children));
	while (node != NULL) {
		sd = container_of(node, struct kernfs_node, rb);
		if (strcmp(sd->name, name) == 0) {
			printk(KERN_ALERT "find sysfs node: %s\r\n", sd->name);
			return sd;
		}
		node = rb_next(node);
	}

	printk(KERN_ALERT "not find sysfs node: %s\r\n", name);
	traverse_sysfs_by_rbtree(root);

	return NULL;
}

static struct kernfs_node *search_sysfs_node_by_path(struct kernfs_node *root, char *path)
{
	char *start, *end;
	struct kernfs_node *sd = root;

	if (path == NULL || root == NULL)
		goto out;

	if ((start = skip_root(path)) == NULL)
		return NULL;

	while (start != NULL) {
		if ((end = strchr(start, '/')) != NULL) {
			*end = '\0';
			sd = search_sysfs_node_by_name(sd, start);
			if (sd == NULL)
				goto out;

			start = end+1;
		} else {
			sd = search_sysfs_node_by_name(sd, start);
			break;
		}
	}
	
	return sd;

out:
	return NULL; 
}

static int create_link_atomic(struct kernfs_node *root, struct kernfs_node *target, char *path)
{
	struct kernfs_node *sd;
	struct kobject *kobj, *t;
	char *start, *end;
	int err_no;

	if ((start = skip_root(path)) == NULL)
		return -EACCES;

	while (start != NULL) {
		if ((end = strchr(start, '/')) != NULL) {
			*end = '\0';
			sd = search_sysfs_node_by_name(root, start);
			if (sd == NULL) {
				if ((kobj = kobject_create_and_add(start, root->priv)) == NULL)
					return -ENOMEM;
				printk(KERN_EMERG "create missing dir %s\r\n", kobj->sd->name);
				sd = kobj->sd;
			}
			root = sd;
			start = end+1;
		} else {
			kobj = root->priv;
			break;
		}
	}

	if ((sd = search_sysfs_node_by_name(root, start)) != NULL) {
		printk(KERN_EMERG "link exist\r\n");
		return -EEXIST;
	}

	t = target->priv;
	if (t->sd == NULL && (target->flags & KERNFS_FILE) != 0) {
		t->sd = target;
	}

	if ((err_no = sysfs_create_link(kobj, t, start)) != 0) {
		printk(KERN_EMERG "fail to create link %s in dir %s\r\n", start, kobj->sd->name);
		return err_no;	
	}

	return 0;
}

static char *format_check(const char *buf)
{
	char *p = (char *)buf;

	if ((p = strchr(p, ' ')) != NULL) {
		if (*(p+1) != ' ')
			return p;
		else
			return NULL;
	}

	return NULL;
}

static unsigned int is_del_link(struct kernfs_node *root, char *src, char *link)
{
	struct kernfs_node *sd;

	printk(KERN_EMERG "----%s %s\r\n", src, link);
	if (memcmp(src, "del", 3) != 0)
		return 0;

	spin_lock_bh(&sysfs_rw_lock);
	sd = search_sysfs_node_by_path(root, link);	
	spin_unlock_bh(&sysfs_rw_lock);
	if (sd == NULL) {
		printk("del target not found\r\n");
		return 1;
	}
	
	spin_lock_bh(&sysfs_rw_lock);
	sysfs_remove_link(sd->parent->priv,  sd->name);
	spin_unlock_bh(&sysfs_rw_lock);

	return 1;
}

static ssize_t cmd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t cmd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct kernfs_node *root = clounix_switch->sd->parent;
	struct kernfs_node *target;
	char *src, *link;
	char *p;
	
	if (count <= MIN_PATH_LEN)
		goto format_err;

	p = format_check(buf);
	if (p == NULL)
		goto format_err;

	src = kzalloc(PAGE_SIZE, GFP_ATOMIC);
	if (src == NULL) {
		printk(KERN_EMERG "Alloc mem failed\r\n");
		goto mem_err;
	}

	memcpy(src, buf, count-1);
	link = src + (p - buf);
	*link = '\0';
	link++;
	
	if (is_del_link(root, src, link) == 1)
		goto del_out;

	target = search_sysfs_node_by_path(root, src);
	if (target == NULL) {
		printk(KERN_EMERG "not find %s\r\n", src);
		goto path_err;
	}

	if ((target->flags & KERNFS_LINK) != 0)
		target = target->symlink.target_kn;

	spin_lock_bh(&sysfs_rw_lock);
	create_link_atomic(root, target, link);
	spin_unlock_bh(&sysfs_rw_lock);

del_out:
	kfree(src);
	return count;

path_err:
	kfree(src);
mem_err:
format_err:
	printk(KERN_ALERT PATH_FORMAT); 
	return count;
}

static struct kobj_attribute ctrl_attr = __ATTR(clounix_cmd, 0644, cmd_show, cmd_store);

static struct attribute * ctrl_attrs[] = {
	&ctrl_attr.attr,
	NULL
};

static const struct attribute_group control_attr_group = {
	.attrs = ctrl_attrs,
};

static int __init main_init(void)
{
	int err_no = 0;

	printk(KERN_EMERG "clounix kobj node_init!\r\n");
	
	clounix_switch = kobject_create_and_add(CLOUNIX_DIR_NAME, NULL);
	if (clounix_switch == NULL)
		return -ENOMEM;
	
	if ((err_no = sysfs_create_group(clounix_switch, &control_attr_group)) != 0)
		goto err_out;

	return 0;

err_out:
	printk(KERN_ALERT "has err_no %d\r\n", err_no);
	kobject_put(clounix_switch);
	return -ENOMEM;
}

static void __exit main_exit(void)
{
	printk(KERN_ALERT "clounix kobj node_del!\r\n");

	spin_lock_bh(&sysfs_rw_lock);
	sysfs_remove_group(clounix_switch, &control_attr_group);
	kobject_put(clounix_switch);
	spin_unlock_bh(&sysfs_rw_lock);
}

module_init(main_init);
module_exit(main_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("baohx@clounix.com");
