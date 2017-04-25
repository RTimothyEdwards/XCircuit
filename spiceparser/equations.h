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

#ifndef __EQUATIONS_H__

#define __EQUATIONS_H__




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
  OPlitv, /* valp */
  OPlitvm, /* -valp */

  OPval,
  OPvalm,
  OPadd,
  OPsub,

  OPmul,
  OPdiv,
  OPexp,
  OPnorm, /* a $ b == pow(pow(a,b),1/b) */
  OPopen,

  OPclose
}itemop_t;

#define OPsyms  { "OPeolist", "OPeol_const", "OPeol_valp", "OPeol_qty",	\
                  "OPlit", "OPlitm", "OPlitv", "OPlitvm",		\
                  "OPval", "OPvalm", "OPadd", "OPsub",			\
                  "OPmul", "OPdiv", "OPexp","OPnorm", "OPopen",		\
		  "OPclose"						\
                }

#define OPprecidence {  0,  0,  0,  0,    \
                        0,  0,  0,  0,    \
                        0,  0,  1,  1,    \
                        2,  2,  3,  3, 4,    \
                        4                 \
                     }


#define OPtype       {  0,  1,  1,   0,		\
                        1, -1,  1,  -1,   	\
                        1, -1,  2,   2,		\
			2,  2,  2,   2, 4,	\
		        4 			\
		     }

#define OP_ISEV(a)   (((a)>OPeolist) && ((a) < OPeol_qty) )
#define OP_ISV(a)    (((a)>=OPlit) && ((a) <= OPvalm))
#define OP_ISEND(a)   ( ((a)<=OPeol_qty) ) 
#define OP_ISANYV(a)  (OP_ISV(a) || OP_ISEV(a))



typedef struct eqntokenlit_st
{
  char *lit;
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
  char unget[4];
  int n_unget;
 
  int errors;
  eqntoken_t nodes[MAXEQNSTACK];
  int nodep;

  int litc;
  int opc;
}eqntop_t;


#define MAXSTACK 512
typedef struct eqneval_st
{
  float stack[MAXSTACK];
  int stackp;
  eqntoken_t *list;

}eqneval_t;


typedef struct eqn_st
{
  float val;
  eqntoken_t *eqn;  
}eqn_t;

/* for named equations */
typedef struct eqndef_st
{
  char *name;
  eqn_t eqn;
}eqndef_t;


/* public interface */
eqn_t equation_parse(uchar *str);
int equation_depend(eqn_t eqn, float *(lookup)(void *user, char *str), void *user);
int equation_eval(eqn_t *peqn);
eqn_t equation_empty(eqn_t eqn);
void equation_debug(eqn_t eqn, void *fp);
#endif
