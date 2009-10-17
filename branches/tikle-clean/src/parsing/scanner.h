/*
 * tikle kernel module
 * Copyright (c) 2009 Felipe 'ecl' Pena
 *                    Gustavo 'nst' Oliveira
 *
 *   Contact us at:
 *   		   #c2zlabs@freenode.net
 *   		   www.c2zlabs.com
 *
 *   This file is part of tikle.
 *
 *   tikle is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   tikle is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with tikle; if not, write to the Free Software Foundation,
 *   Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SCANNER_H
#define SCANNER_H

#include "lemon_parser.h"

#define SCANNER_EOF         0
#define SCANNER_ERROR      -1
#define SCANNER_IMPOSSIBLE -2
#define SCANNER_IGNORE     -3
#define SCANNER_NEWLINE    -4

typedef struct _scanner_state {
	char *start, *end, *ctx;
} scanner_state;

typedef struct _scanner_token {
	union {
		unsigned long num;
		struct _str_token {
			size_t length;
			char *value;
		} str;
		struct _array_token {
			size_t count;
			unsigned long nums[10];
		} array;
	} data;
	faultload_op_type type;
	faultload_num_type num_type;
} scanner_token;

int scan(scanner_state *state, scanner_token *token);

#endif
