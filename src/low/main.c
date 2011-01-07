/**
 * Tikle kernel module
 * Copyright (C) 2009	Felipe 'ecl' Pena
 *			Gustavo 'nst' Oliveira
 * 
 * Contact us at:	flipensp@gmail.com
 * 			gmoliveira@inf.ufrgs.br
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Also thanks to Higor 'enygmata' Euripedes
 */

#include <linux/version.h> /* version checking */
#include <linux/module.h> /* well this is a module, right? */
#include <linux/kernel.h> /* KERN_INFO stuff */

#include <net/net_namespace.h> /* init_net.proc_net declaration */
#include <linux/proc_fs.h> /* /proc management functions */
#include <linux/slab.h> /* kmalloc(); */
#include <linux/kthread.h> /* kthread_run(); */
#include <linux/smp_lock.h> /* lock_kernel(); and related stuff */
#include <linux/sched.h> /* kill_pid(); */
#include <linux/pid.h> /* struct pid header */

#include "low_defs.h" /* tikle global definitions */
#include "low_funcs.h" /* tikle functions prototypes */

/**
 * external definition
 * for /proc tree
 */
procfs_t *sysfs;

/**
 * external definition
 * for main thread control
 */
struct task_struct *main_thread;

/**
 * callback function for /proc/net/tikle/status
 */
static int f_read_status(char *page, char **start,
			     off_t off, int count,
			     int *eof, void *data)
{
	int len;

	len = sprintf(page, "here comes the status from somewhere :D\n");

	return len;
}

/**
 * callback function for /proc/net/tikle/shell
 */
static int f_write_shell(struct file *file,
			     const char *buffer,
			     unsigned long count, 
			     void *data)
{
	char *usr_buff = kmalloc(count, GFP_KERNEL);

	if (!usr_buff)
		return 1;

	if (copy_from_user(usr_buff, buffer, count))
		return -EFAULT;

	usr_buff[count] = '\0';
	printk(KERN_INFO "%s\n", usr_buff);

	return count;
}

/**
 * Module initialization routines.
 *
 * This function register /proc/net/tikle directory
 * and several entries responsible for tikle stuff
 *
 */
static int __init f_init(void)
{
	int error = 0;

	/**
	 * alloc data
	 */
	sysfs = kmalloc(sizeof(procfs_t), GFP_KERNEL);

	/**
	 * create main directory
	 */
	sysfs->proc_dir = proc_mkdir("tikle", init_net.proc_net);

	if (!sysfs->proc_dir) {
		error = -ENOMEM;
		goto exit;
	}

/*
 * after kernel 2.6.30 release, the proc_dir_entry
 * structure has no member named 'owner' anymore
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	sysfs->proc_dir->owner = THIS_MODULE;
#endif

	/**
	 * create /proc/net/tikle entries and its callback functions
	 */
	sysfs->proc_shell = create_proc_entry("shell", S_IWUSR, sysfs->proc_dir);

	if (!sysfs->proc_shell) {
		error = -ENOMEM;
		goto undo_shell;
	}

	sysfs->proc_shell->write_proc = f_write_shell;

/*
 * after kernel 2.6.30 release, the proc_dir_entry
 * structure has no member named 'owner' anymore
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	sysfs->proc_shell->owner = THIS_MODULE;
#endif

	sysfs->proc_status = create_proc_read_entry("status", 
						S_IRUSR, sysfs->proc_dir,
						f_read_status,
						NULL);
	if (!sysfs->proc_status) {
		error  = -ENOMEM;
		goto undo_status;
	}

/*
 * after kernel 2.6.30 release, the proc_dir_entry
 * structure has no member named 'owner' anymore
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	sysfs->proc_status->owner = THIS_MODULE;
#endif

	/**
	 * all gone well
	 */
	printk(KERN_INFO "module initialised\n");

	/**
	 * create the main thread responsible for
	 * the three phases of execution from tikle
	 * configuration -> operation -> data collecting
	 */
//	main_thread = kthread_run((void *)f_config, NULL, "main-thread");

//	if (IS_ERR(main_thread)) {
//		printk(KERN_ERR "error while running kernel thread\n");
//		error = -ENOMEM;
//		goto exit;
///	}

	return 0;

undo_shell:
	remove_proc_entry("tikle", init_net.proc_net);
undo_status:
	remove_proc_entry("shell", sysfs->proc_dir);
exit:
	return error;
}

/**
 * Module remove routines.
 *
 * This function unregister /proc/net/tikle
 * directory and its several entries
 *
 */
static void __exit f_exit(void)
{
	int error = 0;

	/**
	 * stop main kernel thread
	 */
//	error = kthread_stop(main_thread);

	/**
	 * something went wrong, force kill of thread
	 */
/*	if (error == -EINTR) {
		struct pid *temp = get_task_pid(main_thread, PIDTYPE_PID);
		lock_kernel();
		error = kill_pid(temp, SIGKILL, 1);
		unlock_kernel();

		if (error < 0)
			printk(KERN_ERR "error %d while killing kernel thread\n", -error);
		else
			while (!main_thread->state)
				msleep(10);
	}
*/
	/**
	 * unregister procfs main dir and its entries
	 */
	remove_proc_entry("shell", sysfs->proc_dir);
	remove_proc_entry("status", sysfs->proc_dir);
	remove_proc_entry("tikle", init_net.proc_net);
	kfree(sysfs);

	/**
	 * ok, it's time to get back at home
	 */
	printk(KERN_INFO "module removed\n");
}

module_init(f_init);
module_exit(f_exit);

MODULE_AUTHOR("c2zlabs");
//MODULE_VERSION("0.1a");
MODULE_DESCRIPTION("fault injection environment for network protocol evaluation");
MODULE_LICENSE("GPLv3");
