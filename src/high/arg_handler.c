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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* i/o operations */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h> /* in case of errors, define errno variable */
#include <sys/mman.h> /* PROT_READ and MAP_SHARED macros */
#include <unistd.h> /* close() */

#include "high_func.h"
#include "high_types.h"
#include "lemon_parser.h"

faultload_op **f_load_faultload(char *fltld)
{
	int err;
	int fdin;
	char *source;
	struct stat statbuf;
	faultload_op **temp;

	/**
	 * open faultload file for syntax checking
	 */
	fdin = open(fltld, O_RDONLY);
	if (fdin < 0)
		goto error;

	err = fstat(fdin, &statbuf);
	if (err < 0)
		goto error;

	source = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0);
	if (source == MAP_FAILED)
		goto error;

	/**
	 * check faultload syntax
	 */
	temp = faultload_parser(source);

	return temp;

error:
	close(fdin);
	perror("Exiting: ");
	exit(EXIT_FAILURE);
}


int f_arg_handler(usr_args_t *usr)
{
	int err;
	usr_args_t *data = usr;

	faultload_op **faultload, **faultload_pp;

	/**
	 * load faultload and check its syntax
	 */
	faultload_pp = faultload = f_load_faultload(data->faultload);
	
	/**
	 * Parser error
	 */
	if (faultload == NULL) {
		errno = -EAGAIN;
	}

	/**
	 * if it was just a syntax checking, quit with success
	 */
	if (data->check == 1)
		exit(EXIT_SUCCESS);

	/**
	 * it is more than a syntax checking,
	 * lets get the device information
	 */
	err = strncmp(data->device, "auto", 4);
	if (err == 0) {
		data->device = f_get_default_device();
		if (!data->device)
			errno = -ENODEV;
	}

	/**
	 * ok, let's start the campaign
	 */
	err = f_config(faultload, data);

	return 0;
}
