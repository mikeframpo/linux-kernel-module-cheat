/* https://cirosantilli.com/linux-kernel-module-cheat#qemu-buildroot-setup-getting-started */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>

int scull_major;
int scull_minor = 0;

struct scull_dev {
	struct cdev *cdev;	  /* Char device structure		*/
};

static struct scull_dev scull_device;

static char *scull_data = "test\n";
static loff_t read_pos;

// TODO
// * D commit work on dotfiles, change to high-contrast colorscheme, add relativenumber
// * create a repo for storing this work, or use existing
// * copy this skeleton, rename scull to ringdev or similar
// * create a simple ring buffer as a lock-free IPC mechanism
// 	-> need to learn how to lock file to one producer, one consumer

int scull_open(struct inode *inod, struct file *filp)
{
	// get cdev from inode
	// use container_of to get the parent structure (priv data)
	// assign the priv-data to the file pointer
	// return 0
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int err;
	int remaining;

	pr_info("scull_read with ppos: %lld\n", *ppos);

	remaining = strlen(scull_data) - *ppos;

	if (remaining > 0) {
		err = copy_to_user(buf, &scull_data[*ppos], remaining);
		if (err)
			return -1;

		*ppos += remaining;
		return remaining;
	}

	return 0;
}

struct file_operations scull_fops = {
	.open = scull_open,
	.read = scull_read
};

static int scull_setup_cdev(struct scull_dev *dev)
{
	int err = 0;
	int devno = MKDEV(scull_major, scull_minor);

	dev->cdev = cdev_alloc();
	if (!dev->cdev) {
		err = -ENOMEM;
		return err;
	}
    
	cdev_init(dev->cdev, &scull_fops);
	dev->cdev->owner = THIS_MODULE;

	err = cdev_add (dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding scull cdev", err);

	return err;
}

static void scull_cleanup(void)
{
	dev_t dev;

	dev = MKDEV(scull_major, scull_minor);
	unregister_chrdev_region(dev, 1);

	if (scull_device.cdev)
		cdev_del(scull_device.cdev);
}

static int myinit(void)
{
	int result;
	dev_t dev = 0;

	pr_info("scull init\n");
	/* 0 for success, any negative value means failure,
	 * E* consts if you want to specify failure cause.
	 * https://www.linux.com/learn/kernel-newbie-corner-loadable-kernel-modules-coming-and-going */

	result = alloc_chrdev_region(&dev, scull_minor, 1, "scull");
	scull_major = MAJOR(dev);

	if (result < 0) {
		printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
		return result;
	}

	memset(&scull_device, 0, sizeof(struct scull_dev));
	result = scull_setup_cdev(&scull_device);

	if (result < 0) {
		scull_cleanup();
		return result;
	}

	read_pos = 0;

	return 0;
}

static void myexit(void)
{
	pr_info("scull exit\n");
	scull_cleanup();
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
