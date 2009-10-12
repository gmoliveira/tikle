%include {
/*
 * tikle kernel module
 * Copyright (c) 2009 Felipe 'ecl' Pena
 *                    Gustavo 'nst' Oliveira
 *
 *   Contact us at:
 *   		   #c2zlabs@freenode.net
 *   		   www.c2zlabs.com
 *
 *   This file is part of tikle.
 *
 *   tikle is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   tikle is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with tikle; if not, write to the Free Software Foundation,
 *   Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"
#include "parser.h"
#include "lemon_parser.h"
}

%syntax_error {
	pdata->error = 1;
}
%parse_failure {
	printf("- parse failure\n");
}
%stack_overflow {
	printf("- stack!\n");
}
   
%token_prefix T_
%token_type {scanner_token*}

%destructor var_expr { FREE_OP($$); }
%destructor network { FREE_OP($$); }

%type action {int}
%type action2 {int}
%type op_type {int}
%type progressive_opt {int}
%type protocol {int}
%type while_expr {int}
%type after_expr {int}
%type if_expr {int}
%type else_expr {int}

%extra_argument {parser_data *pdata}
%start_symbol program

program ::= decl_network commands.

host_list ::= IP(A).
	{
		pdata->ips.arr[pdata->ips.num++].ip = A->data.num;
	}
host_list ::= host_list COMMA IP(A).
	{
		pdata->ips.arr[pdata->ips.num++].ip = A->data.num;
	}
decl_network ::= .
decl_network ::= decl_network AT DECLARE LBRACES host_list RBRACES END_EXPR.
	{
		NEW_OP(DECLARE);
		SET_OP_ARRAY(pdata);
		ADD_COMMAND();
	}

host_label_opt ::= .
	{
		pdata->label = pdata->in_label = pdata->occur_type = 0;
	}
host_label_opt ::= HOST_LABEL(A).
	{
		NEW_OP(HOST);
		op->occur_type = pdata->occur_type = 0;
		pdata->in_label = 1;
		pdata->label = A->data.num;
		SET_OP(A);
		ADD_COMMAND();
	}

commands ::= .
commands ::= commands host_label_opt START expr STOP stop_expr.

after_expr(B) ::= AFTER LPAREN NUMBER(A) RPAREN DO.
	{
		NEW_OP(AFTER);
		SET_OP(A);
		op->occur_type = pdata->occur_type = A->num_type;
		ADD_COMMAND();
		
		B = OP_NUM();
	}

while_expr(B) ::= WHILE LPAREN NUMBER(A) RPAREN DO.
	{
		NEW_OP(WHILE);
		SET_OP(A);
		op->occur_type = pdata->occur_type = A->num_type;
		ADD_COMMAND();
		
		B = OP_NUM();
	}

var_expr(A) ::= IP_VAR(B).
	{
		ALLOC_DATA(A);
		COPY_DATA(A, B);
	}
var_expr(A) ::= DEVICE_VAR(B).
	{
		ALLOC_DATA(A);
		COPY_DATA(A, B);
	}
var_expr(A) ::= QUOTED_STRING(B).
	{
		ALLOC_DATA(A);
		COPY_DATA(A, B);
		FREE_STR(B);
	}
var_expr(A) ::= IP(B).
	{
		ALLOC_DATA(A);
		COPY_DATA(A, B);
	}

else_expr(A) ::= ELSE.
	{
		NEW_OP(ELSE);
		ADD_COMMAND();

		A = OP_NUM();
	}
	
op_type(A) ::= IS_EQUAL. { A = OP_IS_EQUAL; }
op_type(A) ::= IS_NOT_EQUAL. { A = OP_IS_NOT_EQUAL; }
	
if_expr(D) ::= IF LPAREN var_expr(A) op_type(B) var_expr(C) RPAREN THEN.
	{
		NEW_OP(IF);
		COPY_TO_OP(A);
		COPY_TO_OP(C);

		op->extended_value = B;
		ADD_COMMAND();

		D = OP_NUM();
	}

ip_expr(A) ::= IP(B).
	{
		ALLOC_DATA(A);
		COPY_DATA(A, B);
	}
	
set_expr ::= SET ip_expr(A) ARROW STRING(B).
	{
		NEW_OP(SET);
		COPY_TO_OP(A);
		SET_OP(B);
		ADD_COMMAND();
		FREE_STR(B);
	}
	
foreach_expr ::= FOR EACH DO.
	{
		NEW_OP(FOREACH);
		op->occur_type = pdata->occur_type = ALL_PROTOCOL;
		ADD_COMMAND();
	}

expr ::= .
expr ::= expr if_expr(A) expr else_expr(B) expr END_IF.
	{
		NEW_OP(END_IF);	
		ADD_COMMAND();
		SET_NEXT_OP(A, B);
		SET_NEXT_OP(B, OP_NUM());
	}
expr ::= expr if_expr(A) expr END_IF.
	{
		NEW_OP(END_IF);
		ADD_COMMAND();
		SET_NEXT_OP(A, OP_NUM());
	}
expr ::= expr set_expr END_EXPR.
expr ::= expr after_expr(A) expr END.
	{
		NEW_OP(END);
		ADD_COMMAND();
		SET_NEXT_OP(A, OP_NUM());
	}
expr ::= expr while_expr(A) expr END.
	{
		NEW_OP(END);
		ADD_COMMAND();
		SET_NEXT_OP(A, OP_NUM());
	}
expr ::= expr commands END_EXPR.
expr ::= expr foreach_expr expr2 END.

protocol_label ::= protocol(P) COLON.
	{
		pdata->protocol = P;
	}
		
expr2 ::= .
expr2 ::= expr2 protocol_label commands_2.

action(A) ::= DROP. { A = ACT_DROP; } 
action(A) ::= DUPLICATE. { A = ACT_DUPLICATE; }

action2(A) ::= DELAY. { A = ACT_DELAY; }

progressive_opt(A) ::= . { A = 0; }
progressive_opt(A) ::= PROGRESSIVE. { A = 1; }

protocol(A) ::= SCTP. { A = SCTP_PROTOCOL; }
protocol(A) ::= TCP. { A = TCP_PROTOCOL; }
protocol(A) ::= UDP. { A = UDP_PROTOCOL; }
protocol(A) ::= ALL. { A = ALL_PROTOCOL; }

network_list ::= IP(A).
	{
		pdata->ip_tmp.ips[pdata->ip_tmp.num++] = A->data.num;
		CHECK_USED_IP(A->data.num);
	}
network_list ::= network_list COMMA IP(A).
	{
		pdata->ip_tmp.ips[pdata->ip_tmp.num++] = A->data.num;
		CHECK_USED_IP(A->data.num);
	}
network(A) ::= LBRACES network_list RBRACES.
	{
		ALLOC_DATA(A);
		A->type = ARRAY;
		A->data.array.count = pdata->ip_tmp.num;
		memcpy(A->data.array.nums, pdata->ip_tmp.ips, sizeof(unsigned long) * pdata->ip_tmp.num);
		
		pdata->ip_tmp.num = 0;
		memset(pdata->ip_tmp.ips, 0, sizeof(pdata->ip_tmp.ips));
	}
network_opt ::= .
network_opt ::= network_opt network(B).
	{		
		pdata->partition[pdata->part_num].count = B->data.array.count;
		memcpy(pdata->partition[pdata->part_num].ips, B->data.array.nums, sizeof(unsigned long) * B->data.array.count);
		pdata->part_num++;
		
		FREE_OP(B);
	}

commands ::= protocol(P) action(A) progressive_opt(B) NUMBER(C).
	{
		NEW_OP(COMMAND);
		op->protocol = P;
		SET_OP_EXPR(NUMBER, A);
		SET_OP(C);
		op->extended_value = B;
		ADD_COMMAND();
	}
num_aux(A) ::= NUMBER(C).
	{
		ALLOC_DATA(A);
		COPY_DATA(A, C);
	}
commands ::= protocol(P) action2(A) progressive_opt(B) num_aux(C) FOR NUMBER(D).
	{
		NEW_OP(COMMAND);
		op->protocol = P;
		SET_OP_EXPR(NUMBER, A);
		SET_OP(C);
		SET_OP(D);
		op->extended_value = B;
		ADD_COMMAND();
	}
commands ::= PARTITION network_opt.
	{
		unsigned short int i;
		
		NEW_OP(PARTITION);
		
		for (i = 0; i < pdata->part_num; i++) {
			ALLOC_NEW_OP;
		
			op->op_type[op->num_ops] = ARRAY;
			op->op_value[op->num_ops].array.count = pdata->partition[i].count;
			memcpy(op->op_value[op->num_ops].array.nums, pdata->partition[i].ips, sizeof(unsigned long) * pdata->partition[i].count);
			
			pdata->partition[i].count = 0;
			memset(pdata->partition[i].ips, 0, sizeof(pdata->partition[i].ips));
						
			NEXT_OP;
		}
		pdata->part_num = 0;
		
		ADD_COMMAND();
	}
commands_2 ::= .
commands_2 ::= commands_2 action(A) progressive_opt(B) NUMBER(C) END_EXPR.
	{
		NEW_OP(COMMAND);
		op->protocol = pdata->protocol;
		SET_OP_EXPR(NUMBER, A);
		SET_OP(C);
		op->extended_value = B;	
		ADD_COMMAND();
	}
	
stop_num(A) ::= NUMBER(B).
	{
		ALLOC_DATA(A);
		COPY_DATA(A, B);
	}

stop_expr ::= AFTER LPAREN stop_num(A) RPAREN END_EXPR.
	{
		NEW_OP(AFTER);
		op->protocol = 0;
		SET_OP(A);
		op->occur_type = A->num_type;
		op->block_type = 1;
		FREE_OP(A);
		ADD_COMMAND();
	}
