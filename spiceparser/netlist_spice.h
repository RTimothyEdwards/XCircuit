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
/* spice.h, header for spice parsing routines
   Conrad Ziesler
*/

/* note: alot of this is imported from legacy/adb 
         so it has some historic baggage still
*/

#ifndef __NETLIST_SPICE_H__
#define __NETLIST_SPICE_H__

#ifndef __EQN_H__
#include "eqn.h"
#endif

#ifndef __HASH_H__
#include "hash.h"
#endif

#ifndef __SCANNER_H__
#include "scanner.h"
#endif


/*******************************************************/
/********* SPICE HASH TABLE CLIENTS ********************/

#define PAYLOAD_null     0  
#define PAYLOAD_card_val 1
#define PAYLOAD_parent   2
#define PAYLOAD_gnptr    3
#define PAYLOAD_subckt   4
#define PAYLOAD_node     5


/* HACK **/
#ifndef __NETLIST_H__
#define TERMPTR_BITS_DEVI 24
#define TERMPTR_BITS_TERMI 3
#define TERMPTR_BITS_DEVT  4

typedef struct termptr_st
{
  unsigned devi:  TERMPTR_BITS_DEVI;
  unsigned nonnull: 1;
  unsigned termi: TERMPTR_BITS_TERMI;
  unsigned devt:  TERMPTR_BITS_DEVT;
}termptr_t;
#endif

union paydata_u
{
  void *p;
  char *s;
  struct subckt_st *subckt;
  termptr_t t;
};

typedef struct hashload_st
{
  int flag;
  union paydata_u data;
}hashload_t;

typedef struct nodeclient_st
{
  hashload_t payload;
  unsigned char str[4]; /* alloc in place */
}nodeclient_t;

typedef struct nodeclient_st *node_t;

#define FORALL_HASH(h,i,p)  hash_forall(h,i,p) 


typedef struct paramload_st
{
  eqn_t eqn;
  struct callparam_st *patch_call;
  int global_i;
}paramload_t;


typedef struct paramclient_st
{
  paramload_t payload;
  char str[4];
}paramclient_t;

typedef struct paramlookup_st
{
  struct stack_st *sp;
  struct netlist_st *nl;
}paramlookup_t;


typedef struct callparam_st
{
  eqn_t eqn;
  char str[4];
}callparam_t;


/*****************************************/
/****** SPICE NETLIST READING    *******/

/***** scanner stuff *****/



typedef enum keyword_et
{
  Kother,  Kinclude,  Ksubckt,  Kends,  Kend,  Kendm,  Kmodel,  Kparam,  Kglobal,  Kmult,  Kscale,  Knull
}keyword_t;

#define SPICE_TOKENS { 				\
  { Kinclude, ".include" }, 			\
  { Ksubckt,  ".subckt"  },			\
  { Kends,    ".ends"    },			\
  { Kendm,    ".endm"    },			\
  { Kmodel,   ".model"   },			\
  { Kparam,   ".param"   },			\
  { Kglobal,  ".global"  },			\
  { Kmult,    ".mult"    },			\
  { Kscale,   ".scale"   },			\
  { Knull,    NULL       }			\
}

extern tokenmap_t spice_tokens[];


/****** pass 2 ************/

struct subckt_st; 

typedef struct x_st
{
  int nn;
  struct subckt_st *xp;
  node_t *nodes;
  callparam_t **locals;
  int nl;
  deck_t *deck;
  card_t *rest;
}x_t;

#define INDEX_GATE 1
#define INDEX_DRAIN 0
#define INDEX_SOURCE 2
#define INDEX_BULK 3 

typedef struct m_st
{
  node_t nodes[4];
  eqn_t l,w,as,ad,ps,pd;
  deck_t *deck;
  card_t *rest;
  uchar type;
}m_t;

typedef struct c_st
{
  node_t nodes[2];
  eqn_t c;
  deck_t *deck;
  card_t *rest;
}c_t;

typedef struct r_st
{
  node_t nodes[2];
  eqn_t r;
  deck_t *deck;
  card_t *rest;
}r_t;

typedef struct l_st
{
  node_t nodes[2];
  eqn_t l;
  deck_t *deck;
  card_t *rest;
}l_t;

typedef struct v_st
{
  node_t nodes[2];
  eqn_t v;
  deck_t *deck;
  card_t *rest;
}v_t;

typedef struct i_st
{
  node_t nodes[2];
  eqn_t i;
  deck_t *deck;
  card_t *rest;
}i_t;

#define SUBCKT_MAGIC 0x43236513

typedef struct subckt_st
{
  int subckt_magic;
  struct subckt_st *parent; /* who are we descended from */
  hash_t *nodes;      /* table of nodes */
  hash_t *params;     /* table of parameters  (those defined elsewhere) */
  hash_t *lparams;    /* table of local parameters (those on the .subckt line) */
  hash_t *cktdir;     /* table of locally defined subckts */ 
  hash_t *global;     /* table of locally defined globals */
  uchar *name;            /* our name */
  int ndefn;            
  int nm;
  int nc;
  int nr;
  int nl;
  int nx;
  int nv;
  int ni;
  node_t *defn;   /* ptrs into table of [nodes] for definitions */
  m_t *m;         /* array of mosfets */
  c_t *c;         /* array of capacitors */
  r_t *r;	  /* array of resistors */
  l_t *l;	  /* array of inductors */
  x_t *x;         /* array of subcircuit calls */
  v_t *v;	  /* array of voltage sources */
  i_t *i;	  /* array of current sources */
  int flag;  
  float scale,mult;
}subckt_t;


typedef struct stack_st
{
  int level;
  struct stack_st *parent; 
  subckt_t *ckt;
  x_t *call;
  struct stack_st *top;
  struct netlist_st *nl;
  scanner_t *scan;
}stack_t;

typedef struct spice_st
{
  int spice_magic;
  scanner_t *scan;
  deck_t *deck;
  subckt_t *ckt;
  void *eqn_mem;
}spice_t;



#define minimum(a,b)  (((a)<(b))?(a):(b))


subckt_t *spice_find_subckt(subckt_t *parent, char *name);
spice_t *spice_new(scanner_t *scan);
void spice_release(spice_t *sp);
void spice_debug(void * dbg_fp, spice_t *sp);
list_t spice_list_subckt(subckt_t *parent);




#endif /* __NETLIST_SPICE_H__*/
