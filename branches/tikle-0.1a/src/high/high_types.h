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

#ifndef HIGH_TYPES_H
#define HIGH_TYPES_H

#include <netinet/in.h>
#include "../global_types.h"
/**
 * main communication structure
 */
typedef struct cfg_socket {
	int sock;
	struct sockaddr_in addr;
} cfg_sock_t;

/**
 * structure to keep user options
 */
typedef struct user_arguments {
	char *counter;
	char *device;
	char *faultload;
	int feedback;
	int check; 
} usr_args_t;

typedef struct faultload_extra {
	int num_ips;
	faultload_op *partition_ips;
} faultload_extra_t;


#endif
