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

#include <linux/err.h> /* error control through errno */

#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS and others */
#include <linux/in.h> /* sockaddr_in and other macros */

#include "low_defs.h" /* tikle global definitions */

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
 * function to get the user faultload extra infos
 */
int f_get_faultload_extra(cfg_sock_t *listen)
{
	int size;
	faultload_extra_t *extra = kmalloc(sizeof(faultload_extra_t), GFP_KERNEL);
	size = f_recv_msg(listen->sock, &listen->addr, &extra->num_ips, sizeof(int));
	printk(KERN_INFO "Size: %d -- Data: %d\n", size, extra->num_ips);

	size = f_recv_msg(listen->sock, &listen->addr, &extra->partition_ips, sizeof(unsigned long) * extra->num_ips);

	return 0;
}

/**
 * function to get the user faultload
 */
int f_get_faultload(cfg_sock_t *listen)
{
	static int count = 0;
	int size, i = 0, eof = 1;
	cfg_sock_t *call = listen;
	faultload_op faultload[30];
	while (1) {
		switch (count) {
			case 0: /* Opcode */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].opcode, sizeof(faultload_opcode));
				break;
			case 1: /* Protocol */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].protocol, sizeof(faultload_protocol));
				break;
			case 2: /* Occur type (TEMPORAL, PACKETS, PERCT) */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].occur_type, sizeof(faultload_num_type));
				break;
			case 3: /* Number of ops */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].num_ops, sizeof(unsigned short int));
				break;
			case 4: /* Operand types */
				{
					if (faultload[i].num_ops == 0) {
						faultload[i].op_type = NULL;
						faultload[i].op_value = NULL;
						count += 2;
						continue;
					}
					faultload[i].op_type = kcalloc(faultload[i].num_ops, sizeof(faultload_op_type), GFP_KERNEL | GFP_ATOMIC);
					size = f_recv_msg(call->sock, &call->addr, &faultload[i].op_type, sizeof(faultload_op_type) * faultload[i].num_ops);
				}
				break;
			case 5: /* Operand valyues */
				{
					unsigned short int k;
					faultload[i].op_value = kcalloc(faultload[i].num_ops, sizeof(faultload_value_type), GFP_KERNEL | GFP_ATOMIC);
					size = 0;
					for (k = 0; k < faultload[i].num_ops; k++) {
						switch (faultload[i].op_type[k]) {
							case STRING:
								f_recv_msg(call->sock, &call->addr, &faultload[i].op_value[k].str.length, sizeof(size_t));
								faultload[i].op_value[k].str.value = (char *) kmalloc(faultload[i].op_value[k].str.length, GFP_KERNEL | GFP_ATOMIC);
								size += f_recv_msg(call->sock, &call->addr, &faultload[i].op_value[k].str.value, faultload[i].op_value[k].str.length);
								break;
							case ARRAY:
								{
								f_recv_msg(call->sock, &call->addr, &faultload[i].op_value[k].array.count, sizeof(size_t));
								size += f_recv_msg(call->sock, &call->addr, &faultload[i].op_value[k].array.nums, sizeof(unsigned long) * faultload[i].op_value[k].array.count);
								}
								break;
							default:
								size += f_recv_msg(call->sock, &call->addr, &faultload[i].op_value[k].num, sizeof(unsigned long));
								break;
						}
					}
				}
				break;
			case 6: /* Extended value */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].extended_value, sizeof(int));
				break;
			case 7: /* Label */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].label, sizeof(unsigned long));
				break;
			case 8: /* Block type (START, STOP) */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].block_type, sizeof(short int));
				break;
			case 9: /* Control op number */
				size = f_recv_msg(call->sock, &call->addr, &faultload[i].next_op, sizeof(unsigned int));
				break;
			default:
				break;

		}
		if (signal_pending(current)) {
			goto next;
		}

		if (size < 0) {
			eof = 1;
			return size;
		}
next:
		if ((count + 1) == NUM_FAULTLOAD_OP) {
			if (faultload[i].block_type == 1) {
				break;
			}
			count = -1;
			i++;
		}
		count++;
	}
	return 0;
}

/**
 * create a client socket during the configuration phase
 */
cfg_sock_t *f_create_sock_client(int port)
{
	int err;

	cfg_sock_t *temp = kmalloc(sizeof(cfg_sock_t), GFP_KERNEL);

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
cfg_sock_t *f_create_sock_server(int port)
{
	int err;
	cfg_sock_t *temp = kmalloc(sizeof(cfg_sock_t), GFP_KERNEL);

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
	int err;

	cfg_sock_t *listen;
	cfg_sock_t *send;

	/*
	 * define flags of the thread and run socket
	 * server as a daemon (run in background)
	 */
	f_lock_tikle();
	current->flags |= PF_NOFREEZE;
	daemonize("main thread");
	allow_signal(SIGKILL);
	f_unlock_tikle();

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
	 * we need just two main info data
	 * the total of hosts, as its ips
	 */
	err = f_get_faultload_extra(listen);

	/**
	 * all gone well, let`s receive the user faultload
	 */
//	f_get_faultload(listen);

	sock_release(listen->sock);
	sock_release(send->sock);
	kfree(listen);
	kfree(send);

	return 0;

error:
	return 1;
}
