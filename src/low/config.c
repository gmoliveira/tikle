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
#include <linux/smp_lock.h> /* (un)lock_kernel(); */
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
 * function to get the user faultload
 */
faultload_op *f_get_faultload(cfg_lsock_t *listen)
{
	int num_ips, size;
	unsigned long partition_ips[30];
	faultload_op *temp = kmalloc(sizeof(faultload_op), GFP_KERNEL);

	printk(KERN_INFO "- start------------------------------------\n");

	memset(temp, 0, sizeof(temp));

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
                        printk(KERN_INFO "error %d while getting datagram\n", size);
                        return NULL;
                } else {
                        int j;

                        printk(KERN_INFO "declare={");
                        for (j = 0; j < num_ips; j++)
                                printk(KERN_INFO "%lu%s", partition_ips[j], (j == (num_ips-1) ? "" : ","));
                        printk("}\n");
                }
        }
	printk(KERN_INFO "- end------------------------------------\n");

	return temp;
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
	cfg_lsock_t *listen;
	cfg_lsock_t *send;

	faultload_op *faultload;

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
	faultload = f_get_faultload(listen);

	/**
	 * we need just two more info data
	 * the total of hosts, as its ips
	 */
//	hosts = f_get_faultload_extra(listen);


	return 0;

error:
	return 1;
}
