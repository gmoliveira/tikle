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

#ifndef LEMON_PARSER_H
#define LEMON_PARSER_H

#include "../../tikle_defs.h"

#define IN_DEBUG 0

#if IN_DEBUG
# define DEBUG(msg, ...) printf(msg "\n", ##__VA_ARGS__);
#else
# define DEBUG(msg, ...)
#endif

#define NUM_OPS 10

#define CALL_PARSER() \
	Parse(pParser, tok, token, &pdata);

#define PARSER_FREE()                            \
	ParseFree(pParser, free);                    \
	free(token);                                 \
	free(state);                                 \
	*(pdata.commands+pdata.num_commands) = NULL;

#define NUM_PROTOCOLS 4

#define VAR_IP 		1
#define VAR_DEVICE 	2

#define OP_IS_EQUAL     1
#define OP_IS_NOT_EQUAL 2

typedef struct _parser_data {
	short int in_label;
	unsigned long label;
	faultload_protocol protocol;
	unsigned int num_commands;
	unsigned int num_allocs;
	faultload_op **commands;
	faultload_num_type occur_type;
	short int error;
	unsigned int line;
	struct _ips {
		short int num;
		struct _info {
			unsigned long ip;
			short int used;
		} arr[10];
	} ips;	
	struct _ips_tmp {
		short int num;
		unsigned long ips[10];
	} ip_tmp;
	short int part_num;
	struct _partition {
		size_t count;
		unsigned long ips[10];
	} partition[10];
} parser_data;

faultload_op **faultload_parser(char *s);

#define NEW_OP(x) \
	faultload_op *op = (faultload_op *) malloc(sizeof(faultload_op)); \
	op->protocol = 0;                                \
	op->opcode = x;                                  \
	op->num_ops = 0;                                 \
	op->op_type = NULL;                              \
	op->op_value = NULL;                             \
	op->label = pdata->label;                        \
	op->occur_type = pdata->occur_type;              \
	op->block_type = 0;                              \
	op->extended_value = 0;                          \
	op->next_op = pdata->num_commands;
	
#define OP_NUM() op->next_op

#define GET_OP() op

#define NEXT_OP op->num_ops++;

#define ALLOC_NEW_OP \
	op->op_type = (faultload_op_type *) realloc(op->op_type, (op->num_ops+1) * sizeof(faultload_op_type)); \
	op->op_value = (faultload_value_type *) realloc(op->op_value, (op->num_ops+1) * sizeof(faultload_value_type));

#define SET_OP_ARRAY(_arr) \
({                                                                     \
	unsigned short int i;                                              \
	ALLOC_NEW_OP;                                                      \
	op->op_type[op->num_ops] = ARRAY;                                  \
	op->op_value[op->num_ops].array.count = _arr->ips.num;             \
	for (i = 0; i < _arr->ips.num; i++) {                              \
		op->op_value[op->num_ops].array.nums[i] = _arr->ips.arr[i].ip; \
	}                                                                  \
	NEXT_OP;                                                           \
})

#define SET_NEXT_OP(x, y) \
	pdata->commands[(x)]->next_op = (y);

#define ADD_COMMAND() \
	if ((pdata->num_commands+1) == pdata->num_allocs) {                                                                           \
		pdata->commands = (faultload_op **) realloc(pdata->commands, (pdata->num_commands + NUM_OPS) * sizeof(faultload_op *)+1); \
		pdata->num_allocs += NUM_OPS;                                                                                             \
	}                                                                                                                             \
	*(pdata->commands + pdata->num_commands++) = op;

#define ALLOC_DATA(x) \
	x = (scanner_token *) malloc(sizeof(scanner_token));

#define INIT_DATA(x)  \
	x->type = UNUSED; \
	x->data.num = 0;

#define COPY_DATA(x, y)                                \
	*x = *y;                                           \
	if (y->type == STRING) {                           \
		x->data.str.value = strdup(y->data.str.value); \
		x->data.str.length = y->data.str.length;       \
	} else {                                           \
		x->data.num = y->data.num;                     \
	}
	
#define SET_OP(v) \
	ALLOC_NEW_OP;                                                        \
	op->op_type[op->num_ops] = v->type;                                  \
	if (v->type == STRING) {                                             \
		op->op_value[op->num_ops].str.value = strdup(v->data.str.value); \
		op->op_value[op->num_ops].str.length = v->data.str.length;       \
	} else if (v->type == ARRAY) {                                       \
		op->op_value[op->num_ops].array.count = v->data.array.count;     \
		memcpy(op->op_value[op->num_ops].array.nums, v->data.array.nums, sizeof(unsigned long) * v->data.array.count); \
	} else {                                                             \
		op->op_value[op->num_ops].num = v->data.num;                     \
	}                                                                    \
	NEXT_OP;

#define SET_OP_EXPR(t, v) \
	ALLOC_NEW_OP;                                                \
	op->op_type[op->num_ops] = t;                                \
	if (t == STRING) {                                           \
		op->op_value[op->num_ops].str.value = strdup((char*)v);  \
		op->op_value[op->num_ops].str.length = strlen((char*)v); \
	} else {                                                     \
		op->op_value[op->num_ops].num = v;                       \
	}                                                            \
	NEXT_OP;

#define COPY_TO_OP(v) \
	SET_OP(v);        \
	FREE_OP(v);

#define CHECK_USED_IP(_ip)                             \
	if (pdata->ips.num) {                              \
		int i = pdata->ips.num;                        \
		int found = 0;                                 \
		while (i--) {                                  \
			if ((_ip) == pdata->ips.arr[i].ip) {       \
				pdata->ips.arr[i].used++;              \
				found = 1;                             \
			}                                          \
		}                                              \
		if (found == 0) {                              \
			pdata->error = 2;                          \
		}                                              \
	}

#define FREE_STR(v) \
	free(v->data.str.value);

#define FREE_OP(v) \
	if (v->type == STRING) {      \
		free(v->data.str.value);  \
		v->data.str.value = NULL; \
	}                             \
	free(v);

#endif /* LEMON_PARSER_H */
