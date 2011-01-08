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

#include <linux/err.h>

#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS and others */
#include <linux/smp_lock.h> /* (un)lock_kernel(); */
#include "low_defs.h" /* tikle global definitions */

//#include <linux/ip.h> /* ipip_hdr(const struct sk_buff *sb) */
#include <linux/in.h> /* sockaddr_in and other macros */

unsigned long partition_ips[30];
faultload_op faultload[30];

/**
 * Receive data from socket.
 * @param sock the socket
 * @param addr the host receiving data
 * @param buf the buff to write in
 * @param len the lenght of the buffer
 * @return the number of bytes read.
 * @sa tikle_sockdup_send()
 */
int f_recv_msg(struct socket *sock,
		struct sockaddr_in *addr,
		void *buf,
		int len)
{
	int size = 0;
	struct iovec iov;
	struct msghdr msg;
	mm_segment_t oldfs;

	if (sock->sk == NULL)
		return 0;

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
 * @sa tikle_sockdup_recv()
 */
int f_send_msg(struct socket *sock,
		struct sockaddr_in *addr,
		void *buf, int len)
{
	int size = 0;
	struct iovec iov;
	struct msghdr msg;
	mm_segment_t oldfs;

	if (sock->sk == NULL)
		return 0;

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
 * function to get the user faultload
 */
int f_get_faultload(cfg_lsock_t *listen)
{
	int num_ips, size, i = 0;//, trigger_count = 0;
	static int count = 0;

	printk(KERN_INFO "- start------------------------------------\n");

	memset(faultload, 0, sizeof(faultload));
	
	size = f_recv_msg(listen->sock, &listen->addr, &num_ips, sizeof(int));
	
	if (size < 0) {
		printk(KERN_ERR "error %d while getting datagram\n", size);
	} else {
		printk(KERN_INFO "num_ips=%d\n", num_ips);
		printk(KERN_INFO "received %d bytes\n", size);
	}
	if (num_ips) {
		size = f_recv_msg(listen->sock, &listen->addr,
			&partition_ips, sizeof(unsigned long) * num_ips);
	
		if (size < 0) {
			printk("error %d while getting datagram\n", size);
		} else {
			int j;
			
			printk("declare={");
			for (j = 0; j < num_ips; j++) {
				printk("%lu%s", partition_ips[j], (j == (num_ips-1) ? "" : ","));
			}
			printk("}\n");
			
			printk("received %d bytes\n", size);
		}
	}

	while (1) {
		switch (count) {
			case 0: /* Opcode */
				size = f_recv_msg(listen->sock, &listen->addr,
					&faultload[i].opcode, sizeof(faultload_opcode));
//				if (faultload[i].opcode == AFTER) {
//					trigger_count++;
//				}
				printk("opcode: %d\n", faultload[i].opcode);
				break;
			case 1: /* Protocol */
				size = f_recv_msg(listen->sock, &listen->addr,
					&faultload[i].protocol, sizeof(faultload_protocol));
				printk("protocol: %d\n", faultload[i].protocol);
				break;
			case 2: /* Occur type (TEMPORAL, PACKETS, PERCT) */
				size = f_recv_msg(listen->sock, &listen->addr,
					&faultload[i].occur_type, sizeof(faultload_num_type));
				printk("occur_type: %d\n", faultload[i].occur_type);
				break;
			case 3: /* Number of ops */
				size = f_recv_msg(listen->sock, &listen->addr,
					&faultload[i].num_ops, sizeof(unsigned short int));
				printk("num_ops: %d\n", faultload[i].num_ops);
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

					size = f_recv_msg(listen->sock, &listen->addr,
						faultload[i].op_type, sizeof(faultload_op_type) * faultload[i].num_ops);

					for (k = 0; k < faultload[i].num_ops; k++) {
						printk("op_type%d [%d]\n", k+1, faultload[i].op_type[k]);
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
								f_recv_msg(listen->sock, &listen->addr,
									&faultload[i].op_value[k].str.length, sizeof(size_t));
								faultload[i].op_value[k].str.value = (char*) kmalloc(faultload[i].op_value[k].str.length, GFP_KERNEL | GFP_ATOMIC);
								size += f_recv_msg(listen->sock, &listen->addr,
								faultload[i].op_value[k].str.value, faultload[i].op_value[k].str.length);
								
								printk("%d string [%s] (len: %d)\n", k+1, faultload[i].op_value[k].str.value,
									faultload[i].op_value[k].str.length);
								break;
							case ARRAY:
								{
									unsigned int j;
									
									f_recv_msg(listen->sock, &listen->addr,
										&faultload[i].op_value[k].array.count, sizeof(size_t));
									size += f_recv_msg(listen->sock, &listen->addr,
										&faultload[i].op_value[k].array.nums, sizeof(unsigned long) * faultload[i].op_value[k].array.count);
									
									printk("%d array {", k+1);
									for (j = 0; j < faultload[i].op_value[k].array.count; j++) {
										printk("%ld%s", faultload[i].op_value[k].array.nums[j],
											(j == (faultload[i].op_value[k].array.count-1) ? "" : ","));
									}
									printk("}\n");
								}
								break;
							default:
								size += f_recv_msg(listen->sock, &listen->addr,
									&faultload[i].op_value[k].num, sizeof(unsigned long));
									
								printk("%d number: %ld\n", k+1, faultload[i].op_value[k].num);
								break;
						}
					}
				}
				break;
			case 6: /* Extended value */
				size = f_recv_msg(listen->sock, &listen->addr,
							&faultload[i].extended_value, sizeof(int));
				printk("Extended value: %d\n", faultload[i].extended_value);
				break;
			case 7: /* Label */
				size = f_recv_msg(listen->sock, &listen->addr,
							&faultload[i].label, sizeof(unsigned long));
				printk("Label: %ld\n", faultload[i].label);
				break;
			case 8: /* Block type (START, STOP) */
				size = f_recv_msg(listen->sock,
							&listen->addr, &faultload[i].block_type,
							sizeof(short int));
				printk("Block type: %d\n", faultload[i].block_type);
				break;
			case 9: /* Control op number */
				size = f_recv_msg(listen->sock, &listen->addr,
							&faultload[i].next_op, sizeof(unsigned int));
				printk("Op number: %d\n", faultload[i].next_op);
				break;
			default:
				printk("unexpected type\n");
				break;
		}
		
		if (signal_pending(current)) {
			printk("signal_peding returned true\n");
			goto next;
		}
		
		if (size < 0) {
			printk("error %d while getting datagram\n", size);
		} else {				
			printk("received %d bytes\n", size);
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
	printk("- end------------------------------------\n");

	return 0;
}

/**
 * create a client socket during the configuration phase
 */
cfg_lsock_t *f_create_sock_client(int port)
{
	int err;

	cfg_lsock_t *temp = kmalloc(sizeof(cfg_lsock_t), GFP_KERNEL);

	err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &temp->sock);
	if (err < 0)
		return temp;

	memset(&temp->addr, 0, sizeof(struct sockaddr));
	temp->addr.sin_family = AF_INET;
	temp->addr.sin_port = htons(21508);

	return temp;
}

/**
 * create a server socket during the configuration phase
 */
cfg_lsock_t *f_create_sock_server(int port)
{
	int err;
	cfg_lsock_t *temp = kmalloc(sizeof(cfg_lsock_t), GFP_KERNEL);

	err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &temp->sock);

	if (err < 0)
		return temp;

	memset(&temp->addr, 0, sizeof(struct sockaddr));
	temp->addr.sin_family = AF_INET;
	temp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	temp->addr.sin_port = htons(12608);

	err = temp->sock->ops->bind(temp->sock, (struct sockaddr *)&temp->addr, sizeof(struct sockaddr));
	if (err < 0) {
		sock_release(temp->sock);
		temp->sock = NULL;
	}

	printk(KERN_INFO "listening on port %d\n", port);

	return temp;
}

/**
 * handles the configuration phase by creating a socket listener,
 * for receiving the user faultload, and a sender socket to confirm
 * messages
 */
int f_config(void)
{
//	int flag;
	int err;
//	usr_args_t *data;
	cfg_lsock_t *listen;
	cfg_lsock_t *send;
	/*
	 * define flags of the thread and run socket
	 * server as a daemon (run in background)
	 */
	lock_kernel();
	current->flags |= PF_NOFREEZE;
	daemonize("main thread");
	allow_signal(SIGKILL);
	unlock_kernel();

	/**
	 * sock listener creation
	 */
	listen = f_create_sock_server(12608);
	if (listen == NULL) {
		kfree(listen);
		goto error;
	}

	/**
	 * sock sender creation
	 */
	send = f_create_sock_client(21508);
	if (send == NULL) {
		kfree(send);
		goto error;
	}

	/**
	 * all gone well, let`s receive the user faultload
	 */
//	faultload = f_get_faultload(listen);

	/**
	 * we need just two more info data
	 * the total of hosts, as its ips
	 */
//	hosts = f_get_faultload_extra(listen);


	return 0;

error:
	return 1;
}
