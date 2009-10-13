/* 
 * Tikle kernel module
 * Copyright (C) 2009  Felipe 'ecl' Pena
 *                     Gustavo 'nst' Oliveira
 * 
 * Contact us at: #c2zlabs@freenode.net - www.c2zlabs.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include <linux/init.h> /* module init macros */

#include <linux/netfilter.h> /* kernel protocol stack */
#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS an others */
#include <linux/in.h> /* sockaddr_in and other macros */
#include <linux/ip.h> /* ipip_hdr(const struct sk_buff *sb) */

#include <linux/proc_fs.h> /* procfs manipulation */
#include <net/net_namespace.h> /* proc_net stuff */
#include <linux/sched.h> /* current-euid() */

#include <asm/uaccess.h> /* kernel-mode to user-mode and reverse */
#include <linux/kthread.h> /* kernel threads */
#include <linux/timer.h> /* temporization trigger stuff */

#include <linux/file.h> /* write to file */

//#include <linux/random.h> /* tausworthe generators */

#include "tikle_defs.h" /* tikle macros */

/*
 * path to netiflter constants
 * needed since kernel 2.6.27
 */

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>

/**
 * New reference to proc_net
 */
#define proc_net init_net.proc_net

/**
 * Buffer control to /proc/net/tikle/shell
 */
static char tikle_buffer[TIKLE_PROCFSBUF_SIZE];


/**
 * Struct to register hook operations
 */
static struct nf_hook_ops tikle_pre_hook, tikle_post_hook;
/*
static struct nf_queue_handler nf_q_h = {
	.outfn = &tikle_delay_flow,
	.name = "queue handler",
	.data = (void *) NULL,
};

static struct delay_data {
	struct sk_buff *sb;
	struct nf_info *foo;
	struct timer_list timer_queue;
};
*/
/**
 * Struct to store log data
 */
static tikle_log tikle_logging;

static int num_ips;
static unsigned long *tikle_log_counters = NULL;

static void tikle_send_log(void);

/**
 * tausworthe seeds
 */
struct tikle_seeds {
	u32 s1, s2, s3;
};

static struct tikle_seeds tikle_seed;

/**
 * Opcode names
 */
static const char *op_names[] = { 
	"COMMAND", "AFTER", "WHILE", "HOST", "IF", "ELSE",
	"END_IF", "END", "SET", "FOREACH", "PARTITION", "DECLARE"
};

/**
 * Struct declaration for handling timers
 */
struct tikle_timer {
	struct timer_list trigger;
	/*
	 * The key on faultload[]
	 */
	unsigned int trigger_id;
	/*
	 * The trigger state. 0 for inative,
	 * 1 for active, 2 for killed.
	 */
	unsigned int trigger_state;
};

static struct tikle_timer *tikle_timers;


/**
 * Control flags
 */
static unsigned int tikle_num_timers = 0, tikle_trigger_flag = 0;


/**
 * Faultload
 */
static faultload_op faultload[30];

static unsigned int tikle_pre_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *));

static unsigned int tikle_post_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *));

/**
 * The size of the data
 * hold in the buffer
 */
static unsigned long tikle_buffer_size = 0;


/**
 * procfs structures.
 */
static struct proc_dir_entry *tikle_main_proc_dir,    /* /proc/net/tikle       */
			     *tikle_shell_proc_entry; /* /proc/net/tikle/shell */


/**
 * socket structure
 */
struct tikle_sockudp {
	int flag;
	struct task_struct *thread;
	struct socket *sock_recv, *sock_send, *sock_log;
	struct sockaddr_in addr_recv, addr_send, addr_log;
};

struct tikle_sockudp *tikle_comm = NULL;


/**
 * @brief Called when a /proc/net/tikle file is "read".
 *
 * @param file the file structure
 * @param buffer the buffer to fill with data
 * @param length the lenght of the buffer
 * @param offset 
 * @return number of bytes read
 */
static ssize_t tikle_read(struct file *file,
			     char *buffer,
			     size_t length,
			     loff_t * offset)
{
	static int tikle_eof = 0;

	/* 
	 * return
	 * 0 -> end of file
	 * 1 -> endless loop 
	 */

	if (tikle_eof) {
		printk(KERN_INFO "tikle alert: end of file\n");
		tikle_eof = 0;
		return 0;
	}

	tikle_eof = 1;

	/*
	 * copy_to_user   -> copy the string from the kernel's
	 * memory segment to the memory segment of the process
	 * that called this function.
	 * copy_from_user -> do the reverse
	 */

	if (copy_to_user(buffer, tikle_buffer, tikle_buffer_size)) {
		return -EFAULT;
	}

	printk(KERN_INFO "tikle alert: read %lu bytes\n", tikle_buffer_size);

	return tikle_buffer_size;
}


/**
 * Called when a /proc/net/tikle file is written.
 *
 * @param file the file structure
 * @param buffer the buffer to write
 * @param len the number of bytes to be written
 * @param off
 * @return the number of bytes written.
 */
static ssize_t
tikle_write(struct file *file,
		const char *buffer,
		size_t len,
		loff_t * off)
{
	char command[len];

	if (len > TIKLE_PROCFSBUF_SIZE) {
		tikle_buffer_size = TIKLE_PROCFSBUF_SIZE;
	} else {
		tikle_buffer_size = len;
	}
	
	if (copy_from_user(tikle_buffer, buffer, tikle_buffer_size)) {
		return -EFAULT;
	}

	strncpy(command, buffer, len-1);
	command[len-1] = 0;

	printk(KERN_INFO "tikle alert: write %lu bytes\n", tikle_buffer_size);

	/*
	 * interpretation of commands from user-space
	 * by echo "command" > /proc/net/tikle/shell
	 */

	if (strcmp(command, "halt") == 0) {
		printk(KERN_INFO "tikle alert: halting tests\n");
	} else {
		printk(KERN_INFO "tikle alert: Unknown command\n");
	}

	return tikle_buffer_size;
}


/**
 * Check file permissions.
 *
 * @param inode the inode to check
 * @param op the operation
 * @return 0 on success EACCESS on error.
 */
static int tikle_permission(struct inode *inode,
				int op
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26)
				, struct nameidata *idata
#endif
)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28)
# define current_euid() current->euid
#endif

	printk(KERN_INFO "tikle alert:\n\toperation: %d\n\tpermission: %d\n", op, current_euid());

	/* 
	 * everybody can READ (4, 36),
	 * but ONLY root may write (2, 34)
	 * don't ask me why 34 and 36 too
	 */

	if (op == 36 || op == 4 || ((op == 34 || op == 2) && current_euid() == 0)) {
		return 0;
	}

	/* 
	 * else permission denied 
	 */

	return -EACCES;
}


/**
 * Open the file
 */
static int tikle_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);
	return 0;
}


/**
 * Close the file 
 */
static int tikle_close(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}


/**
 * File operation handlers.
 */
static struct file_operations tikle_fops = {
	.read 	 = tikle_read,
	.write 	 = tikle_write,
	.open 	 = tikle_open,
	.release = tikle_close
};


static struct inode_operations tikle_iops = {
	.permission = tikle_permission
};


/*static int tikle_delay_flow(struct sk_buff *pskb, struct nf_info *info, unsigned int queuenum, void *data)
{
	struct timer_list timer;
	struct delay_data delay;

	delay.sb = pskb;
	delay.info = info;
	delay.timer_queued = timer;
*/
	/*
	 * creating timer to emulate delay behavior
	 */
/*
	init_timer(&timer_queue);
	timer_queue.data = (unsigned long) delay;
	timer_queue.function = (void *) delay_unlock;
	timer_queue.expires = jiffies + faultload[tikle_trigger_flag].op_value[0].num * HZ / 1000;
	//timer_queue->base = &boot_tvec_bases;
	add_timer(&timer_queue);
	
	return 0;
}
*/
/* Reinject a packet in the protocol stack after the delay */
/*static void delay_unlock(struct delay_data delay)
{
	nf_reinject(delay.skb, delay.info, NF_ACCEPT);
}
*/


/**
 * Function to write the final log to /tmp/tikle.log file.
 * The central host gets all logging data from other hosts
 */
static int tikle_write_to_file(char *log)
{
	/* unsigned char message[100];
	mm_segment_t old_fs;
	struct file *filp;
	int writelen, ret = 0;
	char *logbf;

	logbf = kmalloc(100 * sizeof(char), GFP_KERNEL);

	sprintf(logbf, log);

	if ((filp = filp_open("/tmp/tikle.log", O_CREAT | O_WRONLY | O_APPEND, 0666)) != NULL) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		writelen = filp->f_op->write(
					     filp,
					     logbf,
					     strlen(logbf),
					     &filp->f_pos);
		set_fs(old_fs);
		fput(filp);
	}
	
		ret = filp_close(filp, NULL);
*/
	return 0;
}


/**
 * Update flags to maintain a timer execution control.
 *
 * @param id active trigger id
 * @return void
 */
static void tikle_flag_handling(unsigned long id)
{
	char *log;

	log = kmalloc(100 * sizeof(char), GFP_KERNEL);

	/*
	 * set the previous timer as 'killed'
	 * and update the system flags with the
	 * running trigger informations
	 */

	if (id > 0) {
		tikle_timers[tikle_trigger_flag].trigger_state = 2;
		printk(KERN_INFO "tikle alert: killing timer %d\n", tikle_trigger_flag);

		/*
		 * Timer stats
		 */
		printk(KERN_INFO "tikle log: total/accepted/rejected packets on event: %d/%d/%d\n",
				tikle_logging.total_packets, tikle_logging.accept_packets, tikle_logging.reject_packets);
		/*
		 * Go to next timer
		 */
		tikle_trigger_flag++;	
	}
	
	printk(KERN_INFO "tikle alert: activating timer %d\n", tikle_trigger_flag);

	/* 
	 * Set state to active
	 */

	tikle_timers[tikle_trigger_flag].trigger_state = 1;

	sprintf(log, "timestamp: %ld @ event: %s",
			jiffies - tikle_logging.start_time, op_names[faultload[id].opcode]);

	printk(KERN_INFO "tikle alert: %s\n", log);

	tikle_write_to_file(log);

	printk(KERN_INFO "tikle alert: event on %u ms --- %u s\n",
			jiffies_to_msecs(jiffies - tikle_logging.start_time), jiffies_to_msecs(jiffies - tikle_logging.start_time)/1000);
			
	printk(KERN_INFO "tikle log: event %s identified after %u ms --- %u s\n",
			op_names[faultload[id].opcode], jiffies_to_msecs(jiffies - tikle_logging.start_time),
			jiffies_to_msecs(jiffies - tikle_logging.start_time)/1000);
			
	kfree(log);

	/*
	 * reset counters for the right next event
	 */

	tikle_logging.total_packets = 0;
	tikle_logging.accept_packets = 0;
	tikle_logging.reject_packets = 0;
}


/**
 * Stop the trigger.
 *
 * @param base the actual jiffies.
 * @return void
 */
static void tikle_stop_trigger(unsigned int base)
{
	char *log;

	log = kmalloc(100 * sizeof(char), GFP_KERNEL);

	lock_kernel();
	current->flags |= PF_NOFREEZE;
	daemonize("thread stop trigger");
	allow_signal(SIGKILL);
	unlock_kernel();

	/*
	 * do you believe in micracles?
	 * not a method, not a technique,
	 * just cheating a kernel timer ;]
	 *
	 * nf_[un]register_hook() does not
	 * execute friendly with timers
	 * since its atomic implementation
	 * 
	 * for more information go to:
	 * http://www.makelinux.net/ldd3/chp-7-sect-4.shtml
	 */

	for (; jiffies <= (base + msecs_to_jiffies(faultload[tikle_timers[tikle_num_timers-1].trigger_id].op_value[0].num * 1000)););

	nf_unregister_hook(&tikle_pre_hook);
	nf_unregister_hook(&tikle_post_hook);

	//nf_unregister_queue_handler(PF_INET, &qh);

	sprintf(log, "timestamp: %ld @ execution ended", jiffies - tikle_logging.start_time);

	tikle_write_to_file(log);

	printk(KERN_INFO "tikle alert: hook unregistered at %u ms --- %u s\n",
			jiffies_to_msecs(jiffies - tikle_logging.start_time), jiffies_to_msecs(jiffies - tikle_logging.start_time)/1000);

	printk(KERN_INFO "tikle log: execution ended after %u ms --- %u s\n",
			jiffies_to_msecs(jiffies - tikle_logging.start_time), jiffies_to_msecs(jiffies - tikle_logging.start_time)/1000);

	tikle_send_log();

}


/**
 * Setup trigger handling
 * @return void
 */
static void tikle_trigger_handling(void)
{
	unsigned int i = 0, base;
	struct task_struct *stop;

	/*
	 * setting up triggers
 	 */

	for (i = 0; i < tikle_num_timers; i++) {

		init_timer(&tikle_timers[i].trigger);

		tikle_timers[i].trigger.function = tikle_flag_handling;

		/*
		 * send the next trigger ID, if actual trigger it is
		 * not the latest, else send your own trigger ID
		 */

		if (tikle_timers[i].trigger_id < tikle_timers[tikle_num_timers-1].trigger_id) {
			tikle_timers[i].trigger.data = tikle_timers[i+1].trigger_id;
		} else {
			tikle_timers[i].trigger.data = tikle_timers[i].trigger_id;
		}

		/*
		 * calculate expire time to trigger
		 * based on the experiment start time
		 */
		
		base = jiffies;
		tikle_timers[i].trigger.expires = base +
			msecs_to_jiffies(faultload[tikle_timers[i].trigger_id].op_value[0].num * 1000);

		if (tikle_timers[i].trigger_id == 0) { /* if timer corresponds to the first timer */

			/*
			 * register hook
			 */

			tikle_pre_hook.hook = &tikle_pre_hook_function;
			tikle_pre_hook.hooknum = NF_IP_PRE_ROUTING;
			tikle_pre_hook.pf = PF_INET;
			tikle_pre_hook.priority = NF_IP_PRI_FIRST;

			nf_register_hook(&tikle_pre_hook);

			tikle_post_hook.hook = &tikle_post_hook_function;
			tikle_post_hook.hooknum = NF_IP_POST_ROUTING;
			tikle_post_hook.pf = PF_INET;
			tikle_post_hook.priority = NF_IP_PRI_FIRST;

			nf_register_hook(&tikle_post_hook);

			//nf_register_queue_handler(PF_INET, &qh);

			printk(KERN_INFO "tikle alert: hook registered\n");

			/*
			 * if timer corresponds
			 * to the last timer
			 */

		} else if (tikle_timers[i].trigger_id == tikle_timers[tikle_num_timers-1].trigger_id) {
			
			/*
			 * unregister hook timer
			 * this is done by a thread
			 * to avoid console freezing
			 */

			stop = kthread_run((void *)tikle_stop_trigger, (unsigned int *)base, "thread stop trigger");
		}

		/*
		 * WHILE primitives removed
		 * until makes itself necessary
		 */

		/*
		 * if the timer corresponds to a 'while' timer, it
		 * will start on the same time of your after parent
		 */

		/*
		} else {
		*/
			/*
			 * each 'after' timer is a trigger to your
			 * respectivelly 'while' timer. so, 'while'
			 * will start when your parent 'after' ends
			 */
		/*
			tikle_timers[i].trigger.expires = tikle_timers[i-1].trigger.expires +
									msecs_to_jiffies(faultload[tikle_timers[i].trigger_id].op_value[0].num * 1000);
		}
		*/
		add_timer(&tikle_timers[i].trigger);
	}
}


/**
 * Begin script interpretation.
 *
 * @param hooknum the hook number
 * @param pskb
 * @param indev
 * @param outdev
 * @param okfn
 * @return 
 */
static unsigned int tikle_pre_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sb = pskb /*, *duplicate_sb*/;
	unsigned long array_local = 0, array_remote = 1;
	int i = 0, array, position, log_found_flag = -1;
	char *log;

	/*
	 * log counters dummy version
	 */

	for (; i < num_ips && tikle_log_counters[i * 3]; i++) {
		if (tikle_log_counters[i * 3] == ipip_hdr(sb)->saddr) {
			log_found_flag = i;
			break;
		}
	}
		
	if (log_found_flag < 0) {
		tikle_log_counters[i * 3] = ipip_hdr(sb)->saddr;
	}

	/*
	 * log system syntax (in order):
	 * timestamp - in jiffies
	 * sender/receiver - ipv4 format
	 * message protocol - integer
	 *
	 * ps: 100 is a 'sorted' value of my limited brain
	 */

	log = kmalloc(1000 * sizeof(char), GFP_KERNEL | GFP_ATOMIC);
 
	sprintf(log, "INCOMING timestamp: %ld @ remetente: " NIPQUAD_FMT " destinatario: " NIPQUAD_FMT " @ protocolo: %d",
			jiffies - tikle_logging.start_time,
			NIPQUAD(ipip_hdr(sb)->saddr),
			NIPQUAD(ipip_hdr(sb)->daddr),
			ipip_hdr(sb)->protocol);


	printk(KERN_INFO "tikle log: %s\n", log);

	tikle_write_to_file(log);

	printk(KERN_INFO "tikle alert: protocol %d\n", ipip_hdr(sb)->protocol);

	printk(KERN_INFO "tikle alert: hook function called\n");
	
	kfree(log);

	/*
	 * update log informations
	 */

	tikle_logging.total_packets++;

	tikle_log_counters[i * 3 + 1]++;

	printk(KERN_INFO "tikle counters: %lu\n\n\n", tikle_log_counters[i * 3 + 1]);

	/*
	 * packets will be intercepted only if a timer is active. if
	 * it is, process packets according to the flagged trigger
	 */

	if (tikle_timers[tikle_trigger_flag].trigger_state != 1) {
		tikle_logging.accept_packets++;	
		return NF_ACCEPT;
	}
	
	/* Current AFTER */
	i = tikle_timers[tikle_trigger_flag].trigger_id;
	
	do {
		/* Opcode handlers */
		switch (faultload[i].opcode) {
			case HOST:
			case END:
			case END_IF:
			case DECLARE:
				/* Never handled here */
				break;
			case ELSE:
				/* The if-block reached the ELSE,
						 * then skip to out of end if
				 */
				/* i = faultload[i].op_num; */
				break;
			case COMMAND:
				/*
				 * e.g.
				 * tcp drop progressive 10%;
				 * 
				 * tcp: faultload[i].protocol = TCP_PROTOCOL
				 * drop: faultload[i].op_value[0].num
				 * Possible values:
				 * 	ACT_DROP (1)
				 *	ACT_DUPLICATE (2)
				 * 	ACT_DELAY (3)
				 * 
				 * progressive: faultload[i].extended_value
				 * 10: faultload[i].op_value[1].num
				 */
				if (faultload[i].protocol == ipip_hdr(sb)->protocol) {
					/*
					switch (faultload[i].op_value[0].num) {
						case ACT_DELAY:
							NF_QUEUE;
						case ACT_DROP:
							return NF_DROP;
						case ACT_DUPLICATE:
							duplicate_sb = skb_copy(pskb, GFP_ATOMIC);
							if (duplicate_sb) {
								okfn(duplicate_sb);
							}
							return NF_ACCEPT;
					}
					*/
				}
				break;
			case AFTER:
				/*
				 * e.g.
				 * after (10p) do
				 *   ...
				 * end
				 * 
				 * 10: faultload[i].op_value[0]
				 *  p: faultload[i].occur_type
				 * Possible values:
				 *   TEMPORAL
				 *   NPACKETS
				 *   PERCET
				 *   NONE (only the number was specified)
				 */
				break;
			case WHILE:
				/*
				 * e.g.
				 * while (10s) do
				 *   ...
				 * end
				 * 
				 * 10: faultload[i].op_value[0]
				 *  s: faultload[i].occur_type
				 * Possible values:
				 *   TEMPORAL
				 *   NPACKETS
				 *   PERCET
				 *   NONE (only the number was specified)
				 */
				break;
			case IF:
				/*
				 * e.g.
				 * if (%ip is equal 127.0.0.1) then
				 *   ...
				 * end if
				 * 
				 * %ip: faultload[i].op_value[0]
				 *   In this case: op_type[0] == VAR
				 * 
				 * 'is equal': faultload[i].extended_value
				 * 
				 * Possible operators:
				 * 		'is equal'     = 1
				 * 		'is not equal' = 2
				 * 
				 * 127.0.0.1: faultload[i].op_value[1] (inet_addr)
				 *            faultload[i].op_type[1] == NUMBER
				 */
				/*
				 * if (!(skip = tikle_handler_if(faultload[i]))) {
				 * 		i = skip;
				 * } 
				 */
				break;
			case SET:
				/* 
				 * e.g.
				 * set 127.0.0.1 -> FLAG
				 * 
				 * 127.0.0.1: faultload[i].op_value[0] (inet_addr)
				 *            faultload[i].op_type[0] == NUMBER
				 * 
				 * FLAG: faultload[i].op_value[1]
				 *       faultload[i].op_type[1] == STRING
				 */
				break;
			case FOREACH:
				/* 
				 * e.g.
				 * for each do
				 *   tcp: drop 20%;
				 * end
				 */
				break;
			case PARTITION:
				/* 
				 * e.g.
				 * PARTITION BETWEEN A AND B 
				 * 
				 * A: faultload[i].op_value[0]
				 *    faultload[i].op_type[0] == STRING
				 * 
				 * B: faultload[i].op_value[1]
				 *    faultload[i].op_type[0] == STRING
				 */

				/*
				 * loop into partitions to identify allowed communications 
				 */

				for (array = 0; array < faultload[i].num_ops; array++) {
					for (position = 0; position < faultload[i].op_value[array].array.count; position++) {
			
						/* 
						 * since kernel 2.6.22 the API to retrieve data from struct sk_buff *pskb
						 * by typing pskb->nh.iph->saddr has been updated and now it's done by
						 * ipip_hdr(const struct sk_buff *pskb)->foo defined in "linux/ip.h"
						 */
					
						printk(KERN_INFO "tikle alert: de: " NIPQUAD_FMT " para:" NIPQUAD_FMT "\n", NIPQUAD(ipip_hdr(sb)->saddr), NIPQUAD(ipip_hdr(sb)->daddr));

						/*
						 * source address equals to destination address?
						 */

						if (faultload[i].op_value[array].array.nums[position] == ipip_hdr(sb)->saddr) {
							array_remote = array;
						} else if (faultload[i].op_value[array].array.nums[position] == ipip_hdr(sb)->daddr) {
							array_local = array;
						}
					}
				}

				if (array_local == array_remote) {

					/*
					 * if, and only if, the source address of the packet is included
					 * is included in the same partition as destination address, the
					 * packet must be accepted. Otherwise, go to 'else' and drop it
					 */

					tikle_logging.accept_packets++;

					printk(KERN_INFO "ACCEPT %d\n", tikle_trigger_flag);
					return NF_ACCEPT;
				} else {
					tikle_logging.reject_packets++;

					printk(KERN_INFO "DROP %d\n", tikle_trigger_flag);
					return NF_DROP;
				}
				break;
		}
	} while (++i < faultload[tikle_trigger_flag].next_op);

	tikle_logging.accept_packets++;

	printk(KERN_INFO "LIMPO %d\n", tikle_trigger_flag);

	return NF_ACCEPT;
}

static unsigned int tikle_post_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sb = pskb;
	char *log;
	int i = 0, log_found_flag = -1;

	/*
	 * log counters dummy version
	 */

	for (; i < num_ips && tikle_log_counters[i * 3]; i++) {
		if (tikle_log_counters[i * 3] == ipip_hdr(sb)->daddr) {
			log_found_flag = i;
			break;
		}
	}
		
	if (log_found_flag < 0) {
		tikle_log_counters[i * 3] = ipip_hdr(sb)->daddr;
	}

	/*
	 * updating log informations
	 */

	tikle_log_counters[i * 3 + 2]++;

	printk(KERN_INFO "tikle counters: %lu\n\n\n", tikle_log_counters[i * 3 + 2]);

	log = kmalloc(100 * sizeof(char), GFP_KERNEL);

	sprintf(log, "OUTGOING timestamp: %ld @ remetente: " NIPQUAD_FMT " destinatario: " NIPQUAD_FMT " @ protocolo: %d",
			jiffies - tikle_logging.start_time,
			NIPQUAD(ipip_hdr(sb)->saddr),
			NIPQUAD(ipip_hdr(sb)->daddr),
			ipip_hdr(sb)->protocol);

	printk(KERN_INFO "tikle log: %s\n", log);
	
	kfree(log);

	return NF_ACCEPT;
}


/**
 * Receive data from socket.
 * 
 * @param sock the socket
 * @param addr the host receiving data
 * @param buf the buff to write in
 * @param len the lenght of the buffer
 * @return the number of bytes read.
 * @sa tikle_sockdup_send()
 */
static int tikle_sockudp_recv(struct socket* sock,
		struct sockaddr_in* addr,
		void *buf,
		int len)
{
	int size = 0;
	struct iovec iov;
	struct msghdr msg;
	mm_segment_t oldfs;

	if (sock->sk == NULL) {
		return 0;
	}

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_recvmsg(sock, &msg, len, msg. msg_flags);
	set_fs(oldfs);

	return size;
}


/**
 * Send data to socket
 *
 * @param sock the socket to write in
 * @param addr the host sending data.
 * @return the number of bytes sent.
 * @sa tikle_sockdup_recv()
 */
static int tikle_sockudp_send(struct socket *sock,
		struct sockaddr_in *addr,
		void /*unsigned char*/ *buf, int len)
{
	int size = 0;
	struct iovec iov;
	struct msghdr msg;
	mm_segment_t oldfs;

	if (sock->sk == NULL) {
		return 0;
	}

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(sock, &msg, len);
	set_fs(oldfs);

	return size;
}


/**
 * Setup the server to receive faultload.
 *
 * @return 1 all the time
 */
static int tikle_sockudp_start(void)
{
	int size = -1, i = 0, tikle_err, trigger_count = 0;
	static int count = 0;
	char tikle_auth[15];

	/*
	 * define flags of the thread and run socket
	 * server as a daemon (run in background)
	 */

	lock_kernel();
	tikle_comm->flag = 1;
	current->flags |= PF_NOFREEZE;
	daemonize("thread_sock");
	allow_signal(SIGKILL);
	unlock_kernel();

	/*
	 * socket creation
	 * listening on port 12608
	 * connecting on port 21508
	 */

	if (((tikle_err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &tikle_comm->sock_recv)) < 0) 
		|| ((tikle_err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &tikle_comm->sock_send)) < 0)) {
		printk(KERN_ERR "tikle alert: error %d while creating sockudp\n", -ENXIO);
		tikle_comm->thread = NULL;
		tikle_comm->flag = 0;
	}

	memset(&tikle_comm->addr_recv, 0, sizeof(struct sockaddr)); 
	tikle_comm->addr_recv.sin_family = AF_INET;
	tikle_comm->addr_recv.sin_addr.s_addr = htonl(INADDR_ANY);
	tikle_comm->addr_recv.sin_port = htons(PORT_LISTEN);

	memset(&tikle_comm->addr_send, 0, sizeof(struct sockaddr));
	tikle_comm->addr_send.sin_family = AF_INET;
	tikle_comm->addr_send.sin_addr.s_addr = htonl(INADDR_ANY);
	tikle_comm->addr_send.sin_port = htons(PORT_CONNECT);

	if (((tikle_err = tikle_comm->sock_recv->ops->bind(tikle_comm->sock_recv,
			(struct sockaddr *)&tikle_comm->addr_recv, sizeof(struct sockaddr))) < 0)
		|| ((tikle_err = tikle_comm->sock_send->ops->connect(tikle_comm->sock_send,
			(struct sockaddr *)&tikle_comm->addr_send, sizeof(struct sockaddr), 0)) < 0)) {
		printk(KERN_ERR "tikle alert: error %d while binding/connecting to socket\n", -tikle_err);
		sock_release(tikle_comm->sock_recv);
		sock_release(tikle_comm->sock_send);
		tikle_comm->sock_recv = NULL; 
		tikle_comm->sock_send = NULL;
	}

	printk(KERN_INFO "tikle alert: listening on port %d\n", PORT_LISTEN);

	/*
	 * looping to receive data from other hosts
	 * and after that sends a confirmation message
	 */

	printk(KERN_INFO "- start------------------------------------\n");
	memset(faultload, 0, sizeof(faultload));
	
	size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
		&num_ips, sizeof(int));
		
	if (size < 0) {
		printk(KERN_ERR "tikle alert: error %d while getting datagram\n", size);
	} else {				
		printk(KERN_INFO "tikle alert: received %d bytes\n", size);
	}

	while (1) {
		switch (count) {
			case 0: /* Opcode */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].opcode, sizeof(faultload_opcode));
				if (faultload[i].opcode == AFTER) {
					trigger_count++;
				}
				printk(KERN_INFO "opcode: %d\n", faultload[i].opcode);
				break;
			case 1: /* Protocol */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].protocol, sizeof(faultload_protocol));
				printk(KERN_INFO "protocol: %d\n", faultload[i].protocol);
				break;
			case 2: /* Occur type (TEMPORAL, PACKETS, PERCT) */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].occur_type, sizeof(faultload_num_type));
				printk(KERN_INFO "occur_type: %d\n", faultload[i].occur_type);
				break;
			case 3: /* Number of ops */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].num_ops, sizeof(unsigned short int));
				printk(KERN_INFO "num_ops [%d]\n", faultload[i].num_ops);
				break;
			case 4: /* Operand types */
				{
					unsigned short int k;
					
					if (faultload[i].num_ops == 0) {
						faultload[i].op_type = NULL;
						faultload[i].op_value = NULL;
						
						count += 2; /* Jump to case 6 */
						continue;
					}
					
					faultload[i].op_type = kcalloc(faultload[i].num_ops, sizeof(faultload_op_type), GFP_KERNEL);

					size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
						faultload[i].op_type, sizeof(faultload_op_type) * faultload[i].num_ops);

					for (k = 0; k < faultload[i].num_ops; k++) {
						printk(KERN_INFO "op_type%d [%d]\n", k+1, faultload[i].op_type[k]);
					}
				}
				break;
			case 5: /* Operand values */
				{
					unsigned short int k;
						
					faultload[i].op_value = kcalloc(faultload[i].num_ops, sizeof(faultload_value_type), GFP_KERNEL);
					size = 0;
					for (k = 0; k < faultload[i].num_ops; k++) {
						switch (faultload[i].op_type[k]) {
							case STRING:
								tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
									&faultload[i].op_value[k].str.length, sizeof(size_t));
								faultload[i].op_value[k].str.value = (char*) kmalloc(faultload[i].op_value[k].str.length, GFP_KERNEL);
								size += tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
								faultload[i].op_value[k].str.value, faultload[i].op_value[k].str.length);
								
								printk(KERN_INFO "%d string [%s] (len: %d)\n", k+1, faultload[i].op_value[k].str.value,
									faultload[i].op_value[k].str.length);
								break;
							case ARRAY:
								{
									unsigned int j;
									
									tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
										&faultload[i].op_value[k].array.count, sizeof(size_t));
									size += tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
										&faultload[i].op_value[k].array.nums, sizeof(unsigned long) * faultload[i].op_value[k].array.count);
									
									printk(KERN_INFO "%d array {", k+1);
									for (j = 0; j < faultload[i].op_value[k].array.count; j++) {
										printk("%ld%s", faultload[i].op_value[k].array.nums[j],
											(j == (faultload[i].op_value[k].array.count-1) ? "" : ","));
									}
									printk("}\n");
								}
								break;
							default:
								size += tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
									&faultload[i].op_value[k].num, sizeof(unsigned long));
									
								printk(KERN_INFO "%d number: %ld\n", k+1, faultload[i].op_value[k].num);
								break;
						}
					}
				}
				break;
			case 6: /* Extended value */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
							&faultload[i].extended_value, sizeof(int));
				printk(KERN_INFO "Extended value: %d\n", faultload[i].extended_value);
				break;
			case 7: /* Label */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
							&faultload[i].label, sizeof(unsigned long));
				printk(KERN_INFO "Label: %ld\n", faultload[i].label);
				break;
			case 8: /* Block type (START, STOP) */
				size = tikle_sockudp_recv(tikle_comm->sock_recv,
							&tikle_comm->addr_recv,	&faultload[i].block_type,
							sizeof(short int));
				printk(KERN_INFO "Block type: %d\n", faultload[i].block_type);
				break;
			case 9: /* Control op number */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
							&faultload[i].next_op, sizeof(unsigned int));
				printk(KERN_INFO "Op number: %d\n", faultload[i].next_op);
				break;
			default:
				printk(KERN_INFO "tikle alert: unexpected type\n");
				break;
		}
		
		if (signal_pending(current)) {
			printk(KERN_INFO "tikle alert: nothing\n");
			goto next;
		}
		
		if (size < 0) {
			printk(KERN_ERR "tikle alert: error %d while getting datagram\n", size);
		} else {				
			printk(KERN_INFO "tikle alert: received %d bytes\n", size);
		}
next:
		if ((count+1) == NUM_FAULTLOAD_OP) {
			if (faultload[i].block_type == 1) {
				/* STOP! The last instruction was reached. */
				break;
			}
			count = -1;
			i++;
		}
		count++;
	}
	printk(KERN_INFO "- end------------------------------------\n");

	/*
	 * sending confirmation of data received
	 * (for control protocol stuffs)
	 */

	printk(KERN_INFO "tikle alert: sending confirmation of received data\n");

	tikle_sockudp_send(tikle_comm->sock_send, &tikle_comm->addr_send,
		"tikle-received", sizeof("tikle-received"));

	/*
	 * preparing log counters
	 */
	if (num_ips) {
		tikle_log_counters = kcalloc(num_ips * 3, sizeof(unsigned long), GFP_KERNEL);
	}

	/*
	 * waiting for controller
	 * authorization to start
	 */

	printk(KERN_ERR "tikle alert: waiting for authorization to start\n");

	size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv, tikle_auth, sizeof(tikle_auth));
	
	printk(KERN_ERR "tikle alert: received \"%s\" from controller\n", tikle_auth);

	if (size < 0) {
		printk(KERN_ERR "tikle alert: error %d while getting authorization\n", size);
		return 0;
	} else if (strcmp(tikle_auth, "tikle-start") == 0) { 
		printk(KERN_ERR "tikle alert: received permission to start execution\n");

		/*
		 * load faultload in memory
		 */
	
		tikle_timers = kmalloc(trigger_count * sizeof(struct tikle_timer), GFP_KERNEL | GFP_ATOMIC);

		i = 0;

		do {
			switch (faultload[i].opcode) {
				case AFTER:
				/* case WHILE: temporarily disable until WHILE makes itself necessary */
					tikle_timers[tikle_num_timers].trigger_id = i;
					tikle_timers[tikle_num_timers++].trigger_state = 0;
					printk(KERN_INFO "tikle alert: triggers on %lds\n", faultload[i].op_value[0].num);

					i = faultload[i].next_op - 1;
					break;
				default:
					break;
			}
		} while (faultload[i++].block_type == 0);

		/*
		 * calling trigger handling
		 * to manage temporization
	 	 */

		printk(KERN_INFO "tikle alert: starting at %u ms --- %u s\n",
				jiffies_to_msecs(jiffies), jiffies_to_msecs(jiffies)/1000);

		tikle_logging.start_time = jiffies;

		printk(KERN_INFO "tikle log: INIT at %u ms --- %u s\n",
				jiffies_to_msecs(jiffies - tikle_logging.start_time), jiffies_to_msecs(jiffies - tikle_logging.start_time)/1000);

		tikle_trigger_handling();
	} else {
		printk(KERN_ERR "tikle alert: you do not have permission to start execution\n");
		return 0;
	}
	return 0;
}


/**
 * Tausworthe generator
 */
static void tikle_random(struct tikle_seeds *tikle_seed) 
{
	tikle_seed->s1 = LCG(jiffies);
	tikle_seed->s2 = LCG(tikle_seed->s1);
	tikle_seed->s3 = LCG(tikle_seed->s2);

	tikle_seed->s1 = TAUSWORTHE(tikle_seed->s1, 13, 19, 4294967294UL, 12);
	tikle_seed->s2 = TAUSWORTHE(tikle_seed->s2, 2, 25, 4294967288UL, 4);
	tikle_seed->s3 = TAUSWORTHE(tikle_seed->s3, 3, 11, 4294967280UL, 17);
}


/*
 * after ending experiment, host must
 * send your counters to controller
 */
static void tikle_send_log(void)
{
	int tikle_err;

	if ((tikle_err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &tikle_comm->sock_log)) < 0) {
		printk(KERN_ERR "tikle alert: error %d while creating sockudp\n", -ENXIO);
	}

	memset(&tikle_comm->addr_log, 0, sizeof(struct sockaddr));
	tikle_comm->addr_log.sin_family = AF_INET;
	tikle_comm->addr_log.sin_addr.s_addr = htonl(INADDR_ANY);
	tikle_comm->addr_log.sin_port = htons(PORT_LOGGING);

	if ((tikle_err = tikle_comm->sock_send->ops->connect(tikle_comm->sock_send,(struct sockaddr *)&tikle_comm->addr_send, sizeof(struct sockaddr), 0)) < 0) {
		printk(KERN_ERR "tikle alert: error %d while connecting to socket\n", -tikle_err);
		sock_release(tikle_comm->sock_send);
		tikle_comm->sock_log = NULL; 
	}

	tikle_sockudp_send(tikle_comm->sock_log, &tikle_comm->addr_log, tikle_log_counters, sizeof(unsigned long) * 3 * num_ips);
}


/**
 * Module initialization routines.
 *
 * This function create /proc/net directory entries and
 * start the faultload server.
 *
 * @return 0 on success or ENOMEM on error
 */
static int __init tikle_init(void)
{
	int tikle_err = 0;

	/* 
	 * create main directory
	 */

	tikle_main_proc_dir = proc_mkdir("tikle", proc_net);
	
	if (!tikle_main_proc_dir) {
		remove_proc_entry("tikle", proc_net);
		printk(KERN_ERR "tikle alert: error while creating main directory\n");
		tikle_err = -ENOMEM;
		goto tikle_exit;
	}

	/*
	 *  create a file controller
	 */

	tikle_shell_proc_entry = create_proc_entry("shell",
						   S_IWUSR,
						   tikle_main_proc_dir);

	if (!tikle_shell_proc_entry) {
		tikle_err = -ENOMEM;
		printk(KERN_ERR "tikle alert: error while creating shell file\n");
		goto tikle_no_shell;
	}

	/*
	 * define inode operations
	 * define file operations
	 * 
	 * The 'owner' member has
	 * been changed since 2.6.30
	 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	tikle_shell_proc_entry->owner = THIS_MODULE;
#endif
	tikle_shell_proc_entry->proc_iops = &tikle_iops;
	tikle_shell_proc_entry->proc_fops = &tikle_fops;
	tikle_shell_proc_entry->mode = S_IWUSR;

	/*
	 * preparing thread to
	 * init socket stuff
	 */

	tikle_comm = kmalloc(sizeof(struct tikle_sockudp), GFP_KERNEL);
	memset(tikle_comm, 0, sizeof(struct tikle_sockudp));

	tikle_comm->thread = kthread_run((void *)tikle_sockudp_start, NULL, "tikle");

	if (IS_ERR(tikle_comm->thread)) {
		printk(KERN_ERR "tikle alert: error while running kernel thread\n");
		kfree(tikle_comm);
		tikle_comm = NULL;
		tikle_err = -ENOMEM;
		goto tikle_exit;		
	}

	/*
	 * sorting seeds
	 */
	tikle_random(&tikle_seed);

	printk(KERN_INFO "tikle alert: seeds are 0x%x 0x%x 0x%x\n", tikle_seed.s1, tikle_seed.s2, tikle_seed.s3);

	printk(KERN_INFO "tikle alert: module loaded\n");

	return tikle_err;

tikle_no_shell:
	remove_proc_entry("tikle", proc_net);
tikle_exit:
	return tikle_err;
}


/**
 * Cleanup routines to unload the module.
 *
 * Removes the procfs entries and shutdown the server.
 */
static void __exit tikle_exit(void)
{
	unsigned int i = 0, k = 0;

	/*
	 * cleanup procfs
	 */

	remove_proc_entry("shell", tikle_main_proc_dir);
	remove_proc_entry("tikle", proc_net);

	/*
	 * cleanup memory/socket
	 */

	if (!tikle_comm->sock_recv || tikle_comm->sock_send) {
		sock_release(tikle_comm->sock_recv);
		sock_release(tikle_comm->sock_send);
		tikle_comm->sock_recv = NULL;
		tikle_comm->sock_send = NULL;
	}

	kfree(tikle_comm);
	kfree(tikle_timers);
	tikle_comm = NULL;

	/* Check if the faultlets were loaded */
	if (faultload[0].opcode) {
		do {
			if (faultload[i].op_type) {
				for (k = 0; k < faultload[i].num_ops; k++) {
					if (faultload[i].op_type[k] == STRING) {
						SECURE_FREE(faultload[i].op_value[k].str.value);
					}
				}
				kfree(faultload[i].op_type);
				SECURE_FREE(faultload[i].op_value);
			}		
		} while (faultload[i++].block_type == 0);
	}
	
	SECURE_FREE(tikle_log_counters);

	printk(KERN_INFO "tikle alert: module unloaded\n");
}

module_init(tikle_init);
module_exit(tikle_exit);

MODULE_AUTHOR("c2zlabs");
MODULE_DESCRIPTION("faulT Injector for networK protocoL Evaluation");
MODULE_LICENSE("GPL");
