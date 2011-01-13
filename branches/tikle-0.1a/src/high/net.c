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

#include <net/if.h>
#include <ifaddrs.h>

#include "high_types.h"

/**
 * get the broadcast address of the ethernet device defined
 */
uint32_t f_get_broadcast(char *device)
{
	struct ifaddrs *ifs, *ifp;
	char temp[32];

	if (getifaddrs (&ifs)) {
		perror ("getifaddrs: ");
		return 1;
	}

	for (ifp = ifs; ifp; ifp = ifp->ifa_next) {
		if (ifp->ifa_addr->sa_family != AF_INET)
			continue;
		if (strncmp(ifp->ifa_name, device, strlen(device)) == 0)
			break;
	}

	return htonl(inet_addr(inet_ntop(ifp->ifa_addr->sa_family, &((struct sockaddr_in *) (ifp->ifa_broadaddr))->sin_addr, temp, 32)));
}

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
