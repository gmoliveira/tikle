/* 
 * Tikle kernel module
 * Copyright (C) 2009  Felipe 'ecl' Pena
 *                       Gustavo 'nst' Oliveira
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

/**
 * it depends of faultload size
 */

#define TIKLE_PROCFSBUF_SIZE 128

#define PORT_LISTEN 12608
#define PORT_CONNECT 21508
#define PORT_LOGGING 12128 

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

typedef struct {
	unsigned long start_time;
	int total_packets, accept_packets, reject_packets;
} tikle_log;

#endif /* TIKLE_DEFS_H */
