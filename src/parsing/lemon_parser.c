#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"
#include "parser.h"
#include "lemon_parser.h"

faultload_op **faultload_parser(char *s)
{
	int tok;
	scanner_token *token;
	scanner_state *state;
	parser_data pdata;
	void *pParser;
	
	if ((pParser = ParseAlloc(malloc)) == NULL) {
		ParseFree(pParser, free);
		return NULL;
	}
	if ((state = (scanner_state *) malloc(sizeof(scanner_state))) == NULL) {
		ParseFree(pParser, free);
		return NULL;
	}
	if ((token = (scanner_token *) malloc(sizeof(scanner_token))) == NULL) {
		ParseFree(pParser, free);
		free(state);
		return NULL;
	}

	/* Initializes parser info */
	pdata.commands = (faultload_op **) calloc(NUM_OPS+1, sizeof(faultload_op *));
	pdata.num_commands = 0;
	pdata.num_allocs = NUM_OPS;
	pdata.ips.num = 0;
	pdata.ip_tmp.num = 0;
	pdata.part_num = 0;
	memset(pdata.ips.arr, 0, sizeof(pdata.ips.arr));
	memset(pdata.partition, 0, sizeof(pdata.partition));
	memset(pdata.ip_tmp.ips, 0, sizeof(pdata.ip_tmp.ips));
	pdata.in_label = 0;
	pdata.label = 0;
	pdata.line = 1;
	pdata.error = 0;
	pdata.occur_type = 0;
	pdata.protocol = 0;
	
	state->start = s;

	while (SCANNER_EOF != (tok = scan(state, token))) {
		switch (tok) {
			case SCANNER_NEWLINE:
				pdata.line++;
			case SCANNER_IGNORE:
				/* Whitespaces */
				break;
			case SCANNER_ERROR:
				puts("Char não esperado!");
				goto clean;
			default:
				if (tok) {
					if (pdata.error == 1) {
						printf("Syntax error before: '%.*s...' on line %d\n", 15, state->end, pdata.line);
						return NULL;
					}
					CALL_PARSER();
				} else {		
					printf("Unknown error [%d]\n", tok);
				}
				break;
		}
		state->end = state->start;
	}
	if (tok == SCANNER_EOF) {
		CALL_PARSER();
	}

	if (pdata.ips.num) {
		printf("Declared ips:\n");
		for (; pdata.ips.num; pdata.ips.num--) {
			printf("- %s (%lu ; used: %d)\n",
				inet_ntoa(*(struct in_addr*)&pdata.ips.arr[pdata.ips.num-1].ip),
				pdata.ips.arr[pdata.ips.num-1].ip,
				pdata.ips.arr[pdata.ips.num-1].used);
		}
	}

clean:
	PARSER_FREE();
	
	return pdata.commands;
}
