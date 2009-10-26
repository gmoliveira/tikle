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

#ifndef TIKLE_DEFS_H
#define TIKLE_DEFS_H

#undef TDEBUG
#ifdef TIKLE_DEBUG
# define TDEBUG(fmt, ...) printk(KERN_DEBUG "tikle debug: " fmt, ##__VA_ARGS__)
#else
# define TDEBUG(fmt, ...) /* nothing */
#endif

#define TERROR(fmt, ...) printk(KERN_ERR "tikle error: " fmt, ##__VA_ARGS__)
#define TALERT(fmt, ...) printk(KERN_NOTICE "tikle alert: " fmt, ##__VA_ARGS__)
#define TINFO(fmt, ...) printk(KERN_INFO "tikle info: " fmt, ##__VA_ARGS__)

#include <linux/ip.h> /* ipip_hdr(const struct sk_buff *sb) */
#include <linux/in.h> /* sockaddr_in and other macros */
#include "tikle_types.h" /* tikle type definitions */

/**
 * it depends of faultload size
 */
#define TIKLE_PROCFSBUF_SIZE 128

/**
 * Ports
 */
#define PORT_LISTEN 12608
#define PORT_CONNECT 21508
#define PORT_LOGGING 12128 
#define PORT_COMMAND 24187 

/**
 * Number of itens in opcode structure
 */
#define NUM_FAULTLOAD_OP 10

/**
 * Helper for secure kfree calls
 */
#define SECURE_FREE(_var) \
	if (_var) {           \
		kfree(_var);      \
	}

extern unsigned long *tikle_log_counters;
extern int num_ips, log_size;

/**
 * socket structure
 */
struct tikle_sockudp {
	int flag;
	struct task_struct *thread_socket, *thread_command;
	struct socket *sock_recv, *sock_send, *sock_log, *sock_command_recv, *sock_command_send;
	struct sockaddr_in addr_recv, addr_send, addr_log, addr_command_recv, addr_command_send;
};

extern struct tikle_sockudp *tikle_comm;

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

extern struct tikle_timer *tikle_timers;

/**
 * Register trigger timers
 */
extern void tikle_trigger_handling(void);

/**
 * Control flags
 */
extern unsigned int tikle_num_timers, tikle_trigger_flag;

/**
 * Faultload
 */
extern faultload_op faultload[30];

/**
 * Ips used in partition experiment
 */
extern unsigned long partition_ips[30];

/**
 * Struct to register hook operations
 */
extern struct nf_hook_ops tikle_pre_hook, tikle_post_hook;

/**
 * Socket wrappers
 */
extern int tikle_sockudp_send(struct socket *sock,	struct sockaddr_in *addr, void *buf, int len);
extern int tikle_sockudp_recv(struct socket *sock, struct sockaddr_in *addr, void *buf, int len);

#endif /* TIKLE_DEFS_H */
