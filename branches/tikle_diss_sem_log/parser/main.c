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

#define FREE_IF_OP_STR(n)                                   \
	if (faultload_p->op_type[(n)] == STRING) {              \
		free((void *)faultload_p->op_value[(n)].str.value); \
	}
	
#define SENDTO_OP_VALUE(n)                                                                                   \
	switch (faultload_p->op_type[(n)]) {                                                                     \
		case STRING:                                                                                         \
			sendto(pie_sock.s_cli, &faultload_p->op_value[(n)].str.length, sizeof(size_t),                       \
				0, (struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));                        \
			sendto(pie_sock.s_cli, faultload_p->op_value[(n)].str.value, faultload_p->op_value[(n)].str.length, \
				0, (struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));                        \
			break;                                                                                           \
		case ARRAY:                                                                                          \
			sendto(pie_sock.s_cli, &faultload_p->op_value[(n)].array.count, sizeof(size_t), 0,                   \
				(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));                           \
			sendto(pie_sock.s_cli, &faultload_p->op_value[(n)].array.nums,                                       \
				sizeof(unsigned long) * faultload_p->op_value[(n)].array.count, 0,                           \
				(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));                           \
			break;                                                                                           \
		default:                                                                                             \
			sendto(pie_sock.s_cli, &faultload_p->op_value[(n)].num, sizeof(unsigned long), 0,                    \
				(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));                           \
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
/*
#define log_saddr(_i) _i*(line_size) + 1
#define log_event(_i, _j) _i*(line_size) + 2 + 3 * _j
#define log_in(_i, _j)    _i*(line_size) + 3 + 3 * _j
#define log_out(_i, _j)   _i*(line_size) + 4 + 3 * _j
*/
static const char *op_names[] = {
	"COMMAND", "AFTER", "WHILE", "HOST", "IF", "ELSE", 
	"END_IF", "END", "SET", "FOREACH", "PARTITION", "DECLARE"
};

static const char *op_types[] = {
	"UNUSED", "NUMBER", "STRING", "VAR", "ARRAY"
};

struct data_sock {
	int s_cli, s_srv;
	struct sockaddr_in addr_cli, addr_srv;
};

int log_finalize(int num_ips, int num_events)
{
	struct sockaddr_in addr_log;
	unsigned int log_size;
//	unsigned long **log_all;
	unsigned long *log_all;
	socklen_t socklen;
	int i, j, err, sock_log;//, line_size;

//	line_size = 3 * (num_events-1) + 1;
//	log_size = (num_ips * (num_ips - 1)) * 2 + 3 * (num_events-1) * (num_ips * (num_ips - 1));
	log_size = num_ips * 3 * num_events + 1;
	/*
	 * wait for the end of experiment when
	 * hosts will send logs to the controller
	 */
	memset(&addr_log, 0, sizeof(struct sockaddr_in));

	sock_log = socket(AF_INET, SOCK_DGRAM, 0);
	addr_log.sin_family = AF_INET;
	addr_log.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_log.sin_port = htons(24187);

	bind(sock_log, (struct sockaddr *)&addr_log, sizeof(addr_log));

//	log_all = (unsigned long **) calloc(num_ips, sizeof(unsigned long *));
	log_all = (unsigned long *) calloc(log_size, sizeof(unsigned long));

	socklen = sizeof(addr_log);

	for (i = 0; i < num_ips; i++) {

//		err = recvfrom(sock_log, log_all[i], sizeof(unsigned long) * log_size, 0, (struct sockaddr *)&addr_log, &socklen);
		err = recvfrom(sock_log, log_all, sizeof(unsigned long) * log_size, 0, (struct sockaddr *)&addr_log, &socklen);
		printf("%s: %lu\n", inet_ntoa(addr_log.sin_addr), log_all);

	}

	free(log_all);
	
	return 0;
}

int main(int argc, char **argv)
{
	int numreqs = 30, n, i = 0, j = 0;
	int err, num_replies = 0, num_events = 0;
	int fdin, partition_num_ips = 0, broadcast = 1;

	faultload_op **faultload, **faultload_pp, *faultload_p, *partition_ips = NULL;

	char *source, auth_msg[12];

	socklen_t socklen;

	struct stat statbuf;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct data_sock pie_sock;

	if (argc < 2) {
		argv[1] = "default.tkl";
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

	memset(&pie_sock.addr_cli, 0, sizeof(struct sockaddr_in));

	pie_sock.s_cli = socket(AF_INET, SOCK_DGRAM, 0);
	if (setsockopt(pie_sock.s_cli, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast, sizeof(broadcast)) < 0) {
		printf("error while setting broadcast permission to socket");
		close(fdin);
		return 0;
	}

	pie_sock.addr_cli.sin_family = AF_INET;
	pie_sock.addr_cli.sin_port = htons(12608);
	
	/*
	 * create a socket to lock the experiment until receive all "pie:received" messages
	 * and then broadcast a "pie:start" message to trigger the experiment simultaneously
	 */

	memset(&pie_sock.addr_srv, 0, sizeof(struct sockaddr_in));

	pie_sock.s_srv = socket(AF_INET, SOCK_DGRAM, 0);
	pie_sock.addr_srv.sin_family = AF_INET;
	pie_sock.addr_srv.sin_port = htons(21508);
	bind(pie_sock.s_srv, (struct sockaddr *)&pie_sock.addr_srv, sizeof(pie_sock.addr_srv));

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
			pie_sock.addr_cli.sin_addr.s_addr = (in_addr_t) faultload_p->op_value[0].num;
			
			/* Send the number of ips envolved in the experiment */
			sendto(pie_sock.s_cli, &partition_num_ips, sizeof(int), 0,
				(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
		} else {
			int i = 0;
			
			/* Counting the AFTER in the START part */
			if (faultload_p->opcode == AFTER && faultload_p->block_type == 0) {
				num_events++;
			}
						
			do {
				if (partition_ips) {
					/* 
					 * Send the information for each ips declared 
					 * on @declare { } block, when used.
					 */
					pie_sock.addr_cli.sin_addr.s_addr = (in_addr_t) partition_ips->op_value[0].array.nums[i];

					if (partition_ips == *(faultload-1)) {

						sendto(pie_sock.s_cli, &partition_ips->op_value[0].array.nums[i], sizeof(int), 0,
							(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
						/* 
						 * Send the number of ips envolved in the experiment
						 * (i.e. the ones listed in @declare)
						 */
						sendto(pie_sock.s_cli, &partition_num_ips, sizeof(int), 0,
							(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
							
						/* 
						 * Send the stations IP address to avoid the requirement of a
						 * configured environment (closed LAN) to execute testes
						 */
						if (partition_num_ips) {
							sendto(pie_sock.s_cli, &partition_ips->op_value[0].array.nums,
								partition_num_ips * sizeof(unsigned long), 0,
								(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
						}
					}
				}

				sendto(pie_sock.s_cli, &faultload_p->opcode, sizeof(faultload_opcode), 0, 
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
				sendto(pie_sock.s_cli, &faultload_p->protocol, sizeof(faultload_protocol), 0,
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
				sendto(pie_sock.s_cli, &faultload_p->occur_type, sizeof(faultload_num_type), 0,
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
				sendto(pie_sock.s_cli, &faultload_p->num_ops, sizeof(unsigned short int), 0,
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
				
				if (faultload_p->num_ops > 0) {
					sendto(pie_sock.s_cli, faultload_p->op_type, sizeof(faultload_op_type) * faultload_p->num_ops, 0,
						(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
					
					for (j = 0; j < faultload_p->num_ops; j++) {					
						SENDTO_OP_VALUE(j);
					}
				}
			
				sendto(pie_sock.s_cli, &faultload_p->extended_value, sizeof(int), 0,
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
				sendto(pie_sock.s_cli, &faultload_p->label, sizeof(unsigned long), 0,
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
				sendto(pie_sock.s_cli, &faultload_p->block_type, sizeof(short int), 0,
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
				sendto(pie_sock.s_cli, &faultload_p->next_op, sizeof(unsigned int), 0,
					(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));
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

	printf("tikle alert: waiting for host replies...\n");
	
	socklen = sizeof(pie_sock.addr_srv);

	for (; num_replies < partition_num_ips;) {
		printf("tikle alert: confirmations remaing %d\n", partition_num_ips - num_replies);
		err = recvfrom(pie_sock.s_srv, auth_msg, sizeof("pie:received"), 0,
			(struct sockaddr *)&pie_sock.addr_srv, &socklen);
		
		if (strncmp(auth_msg, "pie:received", sizeof("pie:received")) == 0) {
			printf("tikle alert: received %s from %s\n", auth_msg, inet_ntoa(pie_sock.addr_srv.sin_addr));
			num_replies++;
		} else {
			printf("tikle alert: 'pie:received' has been not received from %s\n", inet_ntoa(pie_sock.addr_srv.sin_addr));
		}
		
		memset(auth_msg, 0, sizeof(auth_msg));
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
  		
  		if ((err = ioctl(pie_sock.s_cli, SIOCGIFCONF, &ifc)) < 0) {
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
  	
  	if (err < 0) {
  		fprintf(stderr, "ioctl:");		
  		fprintf(stderr, "got error %d (%s)\n", errno, strerror(errno));
  		exit(1);
  	}  
  
  	/* loop through interfaces returned from SIOCGIFCONF */
  	ifr = ifc.ifc_req;
  	for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) {
  		/* Get the BROADCAST address */
  		err = ioctl(pie_sock.s_cli, SIOCGIFBRDADDR, ifr);
  		
		if (err == 0 ) {
  			if (ifr->ifr_broadaddr.sa_family == AF_INET) {
  				struct sockaddr_in *sin = (struct sockaddr_in *) &ifr->ifr_broadaddr;
				
				if ((strcmp(ifr->ifr_name, "eth0") == 0) || (strcmp(ifr->ifr_name, "eth1") == 0)) {
//				if (strcmp(ifr->ifr_name, "vboxnet0") == 0) {
					pie_sock.addr_cli.sin_addr.s_addr = inet_addr(inet_ntoa(sin->sin_addr));
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

	printf("tikle alert: all replies received\n\tStarting tests!\n");
	sendto(pie_sock.s_cli, "pie:start", sizeof("pie:start"), 0,
		(struct sockaddr *)&pie_sock.addr_cli, sizeof(pie_sock.addr_cli));

	log_finalize(partition_num_ips, num_events);

	if (partition_ips) {
		free(partition_ips->op_type);
		free(partition_ips->op_value);
		free(partition_ips);
	}

	free(faultload_pp);
	
	close(fdin);
	
	return 0;
}
