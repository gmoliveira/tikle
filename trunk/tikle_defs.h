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

#include <linux/ip.h> /* ipip_hdr(const struct sk_buff *sb) */
#include <linux/in.h> /* sockaddr_in and other macros */

#include "tikle_types.h"

/**
 * it depends of faultload size
 */

#define TIKLE_PROCFSBUF_SIZE 128

#define PORT_LISTEN 12608
#define PORT_CONNECT 21508
#define PORT_LOGGING 12128 
#define PORT_HALTING 24187 

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
	
#define TAUSWORTHE(s,a,b,c,d) ((s&c)<<d) ^ (((s <<a) ^ s)>>b)
#define LCG(n) (69069 * n)

#define tikle_log_daddr(_i) _i*5
#define tikle_log_saddr(_i) _i*5+1
#define tikle_log_event(_i) _i*5+2
#define tikle_log_in(_i)    _i*5+3
#define tikle_log_out(_i)   _i*5+4

extern unsigned long *tikle_log_counters;

extern int num_ips, log_size;

/**
 * socket structure
 */
struct tikle_sockudp {
	int flag;
	struct task_struct *thread, *thread_halt;
	struct socket *sock_recv, *sock_send, *sock_log, *sock_halt;
	struct sockaddr_in addr_recv, addr_send, addr_log, addr_halt;
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
 * Control flags
 */
extern unsigned int tikle_num_timers, tikle_trigger_flag;

/**
 * Faultload
 */
extern faultload_op faultload[30];

extern void tikle_trigger_handling(void);
void tikle_flag_handling(unsigned long id);

/**
 * Struct to register hook operations
 */
extern struct nf_hook_ops tikle_pre_hook, tikle_post_hook;

extern int tikle_sockudp_send(struct socket *sock,	struct sockaddr_in *addr, void *buf, int len);
extern int tikle_sockudp_recv(struct socket *sock, struct sockaddr_in *addr, void *buf, int len);

#endif /* TIKLE_DEFS_H */
