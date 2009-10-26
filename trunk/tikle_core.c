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
#include <net/netfilter/nf_queue.h> /* struct nf_queue_handler */
#include <linux/netfilter.h> /* kernel protocol stack */
#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS an others */
#include <linux/proc_fs.h> /* procfs manipulation */
#include <net/net_namespace.h> /* proc_net stuff */
#include <linux/sched.h> /* current-euid() */
#include <asm/uaccess.h> /* kernel-mode to user-mode and reverse */
#include <linux/kthread.h> /* kernel threads */
#include <linux/timer.h> /* temporization trigger stuff */
#include <linux/file.h> /* write to file */
#include <linux/random.h> /* tausworthe generators */
/*
 * path to netiflter constants
 * needed since kernel 2.6.27
 */
#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#include "tikle_hooks.h" /* tikle hooks */
#include "tikle_defs.h" /* tikle macros */

MODULE_AUTHOR("c2zlabs");
MODULE_DESCRIPTION("faulT Injector for networK protocoL Evaluation");
MODULE_LICENSE("GPLv3");

/**
 * New reference to proc_net
 */
#define proc_net init_net.proc_net

/**
 * Buffer control to /proc/net/tikle/shell
 */
static char tikle_buffer[TIKLE_PROCFSBUF_SIZE];

/**
 * tausworthe seeds
 */
struct tikle_seeds {
	u32 s1, s2, s3;
};

static struct tikle_seeds tikle_seed;

struct tikle_timer *tikle_timers;

/**
 * The size of the data
 * hold in the buffer
 */
static unsigned long tikle_buffer_size = 0;

unsigned long *tikle_log_counters = NULL;

unsigned long partition_ips[30];

/**
 * procfs structures.
 */
static struct proc_dir_entry *tikle_main_proc_dir,    /* /proc/net/tikle */
			     *tikle_shell_proc_entry; /* /proc/net/tikle/shell */

/**
 * @brief Called when a /proc/net/tikle file is "read".
 *
 * @param file the file structure
 * @param buffer the buffer to fill with data
 * @param length the lenght of the buffer
 * @param offset 
 * @return number of bytes read
 */
static ssize_t tikle_read(struct file *file, char *buffer, size_t length, loff_t * offset)
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
static ssize_t tikle_write(struct file *file, const char *buffer, size_t len, loff_t * off)
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
static int tikle_permission(struct inode *inode, int op
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

/**
 *  Reinject a packet in the protocol stack after the delay
 */
/*static void delay_unlock(struct nf_queue_entry *foo)
{
	nf_reinject(foo, NF_ACCEPT);
}*/

/**
 * the below routines are responsible
 * to emulate packet delay behavior
 */
/*static int tikle_delay_flow(struct sk_buff *pskb, struct nf_info *info, unsigned int queuenum, void *data)
{
	struct timer_list *timer;
	struct nf_queue_entry *foo;

	timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL | GFP_ATOMIC);

	foo->skb = pskb;

	*
	 * creating timer to emulate delay behavior
	 *
	init_timer(timer);
	timer->data = (unsigned long) foo;
	timer->function = (void *) delay_unlock;
	timer->expires = jiffies + faultload[tikle_trigger_flag].op_value[0].num * HZ / 1000;
	timer->base = &boot_tvec_bases;
	add_timer(timer);
	return 0;
}*/

/*static const struct nf_queue_handler qh = {
	.outfn = &tikle_delay_flow,
	.name = "queue handler",
};*/


/**
 * Free all memory used in faultload structure
 */
static void tikle_faultload_free(void)
{
	unsigned int i = 0, k;
	
	/* 
	 * Checking if the faultlets were loaded
	 */
	if (faultload[0].opcode) {
		printk(KERN_INFO "tikle alert: Freeing faultload structure\n");
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
}

/**
 * Setup the server to receive faultload.
 * @return 1 all the time
 */
static int tikle_sockudp_start(void)
{
	int size = -1, i = 0, tikle_err, trigger_count = 0;//, broadcast = 1;
	static int count = 0;
	char tikle_auth[sizeof("tikle-start")];

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
		|| ((tikle_err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &tikle_comm->sock_send)) < 0)
		|| ((tikle_err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &tikle_comm->sock_command_send)) < 0)) {
		printk(KERN_ERR "tikle alert: error %d while creating sockudp\n", -ENXIO);
		tikle_comm->thread_socket = NULL;
		tikle_comm->flag = 0;
	}

	memset(&tikle_comm->addr_recv, 0, sizeof(struct sockaddr)); 
	tikle_comm->addr_recv.sin_family = AF_INET;
	tikle_comm->addr_recv.sin_addr.s_addr = htonl(INADDR_ANY);
	tikle_comm->addr_recv.sin_port = htons(PORT_LISTEN);

	memset(&tikle_comm->addr_send, 0, sizeof(struct sockaddr));
	tikle_comm->addr_send.sin_family = AF_INET;
	tikle_comm->addr_send.sin_port = htons(PORT_CONNECT);

	if ((tikle_err = tikle_comm->sock_recv->ops->bind(tikle_comm->sock_recv,
			(struct sockaddr *)&tikle_comm->addr_recv, sizeof(struct sockaddr))) < 0)
		{
		printk(KERN_ERR "tikle alert: error %d while binding to socket\n", -tikle_err);
		sock_release(tikle_comm->sock_recv);
		tikle_comm->sock_recv = NULL; 
	}

	printk(KERN_INFO "tikle alert: listening on port %d\n", PORT_LISTEN);

	/*
	 * socket to send user commands through the network
	 * ignored, until we can add broadcast permission
	 * to socket in kernel space
	 */
	/*if (tikle_comm->sock_command_send->ops->setsockopt(tikle_comm->sock_command_send, SOL_SOCKET,
				SO_BROADCAST, (void *)&broadcast, sizeof(broadcast)) < 0) {
		printk(KERN_INFO "error while setting broadcast permission to socket\n");
		return 0;
	}

	memset(&tikle_comm->addr_command_send, 0, sizeof(struct sockaddr));
	tikle_comm->addr_command_send.sin_family = AF_INET;
	tikle_comm->addr_command_send.sin_addr.s_addr = htonl(INADDR_ANY);
	tikle_comm->addr_command_send.sin_port = htons(PORT_COMMAND);*/

	/*
	 * looping to receive data from other hosts
	 * and after that sends a confirmation message
	 */
	TDEBUG("- start------------------------------------\n");
	memset(faultload, 0, sizeof(faultload));
	
	size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
		&num_ips, sizeof(int));
	
	if (size < 0) {
		printk(KERN_ERR "tikle alert: error %d while getting datagram\n", size);
	} else {
		TDEBUG("num_ips=%d\n", num_ips);
		TDEBUG("tikle alert: received %d bytes\n", size);
	}
	if (num_ips) {
		size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
			&partition_ips, sizeof(unsigned long) * num_ips);
	
		if (size < 0) {
			printk(KERN_ERR "tikle alert: error %d while getting datagram\n", size);
		} else {
			int j;
			
			TDEBUG("declare={");
			for (j = 0; j < num_ips; j++) {
				TDEBUG("%lu%s", partition_ips[j], (j == (num_ips-1) ? "" : ","));
			}
			TDEBUG("}\n");
			
			TDEBUG("tikle alert: received %d bytes\n", size);
		}
	}

	while (1) {
		switch (count) {
			case 0: /* Opcode */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].opcode, sizeof(faultload_opcode));
				if (faultload[i].opcode == AFTER) {
					trigger_count++;
				}
				TDEBUG("opcode: %d\n", faultload[i].opcode);
				break;
			case 1: /* Protocol */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].protocol, sizeof(faultload_protocol));
				TDEBUG("protocol: %d\n", faultload[i].protocol);
				break;
			case 2: /* Occur type (TEMPORAL, PACKETS, PERCT) */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].occur_type, sizeof(faultload_num_type));
				TDEBUG("occur_type: %d\n", faultload[i].occur_type);
				break;
			case 3: /* Number of ops */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
					&faultload[i].num_ops, sizeof(unsigned short int));
				TDEBUG("num_ops: %d\n", faultload[i].num_ops);
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
					
					faultload[i].op_type = kcalloc(faultload[i].num_ops, sizeof(faultload_op_type), GFP_KERNEL | GFP_ATOMIC);

					size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
						faultload[i].op_type, sizeof(faultload_op_type) * faultload[i].num_ops);

					for (k = 0; k < faultload[i].num_ops; k++) {
						TDEBUG("op_type%d [%d]\n", k+1, faultload[i].op_type[k]);
					}
				}
				break;
			case 5: /* Operand values */
				{
					unsigned short int k;
						
					faultload[i].op_value = kcalloc(faultload[i].num_ops, sizeof(faultload_value_type), GFP_KERNEL | GFP_ATOMIC);
					size = 0;
					for (k = 0; k < faultload[i].num_ops; k++) {
						switch (faultload[i].op_type[k]) {
							case STRING:
								tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
									&faultload[i].op_value[k].str.length, sizeof(size_t));
								faultload[i].op_value[k].str.value = (char*) kmalloc(faultload[i].op_value[k].str.length, GFP_KERNEL | GFP_ATOMIC);
								size += tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
								faultload[i].op_value[k].str.value, faultload[i].op_value[k].str.length);
								
								TDEBUG("%d string [%s] (len: %d)\n", k+1, faultload[i].op_value[k].str.value,
									faultload[i].op_value[k].str.length);
								break;
							case ARRAY:
								{
									unsigned int j;
									
									tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
										&faultload[i].op_value[k].array.count, sizeof(size_t));
									size += tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
										&faultload[i].op_value[k].array.nums, sizeof(unsigned long) * faultload[i].op_value[k].array.count);
									
									TDEBUG("%d array {", k+1);
									for (j = 0; j < faultload[i].op_value[k].array.count; j++) {
										TDEBUG("%ld%s", faultload[i].op_value[k].array.nums[j],
											(j == (faultload[i].op_value[k].array.count-1) ? "" : ","));
									}
									TDEBUG("}\n");
								}
								break;
							default:
								size += tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
									&faultload[i].op_value[k].num, sizeof(unsigned long));
									
								TDEBUG("%d number: %ld\n", k+1, faultload[i].op_value[k].num);
								break;
						}
					}
				}
				break;
			case 6: /* Extended value */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
							&faultload[i].extended_value, sizeof(int));
				TDEBUG("Extended value: %d\n", faultload[i].extended_value);
				break;
			case 7: /* Label */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
							&faultload[i].label, sizeof(unsigned long));
				TDEBUG("Label: %ld\n", faultload[i].label);
				break;
			case 8: /* Block type (START, STOP) */
				size = tikle_sockudp_recv(tikle_comm->sock_recv,
							&tikle_comm->addr_recv,	&faultload[i].block_type,
							sizeof(short int));
				TDEBUG("Block type: %d\n", faultload[i].block_type);
				break;
			case 9: /* Control op number */
				size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv,
							&faultload[i].next_op, sizeof(unsigned int));
				TDEBUG("Op number: %d\n", faultload[i].next_op);
				break;
			default:
				TDEBUG("tikle alert: unexpected type\n");
				break;
		}
		
		if (signal_pending(current)) {
			printk(KERN_INFO "tikle alert: nothing\n");
			goto next;
		}
		
		if (size < 0) {
			printk(KERN_ERR "tikle alert: error %d while getting datagram\n", size);
		} else {				
			TDEBUG("received %d bytes\n", size);
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
	TDEBUG("- end------------------------------------\n");

	/*
	 * get the controller ip address
	 */
	tikle_comm->addr_send.sin_addr.s_addr = tikle_comm->addr_recv.sin_addr.s_addr;

	/*
	 * sending confirmation of data received
	 * (for control protocol stuffs)
	 */
	if ((tikle_err = tikle_comm->sock_send->ops->connect(tikle_comm->sock_send,
			(struct sockaddr *)&tikle_comm->addr_send, sizeof(struct sockaddr), 0)) < 0)
	{
		printk(KERN_ERR "tikle alert: error %d while connecting to socket\n", -tikle_err);
		sock_release(tikle_comm->sock_send);
		tikle_comm->sock_send = NULL;
	}

	printk(KERN_INFO "tikle alert: sending confirmation of received data\n");

	tikle_sockudp_send(tikle_comm->sock_send, &tikle_comm->addr_send,
		"tikle-received", sizeof("tikle-received"));

	/*
	 * waiting for controller
	 * authorization to start
	 */
	printk(KERN_ERR "tikle alert: waiting for authorization to start\n");

	size = tikle_sockudp_recv(tikle_comm->sock_recv, &tikle_comm->addr_recv, tikle_auth, sizeof("tikle-start"));

	printk(KERN_ERR "tikle alert: received \"%s\" from controller\n", tikle_auth);

	if (size < 0) {
		printk(KERN_ERR "tikle alert: error %d while getting authorization\n", size);
		tikle_faultload_free();
		
		return 0;
	} else if (strncmp(tikle_auth, "tikle-start", sizeof("tikle-start")) == 0) { 
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
					i = faultload[i].next_op - 1;
					break;
				default:
					break;
			}
		} while (faultload[i++].block_type == 0);
		
		/*
		 * preparing log counters
		 */
		if (num_ips) {
			/* -1 because the STOP in AFTER part */
			TDEBUG("tikle alert: log_size = %d (timers) * %d (ips)\n", (tikle_num_timers-1), num_ips);
			
			log_size = (tikle_num_timers-1) * num_ips * 5;
			tikle_log_counters = kcalloc(log_size, sizeof(unsigned long), GFP_KERNEL | GFP_ATOMIC);
		}

		/*
		 * Register trigger timers
		 */
		tikle_trigger_handling();
	} else {
		printk(KERN_ERR "tikle alert: you don't have permission to start execution. received: '%s'\n", tikle_auth);
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

/*static int tikle_command_sender(void)
{
	return 1;
}*/
/**
 * function to perform user commands while test is running (not finished yet)
 */
/*static int tikle_command_listener(void)
{
	int tikle_err = -1, size = -1;
	char tikle_halt[sizeof("tikle-halt")];

	lock_kernel();
	tikle_comm->flag = 1;
	current->flags |= PF_NOFREEZE;
	daemonize("thread_halt");
	allow_signal(SIGKILL);
	unlock_kernel();

	if ((tikle_err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &tikle_comm->sock_command_recv)) < 0) {
		printk(KERN_ERR "tikle alert: error %d while creating sockudp\n", -ENXIO);
		tikle_comm->thread_command = NULL;
		tikle_comm->flag = 0;
	}

	memset(&tikle_comm->addr_command_recv, 0, sizeof(struct sockaddr)); 
	tikle_comm->addr_command_recv.sin_family = AF_INET;
	tikle_comm->addr_command_recv.sin_addr.s_addr = htonl(INADDR_ANY);
	tikle_comm->addr_command_recv.sin_port = htons(PORT_COMMAND);

	if ((tikle_err = tikle_comm->sock_command_recv->ops->bind(tikle_comm->sock_command_recv,
			(struct sockaddr *)&tikle_comm->addr_command_recv, sizeof(struct sockaddr))) < 0)
		{
		printk(KERN_ERR "tikle alert: error %d while binding to socket\n", -tikle_err);
		sock_release(tikle_comm->sock_command_recv);
		tikle_comm->sock_command_recv = NULL; 
	}

	size = tikle_sockudp_recv(tikle_comm->sock_command_recv, &tikle_comm->addr_command_recv, tikle_halt, sizeof("tikle-halt"));

	printk(KERN_INFO "tikle alert: received %s\n", tikle_halt);

	return tikle_err;

}*/

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
	tikle_shell_proc_entry = create_proc_entry("shell", S_IWUSR, tikle_main_proc_dir);

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
	 * sorting seeds
	 */
	tikle_random(&tikle_seed);

	printk(KERN_INFO "tikle alert: module loaded\n");
	TDEBUG("tikle alert: seeds are 0x%x 0x%x 0x%x\n", tikle_seed.s1, tikle_seed.s2, tikle_seed.s3);

	/*
	 * preparing thread to
	 * init socket stuff
	 */
	tikle_comm = kmalloc(sizeof(struct tikle_sockudp), GFP_KERNEL);
	memset(tikle_comm, 0, sizeof(struct tikle_sockudp));

	tikle_comm->thread_socket = kthread_run((void *)tikle_sockudp_start, NULL, "socket");
	//tikle_comm->thread_command = kthread_run((void *)tikle_command_listener, NULL, "command");

	if ((IS_ERR(tikle_comm->thread_socket))) { //|| (IS_ERR(tikle_comm->thread_command))) {
		printk(KERN_ERR "tikle alert: error while running kernel thread\n");
		kfree(tikle_comm);
		tikle_comm = NULL;
		tikle_err = -ENOMEM;
		goto tikle_exit;		
	}

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
	/*
	 * cleanup procfs
	 */
	remove_proc_entry("shell", tikle_main_proc_dir);
	remove_proc_entry("tikle", proc_net);

	/*
	 * cleanup memory/socket
	 */
	if (!tikle_comm->sock_recv || tikle_comm->sock_send) { // || tikle_comm->sock_command_recv) {
		sock_release(tikle_comm->sock_recv);
		sock_release(tikle_comm->sock_send);
		//sock_release(tikle_comm->sock_command_recv);
		tikle_comm->sock_recv = NULL;
		tikle_comm->sock_send = NULL;
		//tikle_comm->sock_command_recv = NULL;
	}

	kfree(tikle_comm);
	kfree(tikle_timers);
	tikle_comm = NULL;

	/*
	 * Freeing faultload structure if needed
	 */
	tikle_faultload_free();
	
	/*
	 * Freeing counters
	 */
	SECURE_FREE(tikle_log_counters);

	printk(KERN_INFO "tikle alert: module unloaded\n");
}

module_init(tikle_init);
module_exit(tikle_exit);
