/** 
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

#include <stdio.h> /* basic i/o hunny */
#include <stdlib.h> /* malloc() */
#include <string.h> /* strerror() */
#include <unistd.h> /* close() */

#include <errno.h> /* in case of errors, define errno variable */

/* socket stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> /* inet_addr(); */

#include "../global_types.h"
#include "high_types.h"
#include "high_macro.h"
#include "lemon_parser.h"

static const char *op_names[] = {
	"COMMAND", "AFTER", "WHILE", "HOST", "IF", "ELSE", 
	"END_IF", "END", "SET", "FOREACH", "PARTITION", "DECLARE"
};

static const char *op_types[] = {
	"UNUSED", "NUMBER", "STRING", "VAR", "ARRAY"
};

/**
 * create a client socket during the configuration phase
 */
cfg_sock_t *f_create_sock_client(int port)
{
	cfg_sock_t *temp = malloc(sizeof(cfg_sock_t));
	int err, bcast = 1;

	memset(&temp->addr, 0, sizeof(struct sockaddr_in));

	temp->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (temp->sock < 0)
		goto error;

	/* allow us to send broadcast messages */
	err = setsockopt(temp->sock,
			SOL_SOCKET,
			SO_BROADCAST,
			(void *)&bcast,
			sizeof(bcast));
	if (err < 0)
		goto error;

	temp->addr.sin_addr.s_addr = inet_addr("192.168.100.255");
	temp->addr.sin_family = AF_INET;
	temp->addr.sin_port = htons(port);
	
	return temp;

error:
	close(temp->sock);
	free(temp);
	fprintf(stdout, "%s", strerror(errno));
	exit(EXIT_FAILURE);
}

/**
 * create a server socket during the configuration phase
 */
cfg_sock_t *f_create_sock_server(int port)
{
	cfg_sock_t *temp = malloc(sizeof(cfg_sock_t));
	int err;

	memset(&temp->addr, 0, sizeof(struct sockaddr_in));

	temp->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (temp->sock < 0)
		goto error;

	temp->addr.sin_family = AF_INET;
	temp->addr.sin_port = htons(port);

	err = bind(temp->sock, (struct sockaddr *)&temp->addr, sizeof(struct sockaddr_in));
	if (err < 0)
		goto error;

	return 0;
error:
	close(temp->sock);
	free(temp);
	perror("Exiting: ");
	exit(EXIT_FAILURE);
}

/**
 * obtain some extra data through user faultload,
 * like number of total hosts in the experiment
 * as its internet addresses
 */
faultload_extra_t *f_read_faultload_extra(faultload_op **usr_faultld)
{
	faultload_extra_t *extra = malloc(sizeof(faultload_extra_t));
	faultload_op **temp = usr_faultld;

	if (temp && *temp && (*temp)->opcode == DECLARE) {
		extra->partition_ips = *temp;
		extra->num_ips = (*temp)->op_value[0].array.count;
		temp++;
	}

	return extra;
}

/**
 * send some extra data, like number of hosts in
 * experiment as its internet addresses
 */
int f_send_faultload_extra(cfg_sock_t *sock, faultload_op **usr_faultld)
{
	int i = 0;
	cfg_sock_t *call = sock;
	faultload_op **temp = usr_faultld, *temp_p;
	faultload_extra_t *extra;

	extra = f_read_faultload_extra(temp++);

	while (temp && *temp) {
		temp_p = *temp;
		do {
			if (extra->partition_ips) {
				call->addr.sin_addr.s_addr = (in_addr_t) extra->partition_ips->op_value[0].array.nums[i];
				if (extra->partition_ips == *(temp-1)) {
					sendto(call->sock, &extra->num_ips, sizeof(int), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
					if (extra->num_ips) {
						sendto(call->sock, &extra->partition_ips->op_value[0].array.nums, extra->num_ips * sizeof(unsigned long), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
					}
				}
			}
		} while (++i < extra->num_ips);
		free(temp_p);
		temp++;
	}

	return 0;
}


/**
 * send the user faultload to each host listed in it
 */
int f_send_faultload(cfg_sock_t *sock, faultload_op **usr_faultld)
{
	int j = 0;
	cfg_sock_t *call = sock;
	faultload_extra_t *extra;
	faultload_op **temp = usr_faultld, *temp_p;

	extra = f_read_faultload_extra(temp++);

	while (temp && *temp) {
		int i = 0;
		temp_p = *temp;
		do {
			call->addr.sin_addr.s_addr = (in_addr_t) extra->partition_ips->op_value[0].array.nums[i];
			sendto(call->sock, &temp_p->opcode, sizeof(faultload_opcode), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
			sendto(call->sock, &temp_p->protocol, sizeof(faultload_protocol), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
			sendto(call->sock, &temp_p->occur_type, sizeof(faultload_num_type), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
			sendto(call->sock, &temp_p->num_ops, sizeof(unsigned short int), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));

			if (temp_p->num_ops > 0) {
				sendto(call->sock, &temp_p->op_type, sizeof(faultload_op_type) * temp_p->num_ops, 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
				for (j = 0; j < temp_p->num_ops; j++) {
					SENDTO_OP_VALUE(j);
				}
			}

			sendto(call->sock, &temp_p->extended_value, sizeof(int), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
			sendto(call->sock, &temp_p->label, sizeof(unsigned long), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
			sendto(call->sock, &temp_p->block_type, sizeof(short int), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
			sendto(call->sock, &temp_p->next_op, sizeof(unsigned int), 0, (struct sockaddr *) &call->addr, sizeof(call->addr));
		} while (++i < extra->num_ips);

		/* Debug information */
		printf("%03d: %-5s>> nr: %03d | opcode[%d]: %s | proto: %d | ",
			i++,
			(temp_p->block_type == 0 ? "START" : "STOP"),
			temp_p->next_op,
			temp_p->opcode,
			op_names[temp_p->opcode],
			temp_p->protocol);

		for (j = 0; j < temp_p->num_ops; j++) {
			SHOW_OP_INFO(j);
			FREE_IF_OP_STR(j);
		}
		printf("ext: %d | occur: %d\n", temp_p->extended_value, temp_p->occur_type);

		if (temp_p->num_ops) {
			free(temp_p->op_value);
			free(temp_p->op_type);
		}
		free(temp_p);
		temp++;
	}

	return 0;
}

/**
 * this function manages the configuration phase by creating
 * a socket to send the faultloads, and other to receive the
 * confirmation.
 */
int f_config(faultload_op **faultload, usr_args_t *data)
{
	int err;

	cfg_sock_t *bcast_sock;
	cfg_sock_t *ready_sock;

//	usr_args_t *usr_defs = data;

	faultload_op **usr_faultload = faultload;

	/**
	 * create a broadcast socket. it sends the faultload
	 * (unicast) through all hosts and then a broadcast
	 * message to trigger the campaign
	 */
	bcast_sock = f_create_sock_client(12608);

	/**
	 * create a socket to lock the campaign triggering
	 * until all hosts confirm the receiving of the faultload
	 */
	ready_sock = f_create_sock_server(21508);

	/**
	 * send the number of total hosts in experiment
	 * as its internet addresses to each host
	 */
	err = f_send_faultload_extra(bcast_sock, usr_faultload);

	/**
	 * send the faultload script to each declared host
	 */
	err = f_send_faultload(bcast_sock, usr_faultload);

//	f_send_usr_args(usr_defs);

//	f_wait_confirm();

//	err = strncmp(data->counter, "remote", 6);
//	if (err == 0)
//		f_oper_remote();

	return 0;

//error:
//	exit(EXIT_FAILURE);
}
