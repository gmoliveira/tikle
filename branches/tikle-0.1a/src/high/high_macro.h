/* 
 * Tikle kernel module
 * Copyright (C) 2009	Felipe 'ecl' Pena
 *			Gustavo 'nst' Oliveira
 * 
 * Contact us at: flipensp@gmail.com
 * 		  gmoliveira@inf.ufrgs.br
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

#ifndef HIGH_MACRO_H
#define HIGH_MACRO_H

#include <netinet/in.h>

#define SENDTO_OP_VALUE(n)									\
	switch (temp_p->op_type[(n)]) {								\
		case STRING:									\
			sendto(call->sock,							\
				&temp_p->op_value[(n)].str.length,				\
				sizeof(size_t),							\
				0, (struct sockaddr *)&call->addr,				\
				sizeof(call->addr));						\
			sendto(call->sock,							\
				temp_p->op_value[(n)].str.value,				\
				temp_p->op_value[(n)].str.length,				\
				0,								\
				(struct sockaddr *)&call->addr,					\
				sizeof(call->addr));						\
			break;									\
		case ARRAY:									\
			sendto(call->sock,							\
				&temp_p->op_value[(n)].array.count,				\
				sizeof(size_t),							\
				0,		   						\
				(struct sockaddr *)&call->addr,					\
				sizeof(call->addr));			   			\
			sendto(call->sock,							\
				&temp_p->op_value[(n)].array.nums,				\
				sizeof(unsigned long) * temp_p->op_value[(n)].array.count,	\
				0,			  					\
				(struct sockaddr *)&call->addr,					\
			sizeof(call->addr));							\
			break;									\
		default:									\
			sendto(call->sock,							\
				&temp_p->op_value[(n)].num,					\
				sizeof(unsigned long),						\
				0,		    						\
				(struct sockaddr *)&call->addr,					\
				sizeof(call->addr));						\
			break;									\
	}

#define SHOW_OP_INFO(n)												\
	switch (temp_p->op_type[(n)]) {									\
		case STRING:											\
			printf("op: %s (len: %d) [%s] | ",							\
				temp_p->op_value[(n)].str.value,						\
				temp_p->op_value[(n)].str.length,						\
				op_types[temp_p->op_type[(n)]]);						\
			break;											\
		case ARRAY:											\
			{											\
				int i;										\
				printf("op: {");								\
				for (i = 0; i < temp_p->op_value[(n)].array.count; i++) {			\
					printf("%ld%s", temp_p->op_value[(n)].array.nums[i],		\
						(i == (temp_p->op_value[(n)].array.count-1) ? "" : ","));	\
				}										\
				printf("} [%s] | ", op_types[temp_p->op_type[(n)]]);			\
			}											\
			break;											\
		default:											\
			printf("op: %lu [%s] | ", temp_p->op_value[(n)].num,				\
				op_types[temp_p->op_type[(n)]]);						\
			break;											\
	}

#define FREE_IF_OP_STR(n)						\
	if (temp_p->op_type[(n)] == STRING) {			\
		free((void *)temp_p->op_value[(n)].str.value);	\
	}

#endif
