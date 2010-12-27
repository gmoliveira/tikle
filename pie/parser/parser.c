/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 1 "parser.y"

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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"
#include "parser.h"
#include "lemon_parser.h"
#line 41 "parser.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 72
#define YYACTIONTYPE unsigned char
#define ParseTOKENTYPE scanner_token*
typedef union {
  ParseTOKENTYPE yy0;
  int yy136;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL parser_data *pdata;
#define ParseARG_PDECL ,parser_data *pdata
#define ParseARG_FETCH parser_data *pdata = yypParser->pdata
#define ParseARG_STORE yypParser->pdata = pdata
#define YYNSTATE 110
#define YYNRULE 55
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
#ifdef __cplusplus
static YYMINORTYPE yyzerominor;
#else
static const YYMINORTYPE yyzerominor;
#endif

/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   138,  138,  138,  138,  138,  148,  148,  148,  148,  138,
 /*    10 */    64,   65,   66,  138,  166,    6,  138,    1,  138,   38,
 /*    20 */     2,  138,    3,  138,  138,   54,   55,   56,   57,    8,
 /*    30 */   138,  138,  138,  138,  138,  110,   21,  146,  133,  133,
 /*    40 */   133,  133,  133,   73,  115,   25,   26,  133,   33,   73,
 /*    50 */   115,  133,  143,    4,  133,    5,  133,  141,   15,  133,
 /*    60 */    98,  133,  133,  141,  141,  141,  141,   24,  133,  133,
 /*    70 */   133,  133,  133,    7,  121,  143,  135,  135,  135,  135,
 /*    80 */   135,  121,  121,   52,   53,  135,   18,   49,   23,  135,
 /*    90 */   144,   88,  135,   20,  135,  131,   74,  135,  145,  135,
 /*   100 */   135,  131,  131,  131,  131,   99,  135,  135,  135,  135,
 /*   110 */   135,  101,  122,  144,  136,  136,  136,  136,  136,  122,
 /*   120 */   122,  145,  152,  136,   97,   63,  152,  136,   30,  105,
 /*   130 */   136,   50,  136,   48,   75,  136,   77,  136,  136,   76,
 /*   140 */    14,   60,   61,   62,  136,  136,  136,  136,  136,   90,
 /*   150 */   123,   91,  137,  137,  137,  137,  137,  123,  123,  153,
 /*   160 */   111,  137,  108,  153,  111,  137,  109,  102,  137,  103,
 /*   170 */   137,  118,   68,  137,   70,  137,  137,   67,  118,  118,
 /*   180 */   118,  116,  137,  137,  137,  137,  137,   71,  124,   10,
 /*   190 */   139,  139,  139,  139,  139,  124,  124,  112,   28,  139,
 /*   200 */    51,  112,  163,  139,   78,   81,  139,   85,  139,  164,
 /*   210 */    80,  139,   41,  139,  139,   82,  164,  164,  164,   84,
 /*   220 */   139,  139,  139,  139,  139,   86,   19,   42,  134,  134,
 /*   230 */   134,  134,  134,   89,   40,   93,  130,  134,   47,  126,
 /*   240 */    35,  134,  129,   92,  134,   45,  134,   95,  147,  134,
 /*   250 */   100,  134,  134,   46,   58,  126,  126,  126,  134,  134,
 /*   260 */   134,  134,  134,   59,  158,   69,  117,  117,  117,  155,
 /*   270 */    79,  104,  107,   72,  155,   83,  155,  155,  155,   43,
 /*   280 */    32,   44,   87,  160,   29,  127,  157,   94,   31,   39,
 /*   290 */   160,  160,  160,  157,  157,  157,   54,   55,   56,   57,
 /*   300 */    17,  127,  127,  127,  132,  132,  132,  154,  132,  167,
 /*   310 */   167,  167,  154,  132,  154,  154,  154,  132,  167,  167,
 /*   320 */   132,  156,  132,  167,  167,  132,  156,  132,  156,  156,
 /*   330 */   156,  167,  167,  167,  132,  132,  132,  132,  132,  167,
 /*   340 */   167,  167,  128,  128,  128,  159,  128,  149,  149,  149,
 /*   350 */   149,  128,  159,  159,  159,  128,  167,  167,  128,  167,
 /*   360 */   128,  167,  167,  128,  167,  128,  167,  167,  150,  150,
 /*   370 */   150,  150,  128,  128,  128,  128,  128,  167,  167,  167,
 /*   380 */   117,  117,  117,   27,   79,  167,  167,  167,  167,   83,
 /*   390 */   167,  167,    8,   13,   12,    9,   87,  167,   29,   22,
 /*   400 */   167,   94,  167,  167,  167,   96,   16,  167,  167,  167,
 /*   410 */    54,   55,   56,   57,   17,  167,  167,  167,  117,  117,
 /*   420 */   117,  167,   79,  167,  167,  167,  167,   83,  151,  151,
 /*   430 */   151,  151,  167,  167,   87,  167,   29,  167,  167,   94,
 /*   440 */   167,   34,  167,  167,  167,  167,  167,  167,   54,   55,
 /*   450 */    56,   57,   17,  167,  167,  167,  117,  117,  117,  167,
 /*   460 */    79,  167,  167,  167,  167,   83,  167,  167,  167,  167,
 /*   470 */   167,  167,   87,  167,   29,  167,  167,   94,  167,  167,
 /*   480 */    36,  167,  167,  167,  167,  167,   54,   55,   56,   57,
 /*   490 */    17,  167,  167,  167,  117,  117,  117,  167,   79,  167,
 /*   500 */   167,  167,  167,   83,  167,  167,  167,  167,  167,  167,
 /*   510 */    87,  167,   29,  167,  167,   94,  167,  167,   37,  167,
 /*   520 */   167,  167,  167,  167,   54,   55,   56,   57,   17,  167,
 /*   530 */   167,  167,  132,  132,  132,  132,  132,  167,  167,  167,
 /*   540 */   167,  132,  167,  167,  167,  167,  167,  167,  132,  167,
 /*   550 */   132,  167,  167,  132,  167,  167,  167,  167,  167,  167,
 /*   560 */   167,  167,  132,  132,  132,  132,  132,  167,  167,  167,
 /*   570 */   132,  132,  132,  167,  132,  167,  167,  167,  167,  132,
 /*   580 */   167,  167,  167,  167,  167,  167,  132,  167,  132,  167,
 /*   590 */   167,  132,  167,  132,  167,  167,  167,  167,  167,  167,
 /*   600 */   132,  132,  132,  132,  132,  167,  167,  167,  132,  132,
 /*   610 */   132,  167,  132,  167,  167,  167,  167,  132,  167,  167,
 /*   620 */   167,  167,  167,  167,  132,  167,  132,  167,  167,  132,
 /*   630 */   167,  167,  132,  167,  167,  167,  167,  167,  132,  132,
 /*   640 */   132,  132,  132,  167,  167,  167,  119,  119,  119,  167,
 /*   650 */   119,  167,  167,  167,  167,  119,  167,  167,  167,  167,
 /*   660 */   167,  167,  119,  167,  119,  167,  167,  119,  167,  167,
 /*   670 */   119,  167,  167,  167,  167,  167,  119,  119,  119,  119,
 /*   680 */   119,  167,  167,  167,  120,  120,  120,  167,  120,  167,
 /*   690 */   167,  167,  167,  120,  167,  167,  167,  167,  167,  167,
 /*   700 */   120,  167,  120,  167,  167,  120,  167,  167,  120,  167,
 /*   710 */   167,  167,  167,  167,  120,  120,  120,  120,  120,  167,
 /*   720 */   167,  167,  125,  125,  125,  113,  125,  167,  113,  167,
 /*   730 */   167,  125,  167,  113,  113,  167,  167,  167,  125,  117,
 /*   740 */   125,  167,  106,  125,  167,  125,  167,  117,  117,  167,
 /*   750 */   167,  167,  125,  125,  125,  125,  125,  167,  167,  167,
 /*   760 */   167,  167,  113,  113,  113,  113,  113,  167,  167,  167,
 /*   770 */   167,  167,  167,  167,  167,  167,   54,   55,   56,   57,
 /*   780 */    17,  114,  167,  167,  114,  167,  167,  167,  167,  114,
 /*   790 */   114,  167,    8,   13,   12,    9,   11,  167,  167,   22,
 /*   800 */   167,  167,  167,  167,  167,   96,   16,  167,  161,  167,
 /*   810 */   161,  161,  167,  167,  161,  161,  161,  161,  114,  114,
 /*   820 */   114,  114,  114,  167,  142,  167,   64,   65,  167,  167,
 /*   830 */   142,  142,  142,  142,  167,  167,  167,  140,  167,  140,
 /*   840 */   140,  167,  167,  140,  140,  140,  140,  162,  167,  162,
 /*   850 */   162,  167,  167,  162,  162,  162,  162,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     7,    8,    9,   10,   11,   32,   33,   34,   35,   16,
 /*    10 */    33,   34,   35,   20,   54,   55,   23,   59,   25,   31,
 /*    20 */    59,   28,   59,   30,   31,   37,   38,   39,   40,   49,
 /*    30 */    37,   38,   39,   40,   41,    0,   56,   13,    7,    8,
 /*    40 */     9,   10,   11,    8,    9,   45,   46,   16,    7,    8,
 /*    50 */     9,   20,   13,   59,   23,   59,   25,   31,   66,   28,
 /*    60 */    36,   30,   31,   37,   38,   39,   40,   45,   37,   38,
 /*    70 */    39,   40,   41,   64,   14,   36,    7,    8,    9,   10,
 /*    80 */    11,   21,   22,   21,   22,   16,   68,   44,   43,   20,
 /*    90 */    13,   43,   23,   47,   25,   31,   58,   28,   13,   30,
 /*   100 */    31,   37,   38,   39,   40,   48,   37,   38,   39,   40,
 /*   110 */    41,   48,   14,   36,    7,    8,    9,   10,   11,   21,
 /*   120 */    22,   36,    2,   16,   49,    1,    6,   20,   48,    2,
 /*   130 */    23,   60,   25,    6,   11,   28,   70,   30,   31,   13,
 /*   140 */    65,   17,   18,   19,   37,   38,   39,   40,   41,    1,
 /*   150 */    14,   61,    7,    8,    9,   10,   11,   21,   22,    2,
 /*   160 */     2,   16,    2,    6,    6,   20,    6,   13,   23,   69,
 /*   170 */    25,    0,   67,   28,    1,   30,   31,    1,    7,    8,
 /*   180 */     9,    9,   37,   38,   39,   40,   41,   57,   14,    9,
 /*   190 */     7,    8,    9,   10,   11,   21,   22,    2,   12,   16,
 /*   200 */     7,    6,   14,   20,   14,   13,   23,   13,   25,    0,
 /*   210 */    12,   28,   15,   30,   31,   14,    7,    8,    9,   12,
 /*   220 */    37,   38,   39,   40,   41,   14,   12,   15,    7,    8,
 /*   230 */     9,   10,   11,   14,   24,   27,    7,   16,   15,    1,
 /*   240 */     7,   20,   26,   26,   23,   32,   25,   29,   13,   28,
 /*   250 */    13,   30,   31,    7,   13,   17,   18,   19,   37,   38,
 /*   260 */    39,   40,   41,   13,   28,    1,    7,    8,    9,    0,
 /*   270 */    11,   28,    4,    1,    5,   16,    7,    8,    9,   20,
 /*   280 */     5,    7,   23,    0,   25,    1,    0,   28,    5,   30,
 /*   290 */     7,    8,    9,    7,    8,    9,   37,   38,   39,   40,
 /*   300 */    41,   17,   18,   19,    7,    8,    9,    0,   11,   71,
 /*   310 */    71,   71,    5,   16,    7,    8,    9,   20,   71,   71,
 /*   320 */    23,    0,   25,   71,   71,   28,    5,   30,    7,    8,
 /*   330 */     9,   71,   71,   71,   37,   38,   39,   40,   41,   71,
 /*   340 */    71,   71,    7,    8,    9,    0,   11,   32,   33,   34,
 /*   350 */    35,   16,    7,    8,    9,   20,   71,   71,   23,   71,
 /*   360 */    25,   71,   71,   28,   71,   30,   71,   71,   32,   33,
 /*   370 */    34,   35,   37,   38,   39,   40,   41,   71,   71,   71,
 /*   380 */     7,    8,    9,   10,   11,   71,   71,   71,   71,   16,
 /*   390 */    71,   71,   49,   50,   51,   52,   23,   71,   25,   56,
 /*   400 */    71,   28,   71,   71,   71,   62,   63,   71,   71,   71,
 /*   410 */    37,   38,   39,   40,   41,   71,   71,   71,    7,    8,
 /*   420 */     9,   71,   11,   71,   71,   71,   71,   16,   32,   33,
 /*   430 */    34,   35,   71,   71,   23,   71,   25,   71,   71,   28,
 /*   440 */    71,   30,   71,   71,   71,   71,   71,   71,   37,   38,
 /*   450 */    39,   40,   41,   71,   71,   71,    7,    8,    9,   71,
 /*   460 */    11,   71,   71,   71,   71,   16,   71,   71,   71,   71,
 /*   470 */    71,   71,   23,   71,   25,   71,   71,   28,   71,   71,
 /*   480 */    31,   71,   71,   71,   71,   71,   37,   38,   39,   40,
 /*   490 */    41,   71,   71,   71,    7,    8,    9,   71,   11,   71,
 /*   500 */    71,   71,   71,   16,   71,   71,   71,   71,   71,   71,
 /*   510 */    23,   71,   25,   71,   71,   28,   71,   71,   31,   71,
 /*   520 */    71,   71,   71,   71,   37,   38,   39,   40,   41,   71,
 /*   530 */    71,   71,    7,    8,    9,   10,   11,   71,   71,   71,
 /*   540 */    71,   16,   71,   71,   71,   71,   71,   71,   23,   71,
 /*   550 */    25,   71,   71,   28,   71,   71,   71,   71,   71,   71,
 /*   560 */    71,   71,   37,   38,   39,   40,   41,   71,   71,   71,
 /*   570 */     7,    8,    9,   71,   11,   71,   71,   71,   71,   16,
 /*   580 */    71,   71,   71,   71,   71,   71,   23,   71,   25,   71,
 /*   590 */    71,   28,   71,   30,   71,   71,   71,   71,   71,   71,
 /*   600 */    37,   38,   39,   40,   41,   71,   71,   71,    7,    8,
 /*   610 */     9,   71,   11,   71,   71,   71,   71,   16,   71,   71,
 /*   620 */    71,   71,   71,   71,   23,   71,   25,   71,   71,   28,
 /*   630 */    71,   71,   31,   71,   71,   71,   71,   71,   37,   38,
 /*   640 */    39,   40,   41,   71,   71,   71,    7,    8,    9,   71,
 /*   650 */    11,   71,   71,   71,   71,   16,   71,   71,   71,   71,
 /*   660 */    71,   71,   23,   71,   25,   71,   71,   28,   71,   71,
 /*   670 */    31,   71,   71,   71,   71,   71,   37,   38,   39,   40,
 /*   680 */    41,   71,   71,   71,    7,    8,    9,   71,   11,   71,
 /*   690 */    71,   71,   71,   16,   71,   71,   71,   71,   71,   71,
 /*   700 */    23,   71,   25,   71,   71,   28,   71,   71,   31,   71,
 /*   710 */    71,   71,   71,   71,   37,   38,   39,   40,   41,   71,
 /*   720 */    71,   71,    7,    8,    9,    0,   11,   71,    3,   71,
 /*   730 */    71,   16,   71,    8,    9,   71,   71,   71,   23,    0,
 /*   740 */    25,   71,    3,   28,   71,   30,   71,    8,    9,   71,
 /*   750 */    71,   71,   37,   38,   39,   40,   41,   71,   71,   71,
 /*   760 */    71,   71,   37,   38,   39,   40,   41,   71,   71,   71,
 /*   770 */    71,   71,   71,   71,   71,   71,   37,   38,   39,   40,
 /*   780 */    41,    0,   71,   71,    3,   71,   71,   71,   71,    8,
 /*   790 */     9,   71,   49,   50,   51,   52,   53,   71,   71,   56,
 /*   800 */    71,   71,   71,   71,   71,   62,   63,   71,   31,   71,
 /*   810 */    33,   34,   71,   71,   37,   38,   39,   40,   37,   38,
 /*   820 */    39,   40,   41,   71,   31,   71,   33,   34,   71,   71,
 /*   830 */    37,   38,   39,   40,   71,   71,   71,   31,   71,   33,
 /*   840 */    34,   71,   71,   37,   38,   39,   40,   31,   71,   33,
 /*   850 */    34,   71,   71,   37,   38,   39,   40,
};
#define YY_SHIFT_USE_DFLT (-28)
#define YY_SHIFT_MAX 109
static const short yy_shift_ofst[] = {
 /*     0 */   725,  259,  373,  411,  449,  487,  739,  -12,  -23,  297,
 /*    10 */   525,  563,  601,  601,  777,  793,   26,  269,  283,  124,
 /*    20 */   124,   35,   41,   62,   24,   24,   24,  123,  126,  148,
 /*    30 */   154,  176,  173,   -7,   31,   69,  107,  145,  183,  221,
 /*    40 */   335,  639,  677,  715,  781,  806,  816,   64,  307,  321,
 /*    50 */   171,  209,  238,  284,  -27,  315,  336,  396,  286,  345,
 /*    60 */    60,   98,  136,  174,   39,   77,   85,  120,  127,  157,
 /*    70 */   158,  160,  195,  172,  180,  186,  188,  190,  193,  198,
 /*    80 */   192,  201,  197,  207,  194,  211,  212,  214,  219,  210,
 /*    90 */   216,  217,  208,  229,  218,  223,  233,  213,  235,  237,
 /*   100 */   246,  241,  236,  243,  250,  264,  268,  275,  272,  274,
};
#define YY_REDUCE_USE_DFLT (-43)
#define YY_REDUCE_MAX 32
static const short yy_reduce_ofst[] = {
 /*     0 */   -40,  743,  343,  343,  343,  343,  -20,   75,    0,  -42,
 /*    10 */   -39,  -37,   -6,   -4,   -8,   22,    9,   18,   43,   45,
 /*    20 */    48,   38,   38,   46,   57,   63,   80,   71,   66,   90,
 /*    30 */   100,  105,  130,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    10 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    20 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    30 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    40 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    50 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    60 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    70 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    80 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*    90 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
 /*   100 */   165,  165,  165,  165,  165,  165,  165,  165,  165,  165,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "IP",            "COMMA",         "AT",          
  "DECLARE",       "LBRACES",       "RBRACES",       "END_EXPR",    
  "HOST_LABEL",    "START",         "STOP",          "AFTER",       
  "LPAREN",        "NUMBER",        "RPAREN",        "DO",          
  "WHILE",         "IP_VAR",        "DEVICE_VAR",    "QUOTED_STRING",
  "ELSE",          "IS_EQUAL",      "IS_NOT_EQUAL",  "IF",          
  "THEN",          "SET",           "ARROW",         "STRING",      
  "FOR",           "EACH",          "END_IF",        "END",         
  "COLON",         "DROP",          "DUPLICATE",     "DELAY",       
  "PROGRESSIVE",   "SCTP",          "TCP",           "UDP",         
  "ALL",           "PARTITION",     "error",         "var_expr",    
  "network",       "action",        "action2",       "op_type",     
  "progressive_opt",  "protocol",      "while_expr",    "after_expr",  
  "if_expr",       "else_expr",     "program",       "decl_network",
  "commands",      "host_list",     "host_label_opt",  "expr",        
  "stop_expr",     "ip_expr",       "set_expr",      "foreach_expr",
  "expr2",         "protocol_label",  "commands_2",    "network_list",
  "network_opt",   "num_aux",       "stop_num",    
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "program ::= decl_network commands",
 /*   1 */ "host_list ::= IP",
 /*   2 */ "host_list ::= host_list COMMA IP",
 /*   3 */ "decl_network ::=",
 /*   4 */ "decl_network ::= decl_network AT DECLARE LBRACES host_list RBRACES END_EXPR",
 /*   5 */ "host_label_opt ::=",
 /*   6 */ "host_label_opt ::= HOST_LABEL",
 /*   7 */ "commands ::=",
 /*   8 */ "commands ::= commands host_label_opt START expr STOP stop_expr",
 /*   9 */ "after_expr ::= AFTER LPAREN NUMBER RPAREN DO",
 /*  10 */ "while_expr ::= WHILE LPAREN NUMBER RPAREN DO",
 /*  11 */ "var_expr ::= IP_VAR",
 /*  12 */ "var_expr ::= DEVICE_VAR",
 /*  13 */ "var_expr ::= QUOTED_STRING",
 /*  14 */ "var_expr ::= IP",
 /*  15 */ "else_expr ::= ELSE",
 /*  16 */ "op_type ::= IS_EQUAL",
 /*  17 */ "op_type ::= IS_NOT_EQUAL",
 /*  18 */ "if_expr ::= IF LPAREN var_expr op_type var_expr RPAREN THEN",
 /*  19 */ "ip_expr ::= IP",
 /*  20 */ "set_expr ::= SET ip_expr ARROW STRING",
 /*  21 */ "foreach_expr ::= FOR EACH DO",
 /*  22 */ "expr ::=",
 /*  23 */ "expr ::= expr if_expr expr else_expr expr END_IF",
 /*  24 */ "expr ::= expr if_expr expr END_IF",
 /*  25 */ "expr ::= expr set_expr END_EXPR",
 /*  26 */ "expr ::= expr after_expr expr END",
 /*  27 */ "expr ::= expr while_expr expr END",
 /*  28 */ "expr ::= expr commands END_EXPR",
 /*  29 */ "expr ::= expr foreach_expr expr2 END",
 /*  30 */ "protocol_label ::= protocol COLON",
 /*  31 */ "expr2 ::=",
 /*  32 */ "expr2 ::= expr2 protocol_label commands_2",
 /*  33 */ "action ::= DROP",
 /*  34 */ "action ::= DUPLICATE",
 /*  35 */ "action2 ::= DELAY",
 /*  36 */ "progressive_opt ::=",
 /*  37 */ "progressive_opt ::= PROGRESSIVE",
 /*  38 */ "protocol ::= SCTP",
 /*  39 */ "protocol ::= TCP",
 /*  40 */ "protocol ::= UDP",
 /*  41 */ "protocol ::= ALL",
 /*  42 */ "network_list ::= IP",
 /*  43 */ "network_list ::= network_list COMMA IP",
 /*  44 */ "network ::= LBRACES network_list RBRACES",
 /*  45 */ "network_opt ::=",
 /*  46 */ "network_opt ::= network_opt network",
 /*  47 */ "commands ::= protocol action progressive_opt NUMBER",
 /*  48 */ "num_aux ::= NUMBER",
 /*  49 */ "commands ::= protocol action2 progressive_opt num_aux FOR NUMBER",
 /*  50 */ "commands ::= PARTITION network_opt",
 /*  51 */ "commands_2 ::=",
 /*  52 */ "commands_2 ::= commands_2 action progressive_opt NUMBER END_EXPR",
 /*  53 */ "stop_num ::= NUMBER",
 /*  54 */ "stop_expr ::= AFTER LPAREN stop_num RPAREN END_EXPR",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  ParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    case 43: /* var_expr */
    case 44: /* network */
{
#line 46 "parser.y"
 FREE_OP((yypminor->yy0)); 
#line 633 "parser.c"
}
      break;
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int ParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
//  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      int iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
//  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
//  assert( i!=YY_REDUCE_USE_DFLT );
//  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
//  assert( i>=0 && i<YY_SZ_ACTTAB );
//  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
#line 39 "parser.y"

	printf("- stack!\n");
#line 810 "parser.c"
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 54, 2 },
  { 57, 1 },
  { 57, 3 },
  { 55, 0 },
  { 55, 7 },
  { 58, 0 },
  { 58, 1 },
  { 56, 0 },
  { 56, 6 },
  { 51, 5 },
  { 50, 5 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 53, 1 },
  { 47, 1 },
  { 47, 1 },
  { 52, 7 },
  { 61, 1 },
  { 62, 4 },
  { 63, 3 },
  { 59, 0 },
  { 59, 6 },
  { 59, 4 },
  { 59, 3 },
  { 59, 4 },
  { 59, 4 },
  { 59, 3 },
  { 59, 4 },
  { 65, 2 },
  { 64, 0 },
  { 64, 3 },
  { 45, 1 },
  { 45, 1 },
  { 46, 1 },
  { 48, 0 },
  { 48, 1 },
  { 49, 1 },
  { 49, 1 },
  { 49, 1 },
  { 49, 1 },
  { 67, 1 },
  { 67, 3 },
  { 44, 3 },
  { 68, 0 },
  { 68, 2 },
  { 56, 4 },
  { 69, 1 },
  { 56, 6 },
  { 56, 2 },
  { 66, 0 },
  { 66, 5 },
  { 70, 1 },
  { 60, 5 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* program ::= decl_network commands */
      case 3: /* decl_network ::= */
      case 7: /* commands ::= */
      case 8: /* commands ::= commands host_label_opt START expr STOP stop_expr */
      case 22: /* expr ::= */
      case 25: /* expr ::= expr set_expr END_EXPR */
      case 28: /* expr ::= expr commands END_EXPR */
      case 29: /* expr ::= expr foreach_expr expr2 END */
      case 31: /* expr2 ::= */
      case 32: /* expr2 ::= expr2 protocol_label commands_2 */
      case 45: /* network_opt ::= */
      case 51: /* commands_2 ::= */
#line 62 "parser.y"
{
}
#line 991 "parser.c"
        break;
      case 1: /* host_list ::= IP */
      case 2: /* host_list ::= host_list COMMA IP */
#line 65 "parser.y"
{
		pdata->ips.arr[pdata->ips.num++].ip = yymsp[0].minor.yy0->data.num;
	}
#line 999 "parser.c"
        break;
      case 4: /* decl_network ::= decl_network AT DECLARE LBRACES host_list RBRACES END_EXPR */
#line 74 "parser.y"
{
		NEW_OP(DECLARE);
		SET_OP_ARRAY(pdata);
		ADD_COMMAND();
	}
#line 1008 "parser.c"
        break;
      case 5: /* host_label_opt ::= */
#line 81 "parser.y"
{
		pdata->label = pdata->in_label = pdata->occur_type = 0;
	}
#line 1015 "parser.c"
        break;
      case 6: /* host_label_opt ::= HOST_LABEL */
#line 85 "parser.y"
{
		NEW_OP(HOST);
		op->occur_type = pdata->occur_type = 0;
		pdata->in_label = 1;
		pdata->label = yymsp[0].minor.yy0->data.num;
		SET_OP(yymsp[0].minor.yy0);
		ADD_COMMAND();
	}
#line 1027 "parser.c"
        break;
      case 9: /* after_expr ::= AFTER LPAREN NUMBER RPAREN DO */
#line 98 "parser.y"
{
		NEW_OP(AFTER);
		SET_OP(yymsp[-2].minor.yy0);
		op->occur_type = pdata->occur_type = yymsp[-2].minor.yy0->num_type;
		ADD_COMMAND();
		
		yygotominor.yy136 = OP_NUM();
	}
#line 1039 "parser.c"
        break;
      case 10: /* while_expr ::= WHILE LPAREN NUMBER RPAREN DO */
#line 108 "parser.y"
{
		NEW_OP(WHILE);
		SET_OP(yymsp[-2].minor.yy0);
		op->occur_type = pdata->occur_type = yymsp[-2].minor.yy0->num_type;
		ADD_COMMAND();
		
		yygotominor.yy136 = OP_NUM();
	}
#line 1051 "parser.c"
        break;
      case 11: /* var_expr ::= IP_VAR */
      case 12: /* var_expr ::= DEVICE_VAR */
      case 14: /* var_expr ::= IP */
      case 19: /* ip_expr ::= IP */
      case 48: /* num_aux ::= NUMBER */
      case 53: /* stop_num ::= NUMBER */
#line 118 "parser.y"
{
		ALLOC_DATA(yygotominor.yy0);
		COPY_DATA(yygotominor.yy0, yymsp[0].minor.yy0);
	}
#line 1064 "parser.c"
        break;
      case 13: /* var_expr ::= QUOTED_STRING */
#line 128 "parser.y"
{
		ALLOC_DATA(yygotominor.yy0);
		COPY_DATA(yygotominor.yy0, yymsp[0].minor.yy0);
		FREE_STR(yymsp[0].minor.yy0);
	}
#line 1073 "parser.c"
        break;
      case 15: /* else_expr ::= ELSE */
#line 140 "parser.y"
{
		NEW_OP(ELSE);
		ADD_COMMAND();

		yygotominor.yy136 = OP_NUM();
	}
#line 1083 "parser.c"
        break;
      case 16: /* op_type ::= IS_EQUAL */
#line 147 "parser.y"
{ yygotominor.yy136 = OP_IS_EQUAL; }
#line 1088 "parser.c"
        break;
      case 17: /* op_type ::= IS_NOT_EQUAL */
#line 148 "parser.y"
{ yygotominor.yy136 = OP_IS_NOT_EQUAL; }
#line 1093 "parser.c"
        break;
      case 18: /* if_expr ::= IF LPAREN var_expr op_type var_expr RPAREN THEN */
#line 151 "parser.y"
{
		NEW_OP(IF);
		COPY_TO_OP(yymsp[-4].minor.yy0);
		COPY_TO_OP(yymsp[-2].minor.yy0);

		op->extended_value = yymsp[-3].minor.yy136;
		ADD_COMMAND();

		yygotominor.yy136 = OP_NUM();
	}
#line 1107 "parser.c"
        break;
      case 20: /* set_expr ::= SET ip_expr ARROW STRING */
#line 169 "parser.y"
{
		NEW_OP(SET);
		COPY_TO_OP(yymsp[-2].minor.yy0);
		SET_OP(yymsp[0].minor.yy0);
		ADD_COMMAND();
		FREE_STR(yymsp[0].minor.yy0);
	}
#line 1118 "parser.c"
        break;
      case 21: /* foreach_expr ::= FOR EACH DO */
#line 178 "parser.y"
{
		NEW_OP(FOREACH);
		op->occur_type = pdata->occur_type = ALL_PROTOCOL;
		ADD_COMMAND();
	}
#line 1127 "parser.c"
        break;
      case 23: /* expr ::= expr if_expr expr else_expr expr END_IF */
#line 186 "parser.y"
{
		NEW_OP(END_IF);	
		ADD_COMMAND();
		SET_NEXT_OP(yymsp[-4].minor.yy136, yymsp[-2].minor.yy136);
		SET_NEXT_OP(yymsp[-2].minor.yy136, OP_NUM());
	}
#line 1137 "parser.c"
        break;
      case 24: /* expr ::= expr if_expr expr END_IF */
#line 193 "parser.y"
{
		NEW_OP(END_IF);
		ADD_COMMAND();
		SET_NEXT_OP(yymsp[-2].minor.yy136, OP_NUM());
	}
#line 1146 "parser.c"
        break;
      case 26: /* expr ::= expr after_expr expr END */
      case 27: /* expr ::= expr while_expr expr END */
#line 200 "parser.y"
{
		NEW_OP(END);
		ADD_COMMAND();
		SET_NEXT_OP(yymsp[-2].minor.yy136, OP_NUM());
	}
#line 1156 "parser.c"
        break;
      case 30: /* protocol_label ::= protocol COLON */
#line 215 "parser.y"
{
		pdata->protocol = yymsp[-1].minor.yy136;
	}
#line 1163 "parser.c"
        break;
      case 33: /* action ::= DROP */
#line 222 "parser.y"
{ yygotominor.yy136 = ACT_DROP; }
#line 1168 "parser.c"
        break;
      case 34: /* action ::= DUPLICATE */
#line 223 "parser.y"
{ yygotominor.yy136 = ACT_DUPLICATE; }
#line 1173 "parser.c"
        break;
      case 35: /* action2 ::= DELAY */
#line 225 "parser.y"
{ yygotominor.yy136 = ACT_DELAY; }
#line 1178 "parser.c"
        break;
      case 36: /* progressive_opt ::= */
#line 227 "parser.y"
{ yygotominor.yy136 = 0; }
#line 1183 "parser.c"
        break;
      case 37: /* progressive_opt ::= PROGRESSIVE */
#line 228 "parser.y"
{ yygotominor.yy136 = 1; }
#line 1188 "parser.c"
        break;
      case 38: /* protocol ::= SCTP */
#line 230 "parser.y"
{ yygotominor.yy136 = SCTP_PROTOCOL; }
#line 1193 "parser.c"
        break;
      case 39: /* protocol ::= TCP */
#line 231 "parser.y"
{ yygotominor.yy136 = TCP_PROTOCOL; }
#line 1198 "parser.c"
        break;
      case 40: /* protocol ::= UDP */
#line 232 "parser.y"
{ yygotominor.yy136 = UDP_PROTOCOL; }
#line 1203 "parser.c"
        break;
      case 41: /* protocol ::= ALL */
#line 233 "parser.y"
{ yygotominor.yy136 = ALL_PROTOCOL; }
#line 1208 "parser.c"
        break;
      case 42: /* network_list ::= IP */
      case 43: /* network_list ::= network_list COMMA IP */
#line 236 "parser.y"
{
		pdata->ip_tmp.ips[pdata->ip_tmp.num++] = yymsp[0].minor.yy0->data.num;
		CHECK_USED_IP(yymsp[0].minor.yy0->data.num);
	}
#line 1217 "parser.c"
        break;
      case 44: /* network ::= LBRACES network_list RBRACES */
#line 246 "parser.y"
{
		ALLOC_DATA(yygotominor.yy0);
		yygotominor.yy0->type = ARRAY;
		yygotominor.yy0->data.array.count = pdata->ip_tmp.num;
		memcpy(yygotominor.yy0->data.array.nums, pdata->ip_tmp.ips, sizeof(unsigned long) * pdata->ip_tmp.num);
		
		pdata->ip_tmp.num = 0;
		memset(pdata->ip_tmp.ips, 0, sizeof(pdata->ip_tmp.ips));
	}
#line 1230 "parser.c"
        break;
      case 46: /* network_opt ::= network_opt network */
#line 257 "parser.y"
{		
		pdata->partition[pdata->part_num].count = yymsp[0].minor.yy0->data.array.count;
		memcpy(pdata->partition[pdata->part_num].ips, yymsp[0].minor.yy0->data.array.nums, sizeof(unsigned long) * yymsp[0].minor.yy0->data.array.count);
		pdata->part_num++;
		
		FREE_OP(yymsp[0].minor.yy0);
	}
#line 1241 "parser.c"
        break;
      case 47: /* commands ::= protocol action progressive_opt NUMBER */
#line 266 "parser.y"
{
		NEW_OP(COMMAND);
		op->protocol = yymsp[-3].minor.yy136;
		SET_OP_EXPR(NUMBER, yymsp[-2].minor.yy136);
		SET_OP(yymsp[0].minor.yy0);
		op->extended_value = yymsp[-1].minor.yy136;
		ADD_COMMAND();
	}
#line 1253 "parser.c"
        break;
      case 49: /* commands ::= protocol action2 progressive_opt num_aux FOR NUMBER */
#line 280 "parser.y"
{
		NEW_OP(COMMAND);
		op->protocol = yymsp[-5].minor.yy136;
		SET_OP_EXPR(NUMBER, yymsp[-4].minor.yy136);
		SET_OP(yymsp[-2].minor.yy0);
		SET_OP(yymsp[0].minor.yy0);
		op->extended_value = yymsp[-3].minor.yy136;
		ADD_COMMAND();
	}
#line 1266 "parser.c"
        break;
      case 50: /* commands ::= PARTITION network_opt */
#line 290 "parser.y"
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
#line 1291 "parser.c"
        break;
      case 52: /* commands_2 ::= commands_2 action progressive_opt NUMBER END_EXPR */
#line 313 "parser.y"
{
		NEW_OP(COMMAND);
		op->protocol = pdata->protocol;
		SET_OP_EXPR(NUMBER, yymsp[-3].minor.yy136);
		SET_OP(yymsp[-1].minor.yy0);
		op->extended_value = yymsp[-2].minor.yy136;	
		ADD_COMMAND();
	}
#line 1303 "parser.c"
        break;
      case 54: /* stop_expr ::= AFTER LPAREN stop_num RPAREN END_EXPR */
#line 329 "parser.y"
{
		NEW_OP(AFTER);
		op->protocol = 0;
		SET_OP(yymsp[-2].minor.yy0);
		op->occur_type = yymsp[-2].minor.yy0->num_type;
		op->block_type = 1;
		FREE_OP(yymsp[-2].minor.yy0);
		ADD_COMMAND();
	}
#line 1316 "parser.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = yyact;
      yymsp->major = yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
//    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
#line 36 "parser.y"

	printf("- parse failure\n");
#line 1366 "parser.c"
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 33 "parser.y"

	pdata->error = 1;
#line 1385 "parser.c"
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,yymajor);
    if( yyact<YYNSTATE ){
//      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
//      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"
#include "parser.h"
#include "lemon_parser.h"

/*
 * Print error message
 */
static void faultload_error(parser_data *pdata, scanner_state **state)
{
	switch (pdata->error) {
		case 1:
			printf("Syntax error before: '%.*s...' on line %d\n", 15, (*state)->end, pdata->line);
			break;
		case 2:
			printf("Undeclared ip found on line %d\n", pdata->line);
			break;
		case 3:
			printf("Unknown character '%.*s' on line %d\n", 1, (*state)->end, pdata->line);
			break;
		default:
			if (pdata->error) {
				printf("Unknown error on line %d\n", pdata->line);
			}
			break;
	}	
}

/*
 * Parsing
 */
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
				pdata.error = 3;
				faultload_error(&pdata, &state);
				return NULL;
			default:
				if (tok) {
					if (pdata.error) {
						faultload_error(&pdata, &state);
						return NULL;
					}
					CALL_PARSER();
				} else {
					pdata.error = -1;
					faultload_error(&pdata, &state);
					return NULL;
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

	PARSER_FREE();
	
	return pdata.commands;
}
