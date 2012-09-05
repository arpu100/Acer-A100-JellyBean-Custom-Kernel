/*
 * Cleancache frontend
 *
 * This code provides the generic "frontend" layer to call a matching
 * "backend" driver implementation of cleancache.  See
 * Documentation/vm/cleancache.txt for more information.
 *
 * Copyright (C) 2009-2010 Oracle Corp. All rights reserved.
 * Author: Dan Magenheimer
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/exportfs.h>
#include <linux/mm.h>
<<<<<<< HEAD
#include <linux/debugfs.h>
=======
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
#include <linux/cleancache.h>

/*
 * This global enablement flag may be read thousands of times per second
<<<<<<< HEAD
 * by cleancache_get/put/invalidate even on systems where cleancache_ops
=======
 * by cleancache_get/put/flush even on systems where cleancache_ops
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
 * is not claimed (e.g. cleancache is config'ed on but remains
 * disabled), so is preferred to the slower alternative: a function
 * call that checks a non-global.
 */
<<<<<<< HEAD
int cleancache_enabled __read_mostly;
=======
int cleancache_enabled;
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
EXPORT_SYMBOL(cleancache_enabled);

/*
 * cleancache_ops is set by cleancache_ops_register to contain the pointers
 * to the cleancache "backend" implementation functions.
 */
<<<<<<< HEAD
static struct cleancache_ops cleancache_ops __read_mostly;

/*
 * Counters available via /sys/kernel/debug/frontswap (if debugfs is
 * properly configured.  These are for information only so are not protected
 * against increment races.
 */
static u64 cleancache_succ_gets;
static u64 cleancache_failed_gets;
static u64 cleancache_puts;
static u64 cleancache_invalidates;
=======
static struct cleancache_ops cleancache_ops;

/* useful stats available in /sys/kernel/mm/cleancache */
static unsigned long cleancache_succ_gets;
static unsigned long cleancache_failed_gets;
static unsigned long cleancache_puts;
static unsigned long cleancache_flushes;
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380

/*
 * register operations for cleancache, returning previous thus allowing
 * detection of multiple backends and possible nesting
 */
struct cleancache_ops cleancache_register_ops(struct cleancache_ops *ops)
{
	struct cleancache_ops old = cleancache_ops;

	cleancache_ops = *ops;
	cleancache_enabled = 1;
	return old;
}
EXPORT_SYMBOL(cleancache_register_ops);

/* Called by a cleancache-enabled filesystem at time of mount */
void __cleancache_init_fs(struct super_block *sb)
{
	sb->cleancache_poolid = (*cleancache_ops.init_fs)(PAGE_SIZE);
}
EXPORT_SYMBOL(__cleancache_init_fs);

/* Called by a cleancache-enabled clustered filesystem at time of mount */
void __cleancache_init_shared_fs(char *uuid, struct super_block *sb)
{
	sb->cleancache_poolid =
		(*cleancache_ops.init_shared_fs)(uuid, PAGE_SIZE);
}
EXPORT_SYMBOL(__cleancache_init_shared_fs);

/*
 * If the filesystem uses exportable filehandles, use the filehandle as
 * the key, else use the inode number.
 */
static int cleancache_get_key(struct inode *inode,
			      struct cleancache_filekey *key)
{
<<<<<<< HEAD
	int (*fhfn)(struct inode *, __u32 *fh, int *, struct inode *);
=======
	int (*fhfn)(struct dentry *, __u32 *fh, int *, int);
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
	int len = 0, maxlen = CLEANCACHE_KEY_MAX;
	struct super_block *sb = inode->i_sb;

	key->u.ino = inode->i_ino;
	if (sb->s_export_op != NULL) {
		fhfn = sb->s_export_op->encode_fh;
		if  (fhfn) {
<<<<<<< HEAD
			len = (*fhfn)(inode, &key->u.fh[0], &maxlen, NULL);
=======
			struct dentry d;
			d.d_inode = inode;
			len = (*fhfn)(&d, &key->u.fh[0], &maxlen, 0);
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
			if (len <= 0 || len == 255)
				return -1;
			if (maxlen > CLEANCACHE_KEY_MAX)
				return -1;
		}
	}
	return 0;
}

/*
 * "Get" data from cleancache associated with the poolid/inode/index
 * that were specified when the data was put to cleanache and, if
 * successful, use it to fill the specified page with data and return 0.
 * The pageframe is unchanged and returns -1 if the get fails.
 * Page must be locked by caller.
 */
int __cleancache_get_page(struct page *page)
{
	int ret = -1;
	int pool_id;
	struct cleancache_filekey key = { .u.key = { 0 } };

	VM_BUG_ON(!PageLocked(page));
	pool_id = page->mapping->host->i_sb->cleancache_poolid;
	if (pool_id < 0)
		goto out;

	if (cleancache_get_key(page->mapping->host, &key) < 0)
		goto out;

	ret = (*cleancache_ops.get_page)(pool_id, key, page->index, page);
	if (ret == 0)
		cleancache_succ_gets++;
	else
		cleancache_failed_gets++;
out:
	return ret;
}
EXPORT_SYMBOL(__cleancache_get_page);

/*
 * "Put" data from a page to cleancache and associate it with the
 * (previously-obtained per-filesystem) poolid and the page's,
 * inode and page index.  Page must be locked.  Note that a put_page
 * always "succeeds", though a subsequent get_page may succeed or fail.
 */
void __cleancache_put_page(struct page *page)
{
	int pool_id;
	struct cleancache_filekey key = { .u.key = { 0 } };

	VM_BUG_ON(!PageLocked(page));
	pool_id = page->mapping->host->i_sb->cleancache_poolid;
	if (pool_id >= 0 &&
	      cleancache_get_key(page->mapping->host, &key) >= 0) {
		(*cleancache_ops.put_page)(pool_id, key, page->index, page);
		cleancache_puts++;
	}
}
EXPORT_SYMBOL(__cleancache_put_page);

/*
<<<<<<< HEAD
 * Invalidate any data from cleancache associated with the poolid and the
 * page's inode and page index so that a subsequent "get" will fail.
 */
void __cleancache_invalidate_page(struct address_space *mapping,
					struct page *page)
=======
 * Flush any data from cleancache associated with the poolid and the
 * page's inode and page index so that a subsequent "get" will fail.
 */
void __cleancache_flush_page(struct address_space *mapping, struct page *page)
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
{
	/* careful... page->mapping is NULL sometimes when this is called */
	int pool_id = mapping->host->i_sb->cleancache_poolid;
	struct cleancache_filekey key = { .u.key = { 0 } };

	if (pool_id >= 0) {
		VM_BUG_ON(!PageLocked(page));
		if (cleancache_get_key(mapping->host, &key) >= 0) {
<<<<<<< HEAD
			(*cleancache_ops.invalidate_page)(pool_id,
							  key, page->index);
			cleancache_invalidates++;
		}
	}
}
EXPORT_SYMBOL(__cleancache_invalidate_page);

/*
 * Invalidate all data from cleancache associated with the poolid and the
 * mappings's inode so that all subsequent gets to this poolid/inode
 * will fail.
 */
void __cleancache_invalidate_inode(struct address_space *mapping)
=======
			(*cleancache_ops.flush_page)(pool_id, key, page->index);
			cleancache_flushes++;
		}
	}
}
EXPORT_SYMBOL(__cleancache_flush_page);

/*
 * Flush all data from cleancache associated with the poolid and the
 * mappings's inode so that all subsequent gets to this poolid/inode
 * will fail.
 */
void __cleancache_flush_inode(struct address_space *mapping)
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
{
	int pool_id = mapping->host->i_sb->cleancache_poolid;
	struct cleancache_filekey key = { .u.key = { 0 } };

	if (pool_id >= 0 && cleancache_get_key(mapping->host, &key) >= 0)
<<<<<<< HEAD
		(*cleancache_ops.invalidate_inode)(pool_id, key);
}
EXPORT_SYMBOL(__cleancache_invalidate_inode);
=======
		(*cleancache_ops.flush_inode)(pool_id, key);
}
EXPORT_SYMBOL(__cleancache_flush_inode);
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380

/*
 * Called by any cleancache-enabled filesystem at time of unmount;
 * note that pool_id is surrendered and may be reutrned by a subsequent
 * cleancache_init_fs or cleancache_init_shared_fs
 */
<<<<<<< HEAD
void __cleancache_invalidate_fs(struct super_block *sb)
=======
void __cleancache_flush_fs(struct super_block *sb)
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
{
	if (sb->cleancache_poolid >= 0) {
		int old_poolid = sb->cleancache_poolid;
		sb->cleancache_poolid = -1;
<<<<<<< HEAD
		(*cleancache_ops.invalidate_fs)(old_poolid);
	}
}
EXPORT_SYMBOL(__cleancache_invalidate_fs);

static int __init init_cleancache(void)
{
#ifdef CONFIG_DEBUG_FS
	struct dentry *root = debugfs_create_dir("cleancache", NULL);
	if (root == NULL)
		return -ENXIO;
	debugfs_create_u64("succ_gets", S_IRUGO, root, &cleancache_succ_gets);
	debugfs_create_u64("failed_gets", S_IRUGO,
				root, &cleancache_failed_gets);
	debugfs_create_u64("puts", S_IRUGO, root, &cleancache_puts);
	debugfs_create_u64("invalidates", S_IRUGO,
				root, &cleancache_invalidates);
#endif
=======
		(*cleancache_ops.flush_fs)(old_poolid);
	}
}
EXPORT_SYMBOL(__cleancache_flush_fs);

#ifdef CONFIG_SYSFS

/* see Documentation/ABI/xxx/sysfs-kernel-mm-cleancache */

#define CLEANCACHE_SYSFS_RO(_name) \
	static ssize_t cleancache_##_name##_show(struct kobject *kobj, \
				struct kobj_attribute *attr, char *buf) \
	{ \
		return sprintf(buf, "%lu\n", cleancache_##_name); \
	} \
	static struct kobj_attribute cleancache_##_name##_attr = { \
		.attr = { .name = __stringify(_name), .mode = 0444 }, \
		.show = cleancache_##_name##_show, \
	}

CLEANCACHE_SYSFS_RO(succ_gets);
CLEANCACHE_SYSFS_RO(failed_gets);
CLEANCACHE_SYSFS_RO(puts);
CLEANCACHE_SYSFS_RO(flushes);

static struct attribute *cleancache_attrs[] = {
	&cleancache_succ_gets_attr.attr,
	&cleancache_failed_gets_attr.attr,
	&cleancache_puts_attr.attr,
	&cleancache_flushes_attr.attr,
	NULL,
};

static struct attribute_group cleancache_attr_group = {
	.attrs = cleancache_attrs,
	.name = "cleancache",
};

#endif /* CONFIG_SYSFS */

static int __init init_cleancache(void)
{
#ifdef CONFIG_SYSFS
	int err;

	err = sysfs_create_group(mm_kobj, &cleancache_attr_group);
#endif /* CONFIG_SYSFS */
>>>>>>> a53f3f4063b88e140d8c242128b7aa4e3316a380
	return 0;
}
module_init(init_cleancache)