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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "high_types.h"
#include "high_func.h"

#define ARGS "c:d:fhm:s:v" /* supported args */
#define TIMEOUT 5 /* execution starts in TIMEOUT usecs */

int main(int argc, char **argv)
{
	int c, index, err;
	usr_args_t *data = malloc(sizeof(usr_args_t));

	/**
	 * setting default arguments
	 */
	data->counter = "local"; /* counter method */
	data->device = "auto"; /* seeks for the default nic device */
	data->faultload = "faultload.tkl"; /* default faultload script file */
	data->feedback = 0; /* reports to the user (on-the-fly) flag. default: no */
	data->check = 0; /* checks only the faultload script and exit */

	opterr = 0;
	while ((c = getopt (argc, argv, ARGS)) != -1)
		switch (c) {
			case 'c':
				/* check for errors in the faultload syntax */
				data->faultload = optarg;
				data->check = 1;
				break;
			case 'd':
				/* use a specific device. default is auto seek one */
				data->device = optarg;
				break;
			case 'f': /* provide feedback during campaign */
				data->feedback = 1;
				break;
			case 'h':
				goto help;
				break;
			case 'm':
				/* must be one of the two methods for counters: local or remote */
				if ((strncmp(optarg, "local", 5) == 0) || (strncmp(optarg, "remote", 6) == 0))
					data->counter = optarg;
				fprintf(stdout, "Unknown argument %s. Using default.\n", optarg);
				break;
			case 's': /* select a faultload script file */
				data->faultload = optarg;
				break;
			case 'v':
				goto version;
				break;
			case '?':
				if (optopt == 'c')
					fprintf(stderr, "Option -%c requires a faultload script file.\n", optopt);
				else if (optopt == 'd')
					fprintf(stderr, "Option -%c requires a network device (ethX).\n", optopt);
				else if (optopt == 'm')
					fprintf(stderr, "Option -%c requires a counter method (local|remote).\n", optopt);
				else if (optopt == 's')
					fprintf(stderr, "Option -%c requires a faultload script file.\n", optopt);
				else if (isprint (optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				goto exit;
			default:
				goto help;
		}
	
	for (index = optind; index < argc; index++) {
		fprintf(stderr, "Non-option argument %s\n", argv[index]);
	}

	/**
	 * go stricly to the point if we want
	 * the faultload script file just check
	 */
	if (data->check) {
		fprintf(stdout, "Checking: %s\n", data->faultload);
	} else {
		fprintf(stdout, "User definitions:\n");
		fprintf(stdout, "\tcounter method: %s\n", data->counter);
		fprintf(stdout, "\tdevice: %s\n", data->device);
		fprintf(stdout, "\tfaultload: %s\n", data->faultload);

		if (data->feedback == 0)
			fprintf(stdout, "\tProvide reports: no (default)\n");
		else
			fprintf(stdout, "\tProvide reports: yes\n");

		/* all ready, counting to TIMEOUT */
		for (c = TIMEOUT; c > 0; c--) {
			fprintf(stdout, "starting execution in %d...\n", c);
			usleep(1000000);
		}
	}

	/* call dispatcher */
	err = f_arg_handler(data);
	if (err == 0)
		return 0;
	else
		goto exit;

help:
	fprintf(stdout, "Command Line Options:\n");
	fprintf(stdout, " %s [%s]\n", argv[0], ARGS);
	fprintf(stdout, "...\n");
	fprintf(stdout, " -c checks for syntax errors in a faultload.\n");
	fprintf(stdout, " -d [ethX] define the system network device, for remote counter only (default: auto).\n");
	fprintf(stdout, " -f provides feedback (graphics and status) during execution.\n");
	fprintf(stdout, " -h print this help menu.\n");
	fprintf(stdout, " -m [local|remote] defines de counter method (default: local).\n");
	fprintf(stdout, " -v print the software version.\n");
	fprintf(stdout, "done.\n");
	goto exit;

version:
	system("cat VERSION");

exit:
	return 1;
}
