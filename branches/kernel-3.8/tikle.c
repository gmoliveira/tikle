/* 
 * PIE kernel module
 * Copyright (C) 2009  Felipe 'ecl' Pena
 *		     Gustavo 'nst' Oliveira
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

#include <linux/in.h> /* sockaddr_in and other macros */
#include <linux/ip.h> /* ipip_hdr(const struct sk_buff *sb) */
#include <linux/udp.h>

#include <linux/timer.h> /* temporization trigger stuff */

#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS an others */

#include <linux/kthread.h> /* kernel threads */

#if HAVE_UNLOCKED_IOCTL
    #include <linux/mutex.h>
#else
    #include <linux/smp_lock.h>
#endif

#include <linux/netfilter.h> /* kernel protocol staff */
#include <linux/netfilter_ipv4.h>

#define PORT_LISTEN 12608
#define PORT_CONNECT 21508
#define PORT_LOGGING 24187

#define INT32_MAX (2147483647)
#define NUM_FAULTLOAD_OP 10

#define SECURE_FREE(_var) \
        if (_var) {           \
                kfree(_var);      \
        }
//static uint32_t f_get_random_bytes(uint32_t *, uint32_t *, uint32_t *);

static DEFINE_MUTEX(fs_mutex);

int eof = 1;

/**
 * Actions
 */
typedef enum {
	ACT_DELAY = 0,
	ACT_DUPLICATE,
	ACT_DROP
} faultload_action;


/**
 * Protocols
 */
typedef enum  {
	ALL_PROTOCOL = 0,
	TCP_PROTOCOL = 6,
	UDP_PROTOCOL = 17,
	SCTP_PROTOCOL = 132
} faultload_protocol;


/**
 * Numeric representation type
 * 1  -> NONE
 * 1s -> TEMPORAL
 * 1p -> NPACKETS
 * 1% -> PERCT
 */
typedef enum _faultload_num_type {
	NONE = 0,
	TEMPORAL,
	NPACKETS,
	PERCT
} faultload_num_type;


/**
 * Opcode data type structure
 */
typedef union {
	unsigned long num;
	struct _str_value {
		size_t length;
		char *value;
	} str;
	struct _array_value {
		size_t count;
		unsigned long nums[10];
	} array;
} faultload_value_type;


/**
 * Opcode type
 */
typedef enum {
	COMMAND,
	AFTER,
	WHILE,
	HOST,
	IF,
	ELSE,
	END_IF,
	END,
	SET,
	FOREACH,
	PARTITION,
	DECLARE
} faultload_opcode;


/**
 * Op data type
 */
typedef enum {
	UNUSED = 0,
	NUMBER,
	STRING,
	VAR,
	ARRAY
} faultload_op_type;

/**
 * Opcode structure
 */
typedef struct {
	faultload_opcode opcode;
	faultload_protocol protocol;
	faultload_num_type occur_type;
	unsigned short int num_ops;
	faultload_op_type *op_type;
	faultload_value_type *op_value;
	int extended_value;
	unsigned long label;
	short int block_type;
	unsigned int next_op;
} faultload_op;

struct timer_event {
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

struct timer_event *event;

struct nf_hook_ops pre_hook, post_hook;

/**
 *  * Opcode names
 *   */
static const char *op_names[] = {
        "COMMAND", "AFTER", "WHILE", "HOST", "IF", "ELSE",
        "END_IF", "END", "SET", "FOREACH", "PARTITION", "DECLARE"
};


struct data_listen {
	int flag;
	struct task_struct *t_func;
	struct socket *s_recv, *s_send;
	struct sockaddr_in addr_recv, addr_send;
};

struct data_listen *pie_sock;

faultload_op faultload[30];

unsigned long partition_ips[30];

unsigned long *log_counters = NULL;
//int log_size, line_size;

//static uint32_t seed1, seed2, seed3;

int num_ips, meuip; //active
unsigned int num_events = 0;
static unsigned int trigger_flag = 0;
MODULE_AUTHOR("c2zlabs");
MODULE_DESCRIPTION("Partitioning Injection Environment");
MODULE_LICENSE("GPL");


/**
 * Free all memory used in faultload structure
 */
static void free_faultload(void)
{
        unsigned int i = 0, k;
        
        /* 
         * Checking if the faultlets were loaded
         */
        if (faultload[0].opcode) {
                printk(KERN_INFO "freeing faultload structure\n");
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

int recv_msg(struct socket *sock,
		struct sockaddr_in *addr,
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

	msg.msg_flags = O_NONBLOCK;
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
 * @sa recv_msg()
 */
int send_msg(struct socket *sock,
		struct sockaddr_in *addr,
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
	size = sock_sendmsg(sock, &msg, len);
	set_fs(oldfs);

	return size;
}


unsigned int f_pre_hook(unsigned int hooknum,
		struct sk_buff *pskb,
		const struct net_device *indev,
		const struct net_device *outdev,
		int (*okfn)(struct sk_buff *))
{

	unsigned long array_local = 0, array_remote = 1;
	int i = 0, array, position;
	struct sk_buff *sb = pskb;

	if (event[trigger_flag].trigger_state != 1) {
		return NF_ACCEPT;
	}
//		for (i = 0; log_counters[i+2] && i < log_size; i+=line_size) {
//			if (log_counters[i+2] == ipip_hdr(sb)->saddr) {
//				printk(KERN_INFO "host encontrado na posicao %d\n", i);
//				break;
//			} else {
//				continue;
//			}
//		}
//		log_counters[i+0] = 0;
//		log_counters[i+1] = ipip_hdr(sb)->daddr;
//		log_counters[i+2] = ipip_hdr(sb)->saddr;
//		log_counters[i+4*trigger_flag+3] = 0;
//		log_counters[i+4*trigger_flag+4] = trigger_flag;
//		log_counters[i+4*trigger_flag+5] += 1;
//		return NF_ACCEPT;
//	}


//	for (i = 0; log_counters[i+2] && i < log_size; i+=line_size) {
//		if (log_counters[i+2] == ipip_hdr(sb)->saddr) {
//			printk(KERN_INFO "host encontrado na posicao %d\n", i);
//			break;
//		} else {
//			continue;
//		}
//	}
//	log_counters[i+0] = 0;
//	log_counters[i+1] = ipip_hdr(sb)->daddr;
//	log_counters[i+2] = ipip_hdr(sb)->saddr;
//	log_counters[i+4*(trigger_flag+1)+3] = 0;
//	log_counters[i+4*(trigger_flag+1)+4] = trigger_flag+1;
//	log_counters[i+4*(trigger_flag+1)+5] += 1;

	
	i = event[trigger_flag].trigger_id;

	do {

//                printk(KERN_INFO "ACTION=%lu ; i=%d ; opcode: %d (%s) ; next_op=%d\n",
//			faultload[i].op_value[0].num, i, faultload[i].opcode, op_names[faultload[i].opcode], faultload[i].next_op);
               
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
                                 *      ACT_DROP (1)
                                 *      ACT_DUPLICATE (2)
                                 *      ACT_DELAY (3)
                                 *
                                 * progressive: faultload[i].extended_value
                                 * 10: faultload[i].op_value[1].num
                                 */
				switch (faultload[i].op_value[0].num) {
					//case ACT_DELAY:
						//NF_QUEUE;
					case ACT_DROP:
						printk(KERN_INFO "DROP\n");
						return NF_DROP;
					//case ACT_DUPLICATE:
					//	duplicate_sb = skb_copy(pskb, GFP_ATOMIC);
					//if (duplicate_sb) {
					//	okfn(duplicate_sb);
					//}
					return NF_ACCEPT;
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
                                 *              'is equal'     = 1
                                 *              'is not equal' = 2
                                 *
                                 * 127.0.0.1: faultload[i].op_value[1] (inet_addr)
                                 *            faultload[i].op_type[1] == NUMBER
                                 */
                                /*
                                 * if (!(skip = tikle_handler_if(faultload[i]))) {
                                 *              i = skip;
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
				printk(KERN_INFO "PARTITION\n");
				for (array = 0; array < faultload[i].num_ops; array++) {
					for (position = 0; position < faultload[i].op_value[array].array.count; position++) {
						if (faultload[i].op_value[array].array.nums[position] == ipip_hdr(sb)->saddr)
							array_remote = array;
						else if (faultload[i].op_value[array].array.nums[position] == meuip)
							array_local = array;
					}
				}
				if (array_local == array_remote)
					return NF_ACCEPT;
				else 
					return NF_DROP;
				break;
			default:
				break;
		}
	} while (++i < faultload[event[trigger_flag].trigger_id].next_op);

	return NF_ACCEPT;
}

unsigned int f_post_hook(unsigned int hooknum,
		struct sk_buff *pskb,
		const struct net_device *indev,
		const struct net_device *outdev,
		int (*okfn)(struct sk_buff *))
{

//	if (active == 0) {
//		return NF_ACCEPT;
//	}

	return NF_ACCEPT;
}


//int log_finalize(void)
//{
//	int err;
//	struct socket *sock_log;
//	struct sockaddr_in addr_log;
//
//	if ((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_log)) < 0)
//		printk(KERN_INFO "error whilie log socket creation. erro: %d\n", err);
//
////	memset(&addr_log, 0, sizeof(struct sockaddr));
//	addr_log.sin_family = AF_INET;
//	addr_log.sin_addr.s_addr = pie_sock->addr_recv.sin_addr.s_addr;
//	addr_log.sin_port = htons(PORT_LOGGING);
//
//	if ((err = sock_log->ops->connect(sock_log, (struct sockaddr *)&addr_log, sizeof(struct sockaddr), 0)) < 0) {
//		printk(KERN_INFO "error while connecting socket. erro: %d\n", err);
//		sock_release(sock_log);
//		sock_log = NULL;
//	}
//
//	printk("enviando logs\n");
//	send_msg(sock_log, &addr_log, log_counters, sizeof(unsigned long) * log_size);
//
//	return 0;
//}


void flag_handler(unsigned long id)
{

	/*
	 * set the previous timer as 'killed'
	 * and update the system flags with
	 * the running trigger informations
	 */
	if (id > 0 && event[trigger_flag].trigger_state == 1) {
		event[trigger_flag].trigger_state = 2;

		/*
		 * go to next timer
		 */
		trigger_flag++;
	}

	/*
	 * set trigger state to active
	 */
	event[trigger_flag].trigger_state = 1;

	if (id == event[trigger_flag].trigger_id) {
		printk(KERN_INFO "End of hooks\n");
		//active = 1;
		//log_finalize();
	}
}

int event_handler(void)
{

	unsigned int i = 0;

	/*
	 * setting up triggers
	 */
	printk(KERN_INFO "Triggers:\n");

	for (; i < num_events; i++) {

		/*
		 * send the next trigger ID, if actual trigger it is
		 * not the latest, else send your own trigger ID
		 */
		if (event[i].trigger_id < event[num_events-1].trigger_id)
			setup_timer(&event[i].trigger, flag_handler, event[i+1].trigger_id);
		else
			setup_timer(&event[i].trigger, flag_handler, event[i].trigger_id);

		printk(KERN_INFO "Registering trigger %d; id=%d; data=%ld\n", i, event[i].trigger_id, event[i].trigger.data);

		/*
		 * calculate expire time to trigger
		 * based on the experiment starting time
		 */
		mod_timer(&event[i].trigger, jiffies + msecs_to_jiffies(faultload[event[i].trigger_id].op_value[0].num * 1000));

		/*
		 * if timer corresponds to the first timer
		 */
		if (event[i].trigger_id == 0) {
			/*
			 * register hook
			 */
			pre_hook.hook = &f_pre_hook;
			pre_hook.hooknum = NF_INET_PRE_ROUTING;
			pre_hook.pf = PF_INET;
			pre_hook.priority = INT_MIN;

			post_hook.hook = &f_post_hook;
			post_hook.hooknum = NF_INET_POST_ROUTING;
			post_hook.pf = PF_INET;
			post_hook.priority = INT_MIN;

		/*
		 * if timer corresponds
		 * to the last timer
		 */
		}
			
	}
	printk(KERN_INFO "---------------------------\n");

	nf_register_hook(&pre_hook);
	nf_register_hook(&post_hook);
	eof = 0;
//	active = 0;

	return 0;
}


static int f_main_sock(void)
{
	int err, size = -1, trigger_count = 0, i = 0;
	static int count = 0;
	char auth_msg[12];



	//printk(KERN_INFO, "foo %0x%x\n", f_get_random_bytes(&seed1, &seed2, &seed3));

	/*
	 * define flags of the thread and run socket
	 * server as a daemon (run in background)
	 */
#if HAVE_UNLOCKED_IOCTL
	mutex_lock(&fs_mutex);
#else
	lock_kernel();
#endif
	pie_sock->flag = 1;
	current->flags |= PF_NOFREEZE;
	allow_signal(SIGKILL);
#if HAVE_UNLOCKED_IOCTL
	mutex_unlock(&fs_mutex);
#else
	unlock_kernel();
#endif

	/*
	 * socket creation
	 * listening on port 12608
	 * connecting on port 21508
	 */

	if (((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &pie_sock->s_recv)) < 0) ||
		((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &pie_sock->s_send)) < 0)) {
		pie_sock->t_func = NULL;
		pie_sock->flag = 0;
	}

	memset(&pie_sock->addr_recv, 0, sizeof(struct sockaddr)); 
	pie_sock->addr_recv.sin_family = AF_INET;
	pie_sock->addr_recv.sin_addr.s_addr = htonl(INADDR_ANY);
	pie_sock->addr_recv.sin_port = htons(PORT_LISTEN);

	memset(&pie_sock->addr_send, 0, sizeof(struct sockaddr));
	pie_sock->addr_send.sin_family = AF_INET;
	pie_sock->addr_send.sin_port = htons(PORT_CONNECT);

	if ((err = pie_sock->s_recv->ops->bind(pie_sock->s_recv,
		(struct sockaddr *)&pie_sock->addr_recv, sizeof(struct sockaddr))) < 0) {
		sock_release(pie_sock->s_recv);
		pie_sock->s_recv = NULL; 
	}

	memset(faultload, 0, sizeof(faultload));

	size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &num_ips, sizeof(int));

	if (size < 0) {
		printk(KERN_INFO "error %d while getting datagram\n", size);
		eof = 1;
		return size;
	} else {
		printk(KERN_INFO "num_ips = %d\n", num_ips);
	}
	if (num_ips) {

		size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &meuip, sizeof(int));

		size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
				&partition_ips, sizeof(unsigned long) * num_ips);


		if (size < 0) {
			printk(KERN_INFO "error %d while getting datagram\n", size);
			eof = 1;
			return size;
		} else {
			int j;

			printk(KERN_INFO "declare={");
			for (j = 0; j < num_ips; j++) {
				printk(KERN_INFO "%lu%s", partition_ips[j], (j == (num_ips-1) ? "" : ","));
			}
			printk("}\n");
		}
	}

	while (1) {
		switch (count) {
			case 0: /* Opcode */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
					&faultload[i].opcode, sizeof(faultload_opcode));
				if (faultload[i].opcode == AFTER) {
					trigger_count++;
				}
				printk(KERN_INFO "opcode: %d\n", faultload[i].opcode);
				break;
			case 1: /* Protocol */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
					&faultload[i].protocol, sizeof(faultload_protocol));
				printk(KERN_INFO "protocol: %d\n", faultload[i].protocol);
				break;
			case 2: /* Occur type (TEMPORAL, PACKETS, PERCT) */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
					&faultload[i].occur_type, sizeof(faultload_num_type));
				printk(KERN_INFO "occur_type: %d\n", faultload[i].occur_type);
				break;
			case 3: /* Number of ops */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
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
					
					faultload[i].op_type = kcalloc(faultload[i].num_ops, sizeof(faultload_op_type), GFP_ATOMIC);


					size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
						faultload[i].op_type, sizeof(faultload_op_type) * faultload[i].num_ops);


					for (k = 0; k < faultload[i].num_ops; k++) {
						printk(KERN_INFO "op_type%d [%d]\n", k+1, faultload[i].op_type[k]);
					}
				}
				break;
			case 5: /* Operand values */
				{
					unsigned short int k;
						
					faultload[i].op_value = kcalloc(faultload[i].num_ops, sizeof(faultload_value_type), GFP_ATOMIC);
					size = 0;
					for (k = 0; k < faultload[i].num_ops; k++) {
						switch (faultload[i].op_type[k]) {
							case STRING:
								recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
									&faultload[i].op_value[k].str.length, sizeof(size_t));
								faultload[i].op_value[k].str.value = (char*) kmalloc(faultload[i].op_value[k].str.length, GFP_ATOMIC);
								size += recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
								faultload[i].op_value[k].str.value, faultload[i].op_value[k].str.length);
								
								printk(KERN_INFO "%d string [%s] (len: %zu)\n", k+1, faultload[i].op_value[k].str.value,
									faultload[i].op_value[k].str.length);
								break;
							case ARRAY:
								{
									unsigned int j;
									
									recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
										&faultload[i].op_value[k].array.count, sizeof(size_t));
									size += recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
										&faultload[i].op_value[k].array.nums, sizeof(unsigned long) * faultload[i].op_value[k].array.count);
									
									printk(KERN_INFO "%d array {", k+1);
									for (j = 0; j < faultload[i].op_value[k].array.count; j++) {
										printk(KERN_INFO "%ld%s", faultload[i].op_value[k].array.nums[j],
											(j == (faultload[i].op_value[k].array.count-1) ? "" : ","));
									}
									printk(KERN_INFO "}\n");
								}
								break;
							default:
								size += recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
									&faultload[i].op_value[k].num, sizeof(unsigned long));
									
								printk(KERN_INFO "%d number: %ld\n", k+1, faultload[i].op_value[k].num);
								break;
						}
					}
				}
				break;
			case 6: /* Extended value */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &faultload[i].extended_value, sizeof(int));
				printk(KERN_INFO "Extended value: %d\n", faultload[i].extended_value);
				break;
			case 7: /* Label */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &faultload[i].label, sizeof(unsigned long));
				printk(KERN_INFO "Label: %ld\n", faultload[i].label);
				break;
			case 8: /* Block type (START, STOP) */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &faultload[i].block_type, sizeof(short int));
				printk(KERN_INFO "Block type: %d\n", faultload[i].block_type);
				break;
			case 9: /* Control op number */
				size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &faultload[i].next_op, sizeof(unsigned int));
				printk(KERN_INFO "Op number: %d\n", faultload[i].next_op);
				break;
			default:
				printk(KERN_INFO "unexpected type\n");
				break;
		}
		
		if (signal_pending(current)) {
			printk(KERN_INFO "signal_pending returned true\n");
			goto next;
		}
		
		if (size < 0) {
			printk(KERN_INFO "error %d while getting datagram\n", size);
			eof = 1;
			return size;
		} else {				
			printk(KERN_INFO "received %d bytes\n", size);
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

	pie_sock->addr_send.sin_addr.s_addr = pie_sock->addr_recv.sin_addr.s_addr;

	if ((err = pie_sock->s_send->ops->connect(pie_sock->s_send,
			(struct sockaddr *)&pie_sock->addr_send, sizeof(struct sockaddr), 0)) < 0)
	{
		printk(KERN_INFO "error %d while connecting to socket\n", -err);
		sock_release(pie_sock->s_send);
		pie_sock->s_send = NULL;
	}

	printk(KERN_INFO "sending confirmation of received data\n");

	send_msg(pie_sock->s_send, &pie_sock->addr_send, "pie:received", sizeof("pie:received"));

	printk(KERN_INFO "waiting for authorization to start\n");

	size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, auth_msg, sizeof(char) + 10);

	printk(KERN_INFO "received %s from controller\n", auth_msg);

	if (size < 0) {
		printk(KERN_INFO "error %d while getting authorization\n", size);
		eof = 1;
		free_faultload();
		return 0;
	} else if (strncmp(auth_msg, "pie:start", sizeof("pie:start")) == 0) { 
		printk(KERN_INFO "received permission to start execution\n");
	
		event = kmalloc(trigger_count * sizeof(struct timer_event), GFP_ATOMIC);

		i = 0;

		do {
			switch (faultload[i].opcode) {
				case AFTER:
					event[num_events].trigger_id = i;
					event[num_events++].trigger_state = 0;
					i = faultload[i].next_op - 1;
					break;
				default:
					break;
			}
		} while (faultload[i++].block_type == 0);

		//if (num_ips) {
		//	line_size = 4 * (num_events) + 3;
		//	log_size = (num_ips-1) * line_size;
		//	log_counters = kcalloc(log_size, sizeof(unsigned long), GFP_KERNEL | GFP_ATOMIC);

		//	if (log_counters == NULL)
		//		return 0;
		//}
      	
		event_handler();
	} else { 
		printk(KERN_INFO "you don't have permission to start execution. received: '%s'\n", auth_msg);
		eof = 1;
	}
	return 0;

}


//static uint32_t f_get_random_bytes(uint32_t * s1, uint32_t * s2, uint32_t * s3)
//{
//	*s1 = (((*s1 & 0xFFFFFFFE) << 12) ^ (((*s1 << 13) ^ *s1) >> 19));
//	*s2 = (((*s2 & 0xFFFFFFF8) << 4) ^ (((*s2 << 2) ^ *s2) >> 25));
//	*s3 = (((*s3 & 0xFFFFFFF0) << 17) ^ (((*s3 << 3) ^ *s3) >> 11));
//	return *s1 ^ *s2 ^ *s3;
//}


/**
 * Module initialization routines.
 *
 * This function create /proc/net directory
 * entries and start the faultload server.
 *
 * @return 0 on success or ENOMEM on error
 */
static int __init pie_init(void)
{
	int err = 0;

//	get_random_bytes(&seed1, sizeof(seed1));
//	get_random_bytes(&seed2, sizeof(seed2));
//	get_random_bytes(&seed3, sizeof(seed3));

//	printk(KERN_INFO "seeds are 0x%x 0x%x 0x%x\n", seed1, seed2, seed3);

//	for (; err < 10; err++)
//		printk(KERN_INFO "RESULT %ld\n", f_get_random_bytes(&seed1, &seed2, &seed3) / ( INT32_MAX / 500 + 1));
//	err = 0;
	/*
	 * preparing thread to
	 * init socket stuff
	 */
	pie_sock = kmalloc(sizeof(struct data_listen), GFP_ATOMIC);
	memset(pie_sock, 0, sizeof(struct data_listen));

	pie_sock->t_func = kthread_run((void *)f_main_sock, NULL, "socket");

	if ((IS_ERR(pie_sock->t_func))) {

		kfree(pie_sock);
		pie_sock = NULL;
		err = -ENOMEM;

		return err;
	}

	printk(KERN_INFO "Module loaded\n");
	
	return err;

}


/**
 * Cleanup routines to unload the module.
 *
 * Removes the procfs entries and shutdown the server.
 */
static void __exit pie_exit(void)
{
	int i = 0;

	/*
	 * cleanup memory/socket
	 */
	if (!pie_sock->s_recv || pie_sock->s_send) {
		sock_release(pie_sock->s_recv);
		sock_release(pie_sock->s_send);

		pie_sock->s_recv = NULL;
		pie_sock->s_send = NULL;

	}

	if (eof == 0) {
		nf_unregister_hook(&pre_hook);
		nf_unregister_hook(&post_hook);
	}

	kfree(pie_sock);
	pie_sock = NULL;

	for (i=0; i<num_events;i++)
		del_timer(&event[i].trigger);

	printk(KERN_INFO "Module unloaded\n");

}

module_init(pie_init);
module_exit(pie_exit);
