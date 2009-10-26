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

#include <linux/kernel.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netfilter.h> /* kernel protocol stack */
#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS an others */
#include <linux/ip.h> /* ipip_hdr(const struct sk_buff *sb) */
#include "tikle_hooks.h"
#include "tikle_defs.h"

struct tikle_sockudp *tikle_comm = NULL;
faultload_op faultload[30];
unsigned int tikle_num_timers = 0, tikle_trigger_flag = 0;

#ifdef TIKLE_DEBUG
static const char *op_names[] = { 
	"COMMAND", "AFTER", "WHILE", "HOST", "IF", "ELSE",
	"END_IF", "END", "SET", "FOREACH", "PARTITION", "DECLARE"
};
#endif

/**
 * Check if the ip is in the list
 */
static inline int tikle_ip_check(unsigned long ip, unsigned long *ip_list)
{
	register int i = 0;
	
	for (; ip_list[i]; i++) {
		if (ip == ip_list[i]) {
			return 1;
		}
	}	
	return 0;
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
unsigned int tikle_pre_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sb = pskb; /*, *duplicate_sb;*/
	unsigned long array_local = 0, array_remote = 1;
	int i = 0, array, position, log_found_flag = -1;

	TDEBUG("hook function called\n");
	
	if (num_ips && tikle_ip_check(ipip_hdr(sb)->saddr, partition_ips) == 0) {
		return NF_DROP;
	}

	/*
	 * log counters dummy version
	 */
	for (; i < log_size && tikle_log_counters[tikle_log_daddr(i)]; i++) {
		if (tikle_log_counters[tikle_log_daddr(i)] == tikle_comm->addr_recv.sin_addr.s_addr &&
			tikle_log_counters[tikle_log_saddr(i)] == ipip_hdr(sb)->saddr &&
			tikle_log_counters[tikle_log_event(i)] == tikle_trigger_flag) {
			log_found_flag = i;
			break;
		}
	}
		
	/*
	 * update log informations
	 */
	if (log_found_flag < 0) {
		if (i > 0) {
			i--;
		}
		printk(KERN_INFO "IP: " NIPQUAD_FMT "\n\n", NIPQUAD(ipip_hdr(sb)->saddr));
		
		tikle_log_counters[tikle_log_daddr(i)] = tikle_comm->addr_recv.sin_addr.s_addr;
		tikle_log_counters[tikle_log_saddr(i)] = ipip_hdr(sb)->saddr;
		tikle_log_counters[tikle_log_event(i)] = tikle_trigger_flag;
	}
	
	tikle_log_counters[tikle_log_in(i)]++;

	TDEBUG("- tikle pre counters: %lu\n", tikle_log_counters[tikle_log_in(i)]);

	TDEBUG("INCOMING @ remetente: " NIPQUAD_FMT " @ destinatario: " NIPQUAD_FMT " @ protocolo: %d\n",
				NIPQUAD(ipip_hdr(sb)->saddr), NIPQUAD(ipip_hdr(sb)->daddr), ipip_hdr(sb)->protocol);
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
		TDEBUG("i=%d ; opcode: %d (%s) ; next_op=%d\n",
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
				if (faultload[i].protocol == ipip_hdr(sb)->protocol) {
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
						if (faultload[i].op_value[array].array.nums[position] == ipip_hdr(sb)->saddr) {
							array_remote = array;
						} else if (faultload[i].op_value[array].array.nums[position] == tikle_comm->addr_recv.sin_addr.s_addr) {
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

unsigned int tikle_post_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *))
{
	struct sk_buff *sb = pskb;
	int i = 0, log_found_flag = -1;

	/*
	 * log counters dummy version
	 */
	for (; i < log_size && tikle_log_counters[tikle_log_daddr(i)]; i++) {
		if (tikle_log_counters[tikle_log_daddr(i)] ==  tikle_comm->addr_recv.sin_addr.s_addr &&
			tikle_log_counters[tikle_log_saddr(i)] == ipip_hdr(sb)->saddr &&
			tikle_log_counters[tikle_log_event(i)] == tikle_trigger_flag) {
			log_found_flag = i;
			break;
		}
	}
		
	/*
	 * updating log informations
	 */
	if (log_found_flag < 0) {
		if (i > 0) {
			i--;
		}
		tikle_log_counters[tikle_log_daddr(i)] = tikle_comm->addr_recv.sin_addr.s_addr;
		tikle_log_counters[tikle_log_saddr(i)] = ipip_hdr(sb)->saddr;
		tikle_log_counters[tikle_log_event(i)] = tikle_trigger_flag;
	}

	tikle_log_counters[tikle_log_out(i)]++;

	TDEBUG("- tikle post counters: %lu\n", tikle_log_counters[tikle_log_out(i)]);

	TDEBUG("OUTCOMING @ remetente: " NIPQUAD_FMT " @ destinatario: " NIPQUAD_FMT " @ protocolo: %d\n",
				NIPQUAD(ipip_hdr(sb)->saddr), NIPQUAD(ipip_hdr(sb)->daddr), ipip_hdr(sb)->protocol);

	return NF_ACCEPT;
}
