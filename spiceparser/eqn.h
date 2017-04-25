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
/* eqn.h, equation parsing and evaluation 
   Conrad Ziesler
*/

#define __EQN_H__

/* defining equations eliminates expression parsing 
   and roughly halves memory needed for storage of numerics
   however you can no longer use parameters.
*/

/* #define NO_EQUATIONS */

/*#ifndef __FILE_H__
#include "file.h"
#endif
*/

#ifndef __LIST_H__
#include "list.h"
#endif

#ifndef __NAMES_H__
#include "names.h"
#endif

#ifndef __HASH_H__
#include "hash.h"
#endif


typedef enum itemop_e
{
  OPeolist,
  OPeol_const,  /* special terminator, for constant (reduced memory usage) */
  OPeol_valp,  /* special terminator, for indirection (float *) */
  OPeol_qty,   /* for simplified decoding of eol */

  OPlit,
  OPlitm,
  OPlitv,
  OPlitvm,

  OPval,
  OPvalm,
  OPadd,
  OPsub,

  OPmul,
  OPdiv,
  OPexp,
  OPopen,

  OPclose
}itemop_t;

#define OPsyms  { "OPeolist", "OPeol_const", "OPeol_valp", "OPeol_qty",	\
                  "OPlit", "OPlitm", "OPlitv", "OPlitvm",		\
                  "OPval", "OPvalm", "OPadd", "OPsub",			\
                  "OPmul", "OPdiv", "OPexp","OPopen",			\
		  "OPclose"						\
                }

#define OPprecidence {  0,  0,  0,  0,    \
                        0,  0,  0,  0,    \
                        0,  0,  1,  1,    \
                        2,  2,  3,  4,    \
                        4                 \
                     }


#define OPtype       {  0,  1,  1,   0,		\
                        1, -1,  1,  -1,   	\
                        1, -1,  2,   2,		\
			2,  2,  2,   4,		\
		        4 			\
		     }

#define OP_ISEV(a)   (((a)>OPeolist) && ((a) < OPeol_qty) )
#define OP_ISV(a)    (((a)>=OPlit) && ((a) <= OPvalm))
#define OP_ISEND(a)   ( ((a)<=OPeol_qty) ) 
#define OP_ISANYV(a)  (OP_ISV(a) || OP_ISEV(a))


typedef int eqn_litref_t;
#define eqn_litref_INIT -1

typedef struct eqntokenlit_st
{
  char *lit;
  eqn_litref_t ref;
  float cache;
}eqntokenlit_t;


typedef struct eqntokenval_st
{
  float x;
}eqntokenval_t;

typedef struct eqntokenpval_st
{
  float *xp;
} eqntokenpval_t;

typedef struct eqntoken_st
{
  itemop_t op;
  union lit_val_un
  {
    eqntokenlit_t lit;
    eqntokenval_t val;
    eqntokenpval_t pval;
  }z;
}eqntoken_t;


#define EQN_DELIM '\''

#define MAXEQNSTACK 512
typedef struct eqntop_st
{
  char *str;
  char last;

  eqntoken_t nodes[MAXEQNSTACK];
  int nodep;

  int litc;
  int opc;
}eqntop_t;



typedef struct plookup_st
{
  eqn_litref_t (*lookup)(struct plookup_st *user, char *str);
  void *user;
}plookup_t;

#define MAXSTACK 512
typedef struct eqneval_st
{
  float stack[MAXSTACK];
  int stackp;
  eqntoken_t *list;

}eqneval_t;


typedef struct eqn_st
{
#ifdef NO_EQUATIONS
  float val;
#else
  eqntoken_t *eqn;  
#endif
}eqn_t;



eqn_t eqn_const(float val);
void debug_eqn(void *dbg_fp, char *str, eqn_t *eqn);
eqn_t eqn_undefined(void);
eqn_t eqn_valp(float *valp);
float eqn_setvalp(eqn_t *eqn, float *valp);
int eqn_is_undefined(eqn_t e);

eqntoken_t *eqntok_parse(char *str);

eqntoken_t *eqntoken_next(eqntoken_t *p);
int eqntok_depend(eqntoken_t *list, plookup_t *lookup);
int eqntok_eval(float *res, eqntoken_t *list);
float parse_float(unsigned char *val);
eqntoken_t *eqntok_copy(eqntoken_t *p);
float eqn_setval(eqn_t *eqn, float val);
float eqn_getval_(eqn_t *eqn); 
#define eqn_getval(a) ___FIXME___ 
eqn_t eqn_copy(eqn_t e);
eqn_t eqn_copy_m(eqn_t e, float m);
eqn_t eqn_empty(void);
eqn_t eqn_parse(unsigned char *val);

void *eqn_mem_new(void);
void eqn_mem_push(void *p);
void *eqn_mem_pop(void);
void eqn_mem_free(void *p);


/**  dependency lists  ***/


typedef struct eqnlist_st
{
  names_t *params;
  list_t  eqnlist;
  list_t  cache;
}eqnlist_t;

void   eqnl_depend(eqnlist_t *nl, eqn_t e);
void   eqnl_free(eqnlist_t *nl);
void   eqnl_init(eqnlist_t *nl);
void   eqnl_evaldep(eqnlist_t *nl);
float  eqnl_eval(eqnlist_t *nl, eqn_t e);
int    eqnl_add(eqnlist_t *nl, eqn_t e, char *str);
eqn_t  eqnl_eqn(eqnlist_t *nl, int index);
int    eqnl_find(eqnlist_t *nl, char *str);
int    eqnl_define(eqnlist_t *nl, int index, float *valp);
int    eqnl_is_undefined(eqnlist_t *nl, int index);
int    eqnl_qty(eqnlist_t *nl);
char * eqnl_lookup(eqnlist_t *nl, int index);
void eqnl_autodepend(eqnlist_t *nl);
