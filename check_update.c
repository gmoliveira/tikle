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
#include <curl/curl.h>

#define CURRENT_FILE "VERSION"

char *new_version = NULL;

char *f_get_current_version(char *file)
{
	FILE *fp;
	char *buf = malloc(64);

	fp = fopen(file, "r");
	fgets(buf, 512 , fp);
	fclose(fp);

	return buf;
}

void f_get_curl_output(void *ptr, size_t size, size_t nmemb, void *stream)
{
	new_version = ptr;
}

int main(void)
{
	CURL *curl;

	curl = curl_easy_init();

	char *current_version = malloc(64);;

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://localhost/update");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, f_get_curl_output);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	current_version = f_get_current_version(CURRENT_FILE);

	if (strcmp(current_version, new_version) == 0)
		fprintf(stdout, "No new versions available\n");
	else
		fprintf(stdout, "New version available\n");

	return 0;
}
