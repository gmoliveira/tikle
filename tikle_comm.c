#include <linux/kernel.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netfilter.h> /* kernel protocol stack */
#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS an others */
#include <linux/proc_fs.h> /* procfs manipulation */
#include <net/net_namespace.h> /* proc_net stuff */
#include <linux/sched.h> /* current-euid() */
#include "tikle_defs.h"

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
int tikle_sockudp_recv(struct socket *sock,
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
int tikle_sockudp_send(struct socket *sock,
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
