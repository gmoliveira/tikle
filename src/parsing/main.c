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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "lemon_parser.h"

static const char *op_names[] = {
	"COMMAND", "AFTER", "WHILE", "HOST", "IF", "ELSE", 
	"END_IF", "END", "SET", "FOREACH", "PARTITION", "DECLARE"
};

static const char *op_types[] = {
	"UNUSED", "NUMBER", "STRING", "VAR", "ARRAY"
};

#define FREE_IF_OP_STR(n)                                   \
	if (faultload_p->op_type[(n)] == STRING) {              \
		free((void *)faultload_p->op_value[(n)].str.value); \
	}
	
#define SENDTO_OP_VALUE(n)                                                                                   \
	switch (faultload_p->op_type[(n)]) {                                                                     \
		case STRING:                                                                                         \
			sendto(tikle_sock_client, &faultload_p->op_value[(n)].str.length, sizeof(size_t),                       \
				0, (struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));                        \
			sendto(tikle_sock_client, faultload_p->op_value[(n)].str.value, faultload_p->op_value[(n)].str.length, \
				0, (struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));                        \
			break;                                                                                           \
		case ARRAY:                                                                                          \
			sendto(tikle_sock_client, &faultload_p->op_value[(n)].array.count, sizeof(size_t), 0,                   \
				(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));                           \
			sendto(tikle_sock_client, &faultload_p->op_value[(n)].array.nums,                                       \
				sizeof(unsigned long) * faultload_p->op_value[(n)].array.count, 0,                           \
				(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));                           \
			break;                                                                                           \
		default:                                                                                             \
			sendto(tikle_sock_client, &faultload_p->op_value[(n)].num, sizeof(unsigned long), 0,                    \
				(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));                           \
			break;                                                                                           \
	}

#define SHOW_OP_INFO(n) \
	switch (faultload_p->op_type[(n)]) {                                               \
		case STRING:                                                                   \
			printf("op: %s (len: %d) [%s] | ",                                         \
				faultload_p->op_value[(n)].str.value,                                  \
				faultload_p->op_value[(n)].str.length,                                 \
				op_types[faultload_p->op_type[(n)]]);                                  \
			break;                                                                     \
		case ARRAY:                                                                    \
			{                                                                          \
				int i;                                                                 \
				printf("op: {");                                                       \
				for (i = 0; i < faultload_p->op_value[(n)].array.count; i++) {         \
					printf("%ld%s", faultload_p->op_value[(n)].array.nums[i],          \
						(i == (faultload_p->op_value[(n)].array.count-1) ? "" : ",")); \
				}                                                                      \
				printf("} [%s] | ", op_types[faultload_p->op_type[(n)]]);              \
			}                                                                          \
			break;                                                                     \
		default:                                                                       \
			printf("op: %lu [%s] | ", faultload_p->op_value[(n)].num,                  \
				op_types[faultload_p->op_type[(n)]]);                                  \
			break;                                                                     \
	}

/**
 * Send the log to all ones involved in the experiment
 */
static void tikle_send_log(int partition_num_ips)
{
	struct sockaddr_in tikle_log_server_addr;
	unsigned long **tikle_log_all;
	unsigned int i, n;
	socklen_t tikle_socklen;
	int tikle_err, tikle_log_sock_server;

	/*
	 * wait for the end of experiment when
	 * hosts will send logs to controller
	 */
	memset(&tikle_log_server_addr, 0, sizeof(struct sockaddr_in));

	tikle_log_sock_server = socket(AF_INET, SOCK_DGRAM, 0);
	tikle_log_server_addr.sin_family = AF_INET;
	tikle_log_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	tikle_log_server_addr.sin_port = htons(12128);
	bind(tikle_log_sock_server, (struct sockaddr *)&tikle_log_server_addr, sizeof(tikle_log_server_addr));
	
	tikle_log_all = (unsigned long **) calloc(partition_num_ips, sizeof(unsigned long *));
	
	if (tikle_log_all == NULL) {
		printf("Unable to alloc memory to tikle_log_all\n");
		return;
	}	
	
	tikle_socklen = sizeof(tikle_log_server_addr);

	for (n = 0; n < partition_num_ips; n++) {
		tikle_log_all[n] = (unsigned long *) malloc(sizeof(unsigned long) * 4 * partition_num_ips);
				
		tikle_err = recvfrom(tikle_log_sock_server, tikle_log_all[n], sizeof(unsigned long) * 4 * partition_num_ips,
			0, (struct sockaddr *)&tikle_log_server_addr, &tikle_socklen);
			
		printf("tikle alert: received log from %s\n", inet_ntoa(tikle_log_server_addr.sin_addr));
		printf(" HOST            EVENT | IN  | OUT\n");

		for (i = 0; i < partition_num_ips; i++) {
			printf("%-15s | %03lu | %03lu | %03lu\n",inet_ntoa(tikle_log_server_addr.sin_addr) /*inet_ntoa(*(struct in_addr*)&tikle_log_all[n][4*i])*/,
				tikle_log_all[n][4*i+1], tikle_log_all[n][4*i+2], tikle_log_all[n][4*i+3]);
		}
		
		/* Freeing */
		free(tikle_log_all[n]);
	}
	
	free(tikle_log_all);
}

int main(int argc, char **argv)
{
	int fdin, partition_num_ips = 0, tikle_sock_client, tikle_sock_server;
	int tikle_err, tikle_num_replies = 0;
	int return_val, numreqs = 30, broadcast = 1, n, i = 0, j = 0;
	faultload_op **faultload, **faultload_pp, *faultload_p, *partition_ips = NULL;
	struct sockaddr_in tikle_client_addr, tikle_server_addr;
	char *source, tikle_reply[sizeof("tikle-received")];
	socklen_t tikle_socklen;
	struct stat statbuf;
	struct ifconf ifc;
	struct ifreq *ifr;

	if (argc < 2) {
		argv[1] = "../faultload/default.tkl";
	} else if (argc > 2) {
		printf("Syntax Error.\nUsage: %s <faultload>\n", argv[0]);
		return 0;
	}

	if ((fdin = open(argv[1], O_RDONLY)) < 0) {
		printf("Error: open\n");
		return 0;
	}
	
	if (fstat(fdin, &statbuf) < 0) {
		printf("Error: fstat\n");
		close(fdin);
		return 0;
	}
	
	if ((source = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0)) == (caddr_t) -1) {
   		printf("Error: mmap\n");
   		close(fdin);
   		return 0;
	}

	faultload_pp = faultload = faultload_parser(source);
	
	/*
	 * Parser error
	 */
	if (faultload == NULL) {
		exit(0);
	}

	/*
	 * create dispatcher socket to send faultloads through all the network (unicast)
	 * with broadcast permission to trigger the experiment simultaneously (tikle-ok)
	 */

	memset(&tikle_client_addr, 0, sizeof(struct sockaddr_in));

	tikle_sock_client = socket(AF_INET, SOCK_DGRAM, 0);
	if (setsockopt(tikle_sock_client, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast, sizeof(broadcast)) < 0) {
		printf("erro ao definir permissao para broadcast\n");
		close(fdin);
		return 0;
	}

	tikle_client_addr.sin_family = AF_INET;
	tikle_client_addr.sin_port = htons(12608);
	
	/*
	 * create a socket to lock the experiment until receive all "tikle-received" messages
	 * and then broadcast a "tikle-start" message to trigger the experiment simultaneously
	 */

	memset(&tikle_server_addr, 0, sizeof(struct sockaddr_in));

	tikle_sock_server = socket(AF_INET, SOCK_DGRAM, 0);
	tikle_server_addr.sin_family = AF_INET;
	tikle_server_addr.sin_port = htons(21508);
	bind(tikle_sock_server, (struct sockaddr *)&tikle_server_addr, sizeof(tikle_server_addr));

	/* Check for partition mode */
	if (faultload && *faultload && (*faultload)->opcode == DECLARE) {
		partition_ips = *faultload;
		partition_num_ips = (*faultload)->op_value[0].array.count;
		faultload++;
	}
		
	/* Sending opcode information */	
	while (faultload && *faultload) {
		faultload_p = *faultload;
		
		/* When using @host, only send the information to the host specified */
		if (faultload_p->opcode == HOST) {
			tikle_client_addr.sin_addr.s_addr = (in_addr_t) faultload_p->op_value[0].num;
			
			/* Send the number of ips envolved in the experiment */
			sendto(tikle_sock_client, &partition_num_ips, sizeof(int), 0,
				(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
		} else {
			int i = 0;
						
			do {
				if (partition_ips) {
					/* 
					 * Send the information for each ips declared 
					 * on @declare { } block, when used.
					 */
					tikle_client_addr.sin_addr.s_addr = (in_addr_t) partition_ips->op_value[0].array.nums[i];
				
					if (partition_ips == *(faultload-1)) {
						/* 
						 * Send the number of ips envolved in the experiment
						 * (i.e. the ones listed in @declare)
						 */
						sendto(tikle_sock_client, &partition_num_ips, sizeof(int), 0,
							(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
					}
				}

				sendto(tikle_sock_client, &faultload_p->opcode, sizeof(faultload_opcode), 0, 
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
				sendto(tikle_sock_client, &faultload_p->protocol, sizeof(faultload_protocol), 0,
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
				sendto(tikle_sock_client, &faultload_p->occur_type, sizeof(faultload_num_type), 0,
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
				sendto(tikle_sock_client, &faultload_p->num_ops, sizeof(unsigned short int), 0,
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
				
				if (faultload_p->num_ops > 0) {
					sendto(tikle_sock_client, faultload_p->op_type, sizeof(faultload_op_type) * faultload_p->num_ops, 0,
						(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
					
					for (j = 0; j < faultload_p->num_ops; j++) {					
						SENDTO_OP_VALUE(j);
					}
				}
			
				sendto(tikle_sock_client, &faultload_p->extended_value, sizeof(int), 0,
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
				sendto(tikle_sock_client, &faultload_p->label, sizeof(unsigned long), 0,
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
				sendto(tikle_sock_client, &faultload_p->block_type, sizeof(short int), 0,
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
				sendto(tikle_sock_client, &faultload_p->next_op, sizeof(unsigned int), 0,
					(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
			} while (++i < partition_num_ips);
		}
		
		/* Debug information */
		printf("%03d: %-5s>> nr: %03d | opcode[%d]: %s | proto: %d | ",
			i++,
			(faultload_p->block_type == 0 ? "START" : "STOP"),
			faultload_p->next_op,
			faultload_p->opcode,
			op_names[faultload_p->opcode],
			faultload_p->protocol);
		
		for (j = 0; j < faultload_p->num_ops; j++) {
			SHOW_OP_INFO(j);
			FREE_IF_OP_STR(j);
		}

		printf("ext: %d | occur: %d\n", faultload_p->extended_value, faultload_p->occur_type);

		if (faultload_p->num_ops) {
			free(faultload_p->op_value);
			free(faultload_p->op_type);
		}
		free(faultload_p);
		faultload++;
	}

  	memset (&ifc, 0, sizeof(ifc));
  
  	ifc.ifc_buf = NULL;
  	ifc.ifc_len = sizeof(struct ifreq) * numreqs;
  	ifc.ifc_buf = malloc(ifc.ifc_len);

  	/*
	 * This code attempts to handle an arbitrary number of interfaces,
	 * it keeps trying the ioctl until it comes back OK and the size
	 * returned is less than the size we sent it.
  	 */

  	for (;;) {
  		ifc.ifc_len = sizeof(struct ifreq) * numreqs;
  		ifc.ifc_buf = realloc(ifc.ifc_buf, ifc.ifc_len);
  		
  		if ((return_val = ioctl(tikle_sock_client, SIOCGIFCONF, &ifc)) < 0) {
  			perror("SIOCGIFCONF");
  			break;
  		}
  		if (ifc.ifc_len == sizeof(struct ifreq) * numreqs) {
  			/* assume it overflowed and try again */
  			numreqs += 10;
  			continue;
  		}
  		break;
  	}
  	
  	if (return_val < 0) {
  		fprintf(stderr, "ioctl:");		
  		fprintf(stderr, "got error %d (%s)\n", errno, strerror(errno));
  		exit(1);
  	}  
  
  	/* loop through interfaces returned from SIOCGIFCONF */
  	ifr = ifc.ifc_req;
  	for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) {
  		/* Get the BROADCAST address */
  		return_val = ioctl(tikle_sock_client, SIOCGIFBRDADDR, ifr);
  		
		if (return_val == 0 ) {
  			if (ifr->ifr_broadaddr.sa_family == AF_INET) {
  				struct sockaddr_in *sin = (struct sockaddr_in *) &ifr->ifr_broadaddr;
				
				if ((strcmp(ifr->ifr_name, "eth0") == 0) || (strcmp(ifr->ifr_name, "eth1") == 0)) {
					tikle_server_addr.sin_addr.s_addr = htonl(inet_addr(inet_ntoa(sin->sin_addr)));
					break;
				}
  			} else {
  				printf("unsupported family for broadcast\n");
  			}
  		} else {
  			perror("Get broadcast failed");
  		}
  
  		/* check the next entry returned */
  		ifr++;
  	}
  
  	/* we don't need this memory any more */
  	free (ifc.ifc_buf);

	printf("tikle alert: waiting for host replies...\n");
	
	tikle_socklen = sizeof(tikle_server_addr);

	for (; tikle_num_replies < partition_num_ips;) {		
		tikle_err = recvfrom(tikle_sock_server, tikle_reply, sizeof("tikle-received"), 0,
			(struct sockaddr *)&tikle_server_addr, &tikle_socklen);
		
		if (strncmp(tikle_reply, "tikle-received", sizeof("tikle-received")) == 0) {
			printf("tikle alert: received confirmation from %s\n", inet_ntoa(tikle_server_addr.sin_addr));
			tikle_num_replies++;
		} else {
			printf("tikle alert: 'tikle-received' has been not received from %s\n", inet_ntoa(tikle_server_addr.sin_addr));
		}
		
		/* Clearing received message */
		memset(tikle_reply, 0, sizeof(tikle_reply));
	}

	printf("tikle alert: all replies received\n\tStarting tests!\n");

	sendto(tikle_sock_client, "tikle-start", sizeof("tikle-start"), 0,
		(struct sockaddr *)&tikle_client_addr, sizeof(tikle_client_addr));
		
	/*
	 * Sending log
	 */
	tikle_send_log(partition_num_ips);

	if (partition_ips) {
		free(partition_ips->op_type);
		free(partition_ips->op_value);
		free(partition_ips);
	}

	free(faultload_pp);
	
	close(fdin);
	
	return 0;
}
