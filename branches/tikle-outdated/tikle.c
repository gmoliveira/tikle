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

#include <linux/kthread.h> /* kernel threads */
#include <linux/timer.h> /* temporization trigger stuff */

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
 * Struct to register hook operations
 */
static struct nf_hook_ops tikle_pre_hook;

/**
 * Opcode names
 */
static const char *op_names[] = { 
	"COMMAND", "AFTER", "WHILE", "HOST", "IF", "ELSE",
	"END_IF", "END", "SET", "FOREACH", "PARTITION", "DECLARE"
};

static int num_ips;

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
 * Free all memory used in faultload structure
 */
static void tikle_faultload_free(void)
{
	unsigned int i = 0, k;
	
	/* 
	 * Checking if the faultlets were loaded
	 */
	if (faultload[0].opcode) {
		printk(KERN_INFO "tikle log: Freeing faultload structure\n");
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
 * Update flags to maintain a timer execution control.
 *
 * @param id active trigger id
 * @return void
 */
static void tikle_flag_handling(unsigned long id)
{
	char *log;

	log = kmalloc(100 * sizeof(char), GFP_KERNEL | GFP_ATOMIC);

	/*
	 * set the previous timer as 'killed'
	 * and update the system flags with the
	 * running trigger informations
	 */
	if (id > 0 && tikle_timers[tikle_trigger_flag].trigger_state == 1) {
		tikle_timers[tikle_trigger_flag].trigger_state = 2;
		printk(KERN_INFO "tikle alert: killing timer %d\n", tikle_trigger_flag);

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

	if (log == NULL) {
		printk(KERN_INFO "tikle log: Unable to alloc memory to log\n");
	} else {
		sprintf(log, "timestamp: %ld @ event: %s",
				jiffies, op_names[faultload[id].opcode]);

		printk(KERN_INFO "tikle alert: %s\n", log);

		kfree(log);
	}

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

	if (log == NULL) {
		printk(KERN_INFO "tikle_log: Unable to alloc memory to log\n");
	} else {

		printk(KERN_INFO "tikle alert: hook unregistered at %u ms --- %u s\n",
				jiffies_to_msecs(jiffies), jiffies_to_msecs(jiffies)/1000);

		printk(KERN_INFO "tikle log: execution ended after %u ms --- %u s\n",
				jiffies_to_msecs(jiffies), jiffies_to_msecs(jiffies)/1000);

		kfree(log);
	}
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
	printk(KERN_INFO "Registering triggers:\n");
	
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
		
		printk(KERN_INFO "Registering timer %d; trigger_id=%d ; trigger_data=%ld\n",
			i, tikle_timers[i].trigger_id, tikle_timers[i].trigger.data);

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

		add_timer(&tikle_timers[i].trigger);
	}
	printk(KERN_INFO "--------------------------------\n");
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
	struct sk_buff *sb = pskb;
	unsigned long array_local = 0, array_remote = 1;
	int i = 0, array, position;
	char *log;

	/*
	 * log system syntax (in order):
	 * timestamp - in jiffies
	 * sender/receiver - ipv4 format
	 * message protocol - integer
	 *
	 * ps: 100 is a 'sorted' value of my limited brain
	 */
	log = kmalloc(100 * sizeof(char), GFP_KERNEL | GFP_ATOMIC);
	
	if (log == NULL) {
		printk(KERN_INFO "tikle log: Unable to alloc memory to log\n");
	} else {
		sprintf(log, "INCOMING timestamp: %ld @ remetente: " NIPQUAD_FMT " destinatario: " NIPQUAD_FMT " @ protocolo: %d",
				jiffies,
				NIPQUAD(ipip_hdr(sb)->saddr),
				NIPQUAD(ipip_hdr(sb)->daddr),
				ipip_hdr(sb)->protocol);

		printk(KERN_INFO "tikle log: %s\n", log);

		kfree(log);
	}

	/*
	 * packets will be intercepted only if a timer is active. if
	 * it is, process packets according to the flagged trigger
	 */
	if (tikle_timers[tikle_trigger_flag].trigger_state != 1) {
		return NF_ACCEPT;
	}
	
	/* Current AFTER */
	i = tikle_timers[tikle_trigger_flag].trigger_id;
	
	do {
		printk(KERN_INFO "i=%d ; opcode: %d (%s) ; next_op=%d\n",
			i, faultload[i].opcode, op_names[faultload[i].opcode], faultload[i].next_op);
		
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
					return NF_ACCEPT;
				} else {
					return NF_DROP;
				}
				break;
		}
	} while (++i < faultload[tikle_timers[tikle_trigger_flag].trigger_id].next_op);

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
		void *buf, int len)
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
		printk(KERN_INFO "num_ips=%d\n", num_ips);
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
				printk(KERN_INFO "num_ops: %d\n", faultload[i].num_ops);
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
						printk(KERN_INFO "op_type%d [%d]\n", k+1, faultload[i].op_type[k]);
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
					tikle_timers[tikle_num_timers].trigger_id = i;
					tikle_timers[tikle_num_timers++].trigger_state = 0;
					i = faultload[i].next_op - 1;
					break;
				default:
					break;
			}
		} while (faultload[i++].block_type == 0);

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
 * Module initialization routines.
 *
 * This function create /proc/net directory entries and
 * start the faultload server.
 *
 * @return 0 on success or ENOMEM on error
 */
static int __init tikle_init(void)
{
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
		return -ENOMEM;
	}

	printk(KERN_INFO "tikle alert: module loaded\n");

	return 0;
}


/**
 * Cleanup routines to unload the module.
 *
 * Removes the procfs entries and shutdown the server.
 */
static void __exit tikle_exit(void)
{
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

	/*
	 * Freeing faultload structure if needed
	 */
	tikle_faultload_free();
	
	printk(KERN_INFO "tikle alert: module unloaded\n");
}

module_init(tikle_init);
module_exit(tikle_exit);

MODULE_AUTHOR("c2zlabs");
MODULE_DESCRIPTION("faulT Injector for networK protocoL Evaluation");
MODULE_LICENSE("GPL");
