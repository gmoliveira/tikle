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

#define _GNU_SOURCE
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _IN_SCANNER
#include "scanner.h"
#include "parser.h"

#define YYCTYPE      char
#define YYCURSOR     (s->start)
#define YYLIMIT      (s->end)
#define YYMARKER     (s->ctx)
#define YYCTXMARKER  (s->ctx)

int scan(scanner_state *s, scanner_token *token)
{
	int r = SCANNER_IMPOSSIBLE;
	char *q = s->start;

	/*!re2c
	re2c:indent:top = 1;
	re2c:yyfill:enable = 0;	
	
	NUMBER         = [0-9]+;
	NEWLINE        = [\n];
	SPACES         = [ \r\t]+;
	COMMENT        = [#][^\r\n]*;
	QUOTED_STRING  = (['][^']*[']|["][^"]*["]);
	STRING         = [A-Za-z]+;
	IP		       = [0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3};
	HOST_LABEL     = [@][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3};
	*/

	/*!re2c	
	'@' { return T_AT; }
	';' { return T_END_EXPR; }
	':' { return T_COLON; }
	',' { return T_COMMA; }
	'{' { return T_LBRACES; }
	'}' { return T_RBRACES; }
	'->' { return T_ARROW; }
	
	NEWLINE { return SCANNER_NEWLINE; }
	SPACES  { return SCANNER_IGNORE; }
	COMMENT { return SCANNER_IGNORE; }

	'(' { return T_LPAREN; }
	')' { return T_RPAREN; }

	'%IP' {
		token->type = VAR;
		token->data.num = VAR_IP;
		return T_IP_VAR;
	}
	
	'%DEVICE' {
		token->type = VAR;
		token->data.num = VAR_DEVICE;
		return T_DEVICE_VAR;
	}	
	
	QUOTED_STRING {
		char *tmp;

		tmp = strndup(q + 1, YYCURSOR - q - 2);
		token->type = STRING;
		token->data.str.value = tmp;
		token->data.str.length = (YYCURSOR - q - 2) + 1;

		return T_QUOTED_STRING;
	}
	
	NUMBER/[m][s] {
		char *tmp;

		tmp = strndup(q, YYCURSOR - q);
		token->data.num = atoi(tmp);
		token->type = NUMBER;
		token->num_type = TEMPORAL;
		free(tmp);
		
		YYCURSOR += 2;

		return T_NUMBER;	
	}

	NUMBER/[%] {
		char *tmp;

		tmp = strndup(q, YYCURSOR - q);
		token->data.num = atoi(tmp);
		token->num_type = PERCT;
		token->type = NUMBER;
		free(tmp);
		
		YYCURSOR++;
		return T_NUMBER;
	}
	
	NUMBER/[p] {
		char *tmp;

		tmp = strndup(q, YYCURSOR - q);
		token->data.num = atoi(tmp);
		token->type = NUMBER;
		token->num_type = NPACKETS;
		free(tmp);
		
		YYCURSOR++;
		return T_NUMBER;
	}
	

	
	NUMBER[s]? {
		char *tmp;

		tmp = strndup(q, YYCURSOR - q);
		token->data.num = atoi(tmp);
		token->type = NUMBER;
		token->num_type = TEMPORAL;
		free(tmp);

		return T_NUMBER;
	}
	
	IP {
		char *tmp;

		tmp = strndup(q, YYCURSOR - q);
		token->data.num = inet_addr(tmp);
		token->type = NUMBER;
		free(tmp);

		return T_IP;
	}
	
	HOST_LABEL {
		char *tmp;

		tmp = strndup(q + 1, YYCURSOR - q - 1);
		token->data.num = inet_addr(tmp);
		token->type = NUMBER;
		free(tmp);

		return T_HOST_LABEL;
	}

	'UDP' { return T_UDP; }	
	'SCTP' { return T_SCTP; }
	'TCP' { return T_TCP; }	
	'ALL' { return T_ALL; }
	
	'STOP:' { return T_STOP; }
	'START:' { return T_START; }	

	'AFTER' { return T_AFTER; }
	'DROP' { return T_DROP; }
	'DUPLICATE' { return T_DUPLICATE; }
	'DELAY' { return T_DELAY; }
	'DECLARE' { return T_DECLARE; }
	'PROGRESSIVE' { return T_PROGRESSIVE; }
	'DO' { return T_DO; }
	'FOR' { return T_FOR; }
	'WHILE' { return T_WHILE; }
	'SET' { return T_SET; }
	'PARTITION' { return T_PARTITION; }
	'IF' { return T_IF; }
	'THEN' { return T_THEN; }
	'ELSE' { return T_ELSE; }
	'END IF' { return T_END_IF; }
	'END' { return T_END; }
	'FOR' { return T_FOR; }
	'EACH' { return T_EACH; }
	
	'is equal' { return T_IS_EQUAL; }
	
	'is not equal' { return T_IS_NOT_EQUAL; }
	
	STRING {
		char *tmp;

		tmp = strndup(q, YYCURSOR - q);
		token->type = STRING;
		token->data.str.value = tmp;
		token->data.str.length = (YYCURSOR - q) + 1;

		return T_STRING;
	}

	"\000"	{ return SCANNER_EOF; }
	
	[^]		{ return SCANNER_ERROR; }
	*/
	return r;
}
