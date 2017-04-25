/********************
    This file is part of the software library CADLIB written by Conrad Ziesler
    Copyright 2003, Conrad Ziesler, all rights reserved.

*************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

******************/
/* scanner.h,  header for scanner routines
   Conrad Ziesler
*/

#include <stdarg.h>

#ifndef __SCANNER_H__
#define __SCANNER_H__


#ifndef __MEMORY_H__
#include "memory.h"
#endif

#ifndef __NAMES_H__
#include "names.h"
#endif

/* we provide a consistent way for tools to read large text files
   of various line oriented formats
   
   in addition, we manage memory, so freeing these structures is simple
*/
typedef struct file_line_st
{
  unsigned int fileindex:8;
  unsigned int line:24;
}file_line_t;

typedef struct tokenmap_st
{
  int token;
  char *str;
}tokenmap_t;


typedef struct card_st
{
  struct card_st *next;
  int token;
  char *val;
  char str[4];
}card_t;

typedef struct deck_st
{
  struct deck_st *next;
  card_t *card;
  file_line_t line;
}deck_t;



typedef struct scanner_def_st
{
  char line_stop[32];     /* stop scanning when we hit this line            */
  char eol_continue[8];  /* continue line if this is detected at the end   */
  char bol_continue[8];  /* continue line if this is detected at beggining */
  int  convert_case;      /* convert everything to a case ?                 */
  char quote_char;        /* what to use as value quote                     */
  char newline;           /* what to use as new line character              */
  char assignment;        /* what to use as assignment '=' delimiter        */
  char tokenize[8];       /* prefix for all tokens of interest              */
  char whitespace[32];    /* ignore all of this                             */
  char commentstart[8];   /* this starts a comment */
}scanner_def_t;

typedef struct scanner_sect_st
{
  struct scanner_sect_st *back;
  names_t *tokenizer;     /* we use this as our hasher       */
  scanner_def_t *def;     /* we get our defaults from here   */

  card_t *cp;
  deck_t *dp,*dhead;
  int eolstring;         /* got = [space] EOLINE */
  char lbuf[512];         /* line buffer */
  char eoline[512];       /* continue buffer */
  int line_cont;
}scanner_sect_t;

typedef struct scanner_input_st
{
  struct scanner_input_st *back, *next;  /* list of all inputs*/
  void *file_fp; /* FILE *  */
  int line;
  int index;
}scanner_input_t;

typedef struct scanner_st
{
  memory_t strmem;          /* where we allocate our strings      */
  scanner_input_t *inputp;  /* current input stack, for #include files    */
  scanner_input_t *allinputs; /* list of all inputs */
  scanner_sect_t *sectp;    /* section stack, who is parsing this */
  /****** error processing ****/ 
  deck_t *errdeck;  
  card_t *errcard;
  void (*errfunc)(struct scanner_st *sp, char *format, va_list vp); 
  void (*warnfunc)(struct scanner_st *sp, char *format, va_list vp);
}scanner_t;

#define PARSE_CASE_TOLOWER 1
#define PARSE_CASE_TOUPPER 2
#define PARSE_NOCASECVT 0 


/****  initialization  and release ******/
void scanner_add_tokens(scanner_t *scan, tokenmap_t *map);
void scanner_reset_tokens(scanner_t *scan);

void scanner_init(scanner_t *scan);    /* init all structures */
void scanner_release(scanner_t *scan); /* free all associated structures */

void scanner_input_newfp(scanner_t *scan, void *fp);
void scanner_sect_new(scanner_t *scan, scanner_def_t *defs, tokenmap_t *map);
void scanner_input_release(scanner_t *scan); /* release top of stack */
void scanner_sect_release(scanner_t *scan);  /* release top of stack */

/* some standardized scanner defaults */
scanner_def_t *scanner_def_spice(void);
void scanner_def_err(scanner_t *scan, char *fmt, va_list vp);

/* the scanner functions, file section oriented  
   keeps allocating memory, so you get roughly the file size + about 25% overhead
   of memory usage.  free_all dumps it all quickly, though.
*/

int scanner_parse_all(scanner_t *scan);
void scanner_free_all(scanner_t *scan); /* free all string (&card/&deck) memory */ 
int scanner_parse_line(scanner_t *scan);
unsigned parse_binary(char **str);

/* misc. */

void parse_error (scanner_t *scan, char *fmt, ...);
void parse_warn  (scanner_t *scan, char *fmt, ...);
int scanner_checkvalid(deck_t *d, int qty);
void scanner_debug_all(scanner_t *scan, void *dbg);
file_line_t scanner_current_file_line(scanner_t *scan);


#endif
