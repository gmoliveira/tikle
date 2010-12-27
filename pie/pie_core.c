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

#include <linux/netfilter.h> /* kernel protocol staff */
#undef __KERNEL__
#include <linux/netfilter_ipv4.h>

#define PORT_LISTEN 12608
#define PORT_CONNECT 21508
#define PORT_LOGGING 24187

#define NUM_FAULTLOAD_OP 10

#define log_saddr(_i) _i*(line_size) + 1
#define log_event(_i) _i*(line_size) + 2 + 3 * trigger_flag
#define log_in(_i)    _i*(line_size) + 3 + 3 * trigger_flag
#define log_out(_i)   _i*(line_size) + 4 + 3 * trigger_flag

#define SECURE_FREE(_var) \
        if (_var) {           \
                kfree(_var);      \
        }

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

int num_ips, log_size, line_size, active, meuip;
unsigned int num_events = 0;
long unsigned int intervalo = 0;
static unsigned int trigger_flag = 0;
static int foobar=0, boofar=0;
MODULE_AUTHOR("c2zlabs");
MODULE_DESCRIPTION("Partitioning Injection Environment");
MODULE_LICENSE("GPLv3");

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
	struct sk_buff *sb = pskb;
	unsigned long array_local = 0, array_remote = 1;
	int i = 0, array, position;
	struct udphdr *udp;
//	int j;

	udp = (struct udphdr *)(sb->transport_header);
//	printk(KERN_INFO "PORTA %d--%d\n\n", udp->source, udp->dest);

	foobar++;

	if (active == 0) {
//	for (j = 0; j < num_ips; j++) {
//		if (ipip_hdr(sb)->saddr == partition_ips[j]) {
//		//			printk(KERN_INFO "Pacote do IP: %d -- %d\n", ipip_hdr(sb)->saddr, partition_ips[j]);
//			foda=0;
//			break;
//		} else { 
//			printk(KERN_INFO "NAO FAZ PARTE DO EXPERIMENTO: %d -- %d\n", ipip_hdr(sb)->saddr, partition_ips[j]);
//			foda=1;
//		}
//	}

	if (ipip_hdr(sb)->protocol == 17) boofar++;

		if (event[trigger_flag].trigger_state != 1) {
//		printk(KERN_INFO "NAO INICIADO AINDA!\n");
		for (i = 0; log_counters[i+2] && i < log_size; i+=line_size) {
			if (log_counters[i+2] == ipip_hdr(sb)->saddr) {
				printk(KERN_INFO "host encontrado na posicao %d\n", i);
				break;
			} else {
				continue;
//				printk(KERN_INFO "host nao encontrado\n");
		}
		}
		log_counters[i+0] = 0;
		log_counters[i+1] = ipip_hdr(sb)->daddr;
		log_counters[i+2] = ipip_hdr(sb)->saddr;
		log_counters[i+4*trigger_flag+3] = 0;
		log_counters[i+4*trigger_flag+4] = trigger_flag;
		log_counters[i+4*trigger_flag+5] += 1;
		printk(KERN_INFO "gravado 1 " NIPQUAD_FMT " na posicao %d no evento %d\n", NIPQUAD(ipip_hdr(sb)->saddr), i, trigger_flag);

		return NF_ACCEPT;
	}


		for (i = 0; log_counters[i+2] && i < log_size; i+=line_size) {
			if (log_counters[i+2] == ipip_hdr(sb)->saddr) {
				printk(KERN_INFO "host encontrado na posicao %d\n", i);
				break;
			} else {
				continue;
//				printk(KERN_INFO "host nao encontrado\n");
		}
		}
		log_counters[i+0] = 0;
		log_counters[i+1] = ipip_hdr(sb)->daddr;
		log_counters[i+2] = ipip_hdr(sb)->saddr;
		log_counters[i+4*(trigger_flag+1)+3] = 0;
		log_counters[i+4*(trigger_flag+1)+4] = trigger_flag+1;
		log_counters[i+4*(trigger_flag+1)+5] += 1;
		printk(KERN_INFO "gravado 2 " NIPQUAD_FMT " na posicao %d no evento %d\n", NIPQUAD(ipip_hdr(sb)->saddr), i, trigger_flag);
//
//
//		if (udp->dest == 45588) foobar++;
//		printk(KERN_INFO "hooking\n");

		
		i = event[trigger_flag].trigger_id;
		do {
			/*
			 * opcode handlers
			 */
			switch (faultload[i].opcode) {
				case PARTITION:
					/*
					 * loop into partitions to identify allowed communications
					 */
					for (array = 0; array < faultload[i].num_ops; array++) {
						for (position = 0; position < faultload[i].op_value[array].array.count; position++) {
							if (faultload[i].op_value[array].array.nums[position] == ipip_hdr(sb)->saddr) {
//								printk(KERN_INFO "REMOTO: %ld -- VERIFICADO COM: %ld -- ARRAY: %d\n", ipip_hdr(sb)->saddr, faultload[i].op_value[array].array.nums[position], array);
								array_remote = array;
//							} else if (faultload[i].op_value[array].array.nums[position] == pie_sock->addr_recv.sin_addr.s_addr) {
							} else if (faultload[i].op_value[array].array.nums[position] == meuip) {

//								printk(KERN_INFO "LOCAL: %ld -- VERIFICADO COM: %ld -- ARRAY: %d\n", meuip, faultload[i].op_value[array].array.nums[position], array);
								array_local = array;
							}
						}
					}
					if (array_local == array_remote) {
//						printk(KERN_INFO "Evento: %u --> " NIPQUAD_FMT " Aceito\n", trigger_flag, NIPQUAD(ipip_hdr(sb)->saddr));
						return NF_ACCEPT;
					} else {
//						printk(KERN_INFO "Evento: %u --> " NIPQUAD_FMT " Rejeitado\n", trigger_flag, NIPQUAD(ipip_hdr(sb)->saddr));
//						printk(KERN_INFO "Descartado IP: %ld -- Evento %d\n", ipip_hdr(sb)->saddr, trigger_flag);
						return NF_DROP;
					}
					break;
				default:
					break;
			}
		} while (++i < faultload[event[trigger_flag].trigger_id].next_op);
	} //else
//		printk(KERN_INFO "not hooking\n");
	return NF_ACCEPT;
}

unsigned int f_post_hook(unsigned int hooknum,
		struct sk_buff *pskb,
		const struct net_device *indev,
		const struct net_device *outdev,
		int (*okfn)(struct sk_buff *))
{
	int i;
	struct iphdr *iph;
	struct sk_buff *sb = pskb;

	iph = (struct iphdr *)(sb->network_header);

	foobar++;


	if (active == 0) {

	if (ipip_hdr(sb)->protocol == 17) boofar++;
		printk(KERN_INFO "hooking out\n");

		if (event[trigger_flag].trigger_state != 1) {
//		printk(KERN_INFO "NAO INICIADO AINDA!\n");
		for (i = 0; log_counters[i+2] && i < log_size; i+=line_size) {
			if (log_counters[i+2] == iph->daddr) {
				printk(KERN_INFO "host encontrado na posicao %d\n", i);
				break;
			} else {
				continue;
//				printk(KERN_INFO "host nao encontrado\n");
		}
		}
		log_counters[i+0] = 0;
		log_counters[i+1] = iph->saddr;
		log_counters[i+2] = iph->daddr;
		log_counters[i+4*trigger_flag+3] = 0;
		log_counters[i+4*trigger_flag+4] = trigger_flag;
		log_counters[i+4*trigger_flag+6] += 1;
		printk(KERN_INFO "out 1 gravado " NIPQUAD_FMT " na posicao %d no evento %d\n", NIPQUAD(iph->daddr), i, trigger_flag);

		return NF_ACCEPT;
	}


		for (i = 0; log_counters[i+2] && i < log_size; i+=line_size) {
			if (log_counters[i+2] == iph->daddr) {
				printk(KERN_INFO "host encontrado na posicao %d\n", i);
				break;
			} else {
				continue;
//				printk(KERN_INFO "host nao encontrado\n");
		}
		}

		log_counters[i+0] = 0;
		log_counters[i+1] = iph->saddr;
		log_counters[i+2] = iph->daddr;
		log_counters[i+4*(trigger_flag+1)+3] = 0;
		log_counters[i+4*(trigger_flag+1)+4] = trigger_flag+1;
		log_counters[i+4*(trigger_flag+1)+6] += 1;
		printk(KERN_INFO "out 2 gravado " NIPQUAD_FMT " na posicao %d no evento %d\n", NIPQUAD(iph->daddr), i, trigger_flag);

	}// else {
//		printk(KERN_INFO "not hooking out\n");
//	}
	return NF_ACCEPT;
}

int log_finalize(void)
{
	int err, i;
	struct socket *sock_log;
	struct sockaddr_in addr_log;

	if ((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_log)) < 0)
		printk(KERN_INFO "error whilie log socket creation. erro: %d\n", err);

	memset(&addr_log, 0, sizeof(struct sockaddr));
	addr_log.sin_family = AF_INET;
	addr_log.sin_addr.s_addr = pie_sock->addr_recv.sin_addr.s_addr;
	addr_log.sin_port = htons(PORT_LOGGING);

	if ((err = sock_log->ops->connect(sock_log, (struct sockaddr *)&addr_log, sizeof(struct sockaddr), 0)) < 0) {
		printk(KERN_INFO "error while connecting socket. erro: %d\n", err);
		sock_release(sock_log);
		sock_log = NULL;
	}
//	send_msg(sock_log, &addr_log, log_counters, sizeof(unsigned long) * log_size);
	printk("enviando logs\n");
	send_msg(sock_log, &addr_log, log_counters, sizeof(unsigned long)*log_size);
	return 0;
}

int flag_handler(unsigned long id)
{
	/*
	 * set the previous timer as 'killed'
	 * and update the system flags with
	 * the running trigger informations
	 */
	if (id > 0 && event[trigger_flag].trigger_state == 1) {
		event[trigger_flag].trigger_state = 2;
		printk(KERN_INFO "killing timer %d\n", trigger_flag);

		/*
		 * go to next timer
		 */
		trigger_flag++;
	}
//	printk(KERN_INFO "jiffies: %lu/%u ms / %lu secs -- activating timer %d \n", jiffies, jiffies_to_msecs(jiffies), jiffies/HZ, trigger_flag);
	printk(KERN_INFO "%lu -- intervalo: %lu jiffies / %u ms / %lu secs -- activating_timer: %d - Pacotes_rede/Pacotes_app: %d/%d\n", jiffies, jiffies-intervalo, jiffies_to_msecs((jiffies-intervalo)), (jiffies-intervalo)/HZ, trigger_flag, foobar, boofar);
	intervalo = jiffies;
	/*
	 * set trigger state to active
	 */
	event[trigger_flag].trigger_state = 1;

	if (id == event[trigger_flag].trigger_id) {
		active = 1;
		log_finalize();
	}

	return 0;
}

/*
 * Setup trigger handling
 * @return void
	 */
int event_handler(void)
{
	unsigned int i = 0, base;

	/*
	 * setting up triggers
	 */
	printk(KERN_INFO "Triggers:\n");

	for (; i < num_events; i++) {
		init_timer(&event[i].trigger);

		event[i].trigger.function = flag_handler;

		/*
		 * send the next trigger ID, if actual trigger it is
		 * not the latest, else send your own trigger ID
		 */
		if (event[i].trigger_id < event[num_events-1].trigger_id)
			event[i].trigger.data = event[i+1].trigger_id;
		else
			event[i].trigger.data = event[i].trigger_id;

		printk(KERN_INFO "Registering trigger %d; id=%d; data=%ld\n", i, event[i].trigger_id, event[i].trigger.data);

		/*
		 * calculate expire time to trigger
		 * based on the experiment starting time
		 */
		base = jiffies;
		event[i].trigger.expires = base + msecs_to_jiffies(faultload[event[i].trigger_id].op_value[0].num * 1000);

		/*
		 * if timer corresponds to the first timer
		 */
		if (event[i].trigger_id == 0) {
			/*
			 * register hook
			 */
			pre_hook.hook = &f_pre_hook;
			pre_hook.hooknum = NF_IP_PRE_ROUTING;
			pre_hook.pf = PF_INET;
			pre_hook.priority = NF_IP_PRI_FIRST;

			post_hook.hook = &f_post_hook;
			post_hook.hooknum = NF_IP_POST_ROUTING;
			post_hook.pf = PF_INET;
			post_hook.priority = NF_IP_PRI_FIRST;

		/*
		 * if timer corresponds
		 * to the last timer
		 */
		}// else if (event[i].trigger_id == event[num_events-1].trigger_id) {
			
//		}
		add_timer(&event[i].trigger);
	}
	printk(KERN_INFO "---------------------------\n");

	nf_register_hook(&pre_hook);
	nf_register_hook(&post_hook);
	eof = 0;
	active = 0;
	intervalo = jiffies;
	printk(KERN_INFO "activating_timer jiffie inicial: %lu\n", jiffies);
	return 0;
}

static int f_main_sock(void)
{
	int err, size = -1, trigger_count = 0, i = 0;
	static int count = 0;
	char auth_msg[] = { 'p', 'i', 'e', ':', 's', 't', 'a', 'r', 't' };

	/*
	 * define flags of the thread and run socket
	 * server as a daemon (run in background)
	 */
	lock_kernel();
	pie_sock->flag = 1;
	current->flags |= PF_NOFREEZE;
	daemonize("thread_sock");
	allow_signal(SIGKILL);
	unlock_kernel();

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

	size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &meuip, sizeof(int));
//	printk(KERN_INFO "MEU IPPPPPPP: %ld\n\n", meuip);

	size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, &num_ips, sizeof(int));

	if (size < 0) {
		printk(KERN_INFO "error %d while getting datagram\n", size);
		eof = 1;
		return size;
	} else {
		printk(KERN_INFO "num_ips = %d\n", num_ips);
	}
	if (num_ips) {
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
					
					faultload[i].op_type = kcalloc(faultload[i].num_ops, sizeof(faultload_op_type), GFP_KERNEL | GFP_ATOMIC);


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
						
					faultload[i].op_value = kcalloc(faultload[i].num_ops, sizeof(faultload_value_type), GFP_KERNEL | GFP_ATOMIC);
					size = 0;
					for (k = 0; k < faultload[i].num_ops; k++) {
						switch (faultload[i].op_type[k]) {
							case STRING:
								recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
									&faultload[i].op_value[k].str.length, sizeof(size_t));
								faultload[i].op_value[k].str.value = (char*) kmalloc(faultload[i].op_value[k].str.length, GFP_KERNEL | GFP_ATOMIC);
								size += recv_msg(pie_sock->s_recv, &pie_sock->addr_recv,
								faultload[i].op_value[k].str.value, faultload[i].op_value[k].str.length);
								
								printk(KERN_INFO "%d string [%s] (len: %d)\n", k+1, faultload[i].op_value[k].str.value,
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
			printk(KERN_INFO "signal_peding returned true\n");
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

	/*
	 * get the controller ip address
	 */
	pie_sock->addr_send.sin_addr.s_addr = pie_sock->addr_recv.sin_addr.s_addr;
	/*
	 * sending confirmation of data received
	 * (for control protocol stuffs)
	 */
	if ((err = pie_sock->s_send->ops->connect(pie_sock->s_send,
			(struct sockaddr *)&pie_sock->addr_send, sizeof(struct sockaddr), 0)) < 0)
	{
		printk(KERN_INFO "error %d while connecting to socket\n", -err);
		sock_release(pie_sock->s_send);
		pie_sock->s_send = NULL;
	}

	printk(KERN_INFO "sending confirmation of received data\n");

	send_msg(pie_sock->s_send, &pie_sock->addr_send, "pie:received", sizeof("pie:received"));

//	/*
//	 * waiting for controller
//	 * authorization to start
//	 */
	printk(KERN_INFO "waiting for authorization to start\n");


//	size = send_msg(pie_sock->s_recv, &pie_sock->addr_recv, auth_msg, sizeof("pie:start"));
	size = recv_msg(pie_sock->s_recv, &pie_sock->addr_recv, auth_msg, sizeof("pie:start"));

	printk(KERN_INFO "received \"%s\" from controller\n", auth_msg);

	if (size < 0) {
		printk(KERN_INFO "error %d while getting authorization\n", size);
		eof = 1;
		free_faultload();
		return 0;
	} else if (strncmp(auth_msg, "pie:start", sizeof("pie:start")) == 0) { 
		printk(KERN_INFO "received permission to start execution\n");
	
		event = kmalloc(trigger_count * sizeof(struct timer_event), GFP_KERNEL | GFP_ATOMIC);

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
		
		if (num_ips) {
			line_size = 4 * (num_events) + 3;
			log_size = (num_ips-1) * line_size;
			log_counters = kcalloc(log_size, sizeof(unsigned long), GFP_KERNEL | GFP_ATOMIC);
		}
		event_handler();
	} else { 
		printk(KERN_INFO "you don't have permission to start execution. received: '%s'\n", auth_msg);
		eof = 1;
	}
	return 0;
}


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

	/*
	 * preparing thread to
	 * init socket stuff
	 */
	pie_sock = kmalloc(sizeof(struct data_listen), GFP_KERNEL);
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
	/*
	 * cleanup memory/socket
	 */
	int i;
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
//	printk(KERN_INFO "Total de Pacotes na Rede: %d\n", foobar);
//	printk(KERN_INFO "Total de Pacotes da aplicação: %d\n", boofar);
	for (i=0;i < log_size; i++) {
		printk(KERN_INFO "%lu\n", log_counters[i]);
	}
	printk(KERN_INFO "Module unloaded\n");

}

module_init(pie_init);
module_exit(pie_exit);
