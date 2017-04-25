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
/* netlist.h, headers for generic netlist wrappers
Conrad Ziesler
*/

/* even though the different file formats (spice, magic extract, verilog)
   have widely different requirements, the basic information is still 
   comparable, so we try to build a common interface to the subtype procedures
*/

#ifndef __NETLIST_H__


#ifndef __LIST_H__
#include "list.h"
#endif


#ifndef __NAMES_H__
#include "names.h"
#endif

#ifndef __EQN_H__
#include "eqn.h"
#endif

#ifndef __SCANNER_H__
#include "scanner.h"
#endif
 
#define __NETLIST_H__

struct extract_st;
struct spice_st;

#define EXTRACT_MAGIC 0x55315662
#define SPICE_MAGIC 0x435acde0


typedef struct geninput_st
{
  int magic;
  int other[4];
}geninput_t;


/* internal definition of netlist type */
typedef union netlist_input_un
{
  void *p;
  geninput_t *generic;
  struct spice_st *spice;
  struct extract_st *ext;
}netlist_input_t;

typedef union dev_input_un
{
  void *p;
  struct m_st *spice_fet;
  struct c_st *spice_cap;
  void *spice_res;
  void *extract_fet;
  void *extract_cap;
  void *extract_res;
}dev_input_t;






/* compact netlist:

 device is: object with k terminals (k small)
 netlist is connection of devices

 share common terminal pointer.

*/


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

#define TERMPTR_MAX_DEVI  ((1<<TERMPTR_BITS_DEVI)-1)
#define TERMPTR_MAX_TERMI ((1<<TERMPTR_BITS_TERMI)-1)
#define TERMPTR_MAX_DEVT  ((1<<TERMPTR_BITS_DEVT)-1)
#define netlist_termptr_isnull(t)  (t.nonnull==0)


/**** entities:  

      an entity_t describes and holds a list of devices 
      
      the flexible device structure is formatted as follows 
      {
      dev_input_t parent;
      termptr_t terms[e->qterms];
      eqn_t vals[e->qvals];
      rest....
      }

*****/

typedef struct entity_st
{
  list_t l;
  int qterms;   /* q terms of in generic structure */
  int qvals;    /* q values in generic structure   */
  int qcount;   /* counted number, low level       */
  int id;       /* uniq device id                  */
  int qother;   /* bytes of other info in dev structure */
  char sym[8];  /* symbolic name                   */
}entity_t;

#define ENTITY_INVALID { LIST_INITDATA, 0, 0, 0, -1,0, "INVALID" }

/* macros for accessing entities  */

#define NETLIST_E(nl,t,i)        ((void *)list_data(&((nl)->e[(t)].l),i))
#define NETLIST_E_DATA(nl,t,i)   ((char *)NETLIST_E(nl,t,i))
#define NETLIST_ED(e,i) ((char *)(list_data(&(e->l),i)))
#define NETLIST_OFFSET_PARENT(e)  (0)
#define NETLIST_PARENTS(nl,t,i)  ((dev_input_t*) (NETLIST_E_DATA(nl,t,i)+NETLIST_OFFSET_PARENT(nl->e[t])))
#define NETLIST_PARENT(nl,t,i)  (NETLIST_PARENTS((nl),(t),(i))[0])

#define NETLIST_OFFSET_TERMS(e)   (sizeof(dev_input_t) + NETLIST_OFFSET_PARENT(e) )
#define NETLIST_TERMS(nl,t,i)  ((termptr_t*)(NETLIST_E_DATA(nl,t,i)+NETLIST_OFFSET_TERMS(nl->e[t])))
#define NETLIST_TERM(nl,t,i,j)  (NETLIST_TERMS(nl,t,i)[(j)])
#define NETLIST_E_TERMS(e,i)    ((termptr_t*) (NETLIST_ED(e,i) + NETLIST_OFFSET_TERMS(*(e))))
#define NETLIST_E_TERM(e,i,j)   (NETLIST_E_TERMS(e,i)[(j)])

#define NETLIST_OFFSET_VALS(e)  (NETLIST_OFFSET_TERMS((e)) + ((e).qterms*sizeof(termptr_t)))
#define NETLIST_VALS(nl,t,i)   ((eqn_t *) (NETLIST_E_DATA(nl,t,i)+NETLIST_OFFSET_VALS(nl->e[t])))
#define NETLIST_VAL(nl,t,i,j)  (NETLIST_VALS(nl,t,(i))[(j)])


#define NETLIST_OFFSET_REST(e)     ( NETLIST_OFFSET_VALS(e) + (sizeof(eqn_t)*((e).qvals)))
#define NETLIST_REST(nl,t,i)   ((void *) (NETLIST_E_DATA(nl,t,i) + NETLIST_OFFSET_REST(nl->e[t]))

#define NETLIST_OFFSET_OTHER(e)    ( NETLIST_OFFSET_REST(e) + (e).qother )
#define NETLIST_OTHER(nl,t,i)      ((void *) (NETLIST_E_DATA(nl,t,i)+NETLIST_OFFSET_OTHER(nl->e[t])))

#define NETLIST_E_QTERMS(e)  ((e)->qterms)
#define NETLIST_QTERMS(nl,t)  ((nl)->e[t].qterms)
#define NETLIST_E_QVALS(e)   ((e)->qvals)
#define NETLIST_QVALS(nl,t)   ((nl)->e[t].qvals)

#define DEVT_NODE 0 /* all netlists have type 0 as a node */
#define DEVP(nl,t)       list_data(&((nl)->e[(t).devt].l),(t).devi)
#define DEVT_NODE_TERMS 2

typedef struct devnode_st
{
  dev_input_t parent;
  termptr_t n[DEVT_NODE_TERMS];
}devnode_t;


typedef struct netlist_st
{
  netlist_input_t input; /* where the netlist came from                   */
  void *eqn_mem; /* where we store our equations */
  int iscopy;   /* this is set if we shouldn't free/modify some structures */
  names_t *names; /* key off of node indices                              */
  eqnlist_t eqnl;
  int qe; /* valid entities */
  entity_t e[TERMPTR_MAX_DEVT];  
}netlist_t;





typedef struct netlistfunc_st
{
  netlist_t *nl;
  entity_t *entity;

  /* for merge */
  int qcmp;
  int (*sum)(struct netlistfunc_st *nm,int ia, int ib);
  void (*tocompare)(struct netlistfunc_st *nm, int index, void *data);

  /* for distribute */
  int qproperties;
  void (*accumulate)(struct netlistfunc_st *nm, int devi, int termi, float *nodeprops);
  void (*distribute)(struct netlistfunc_st *nm, int devi, int termi, float *nodeprops);
  
  /* for fixup */
  void (*fixup)(struct netlistfunc_st *nm, termptr_t t);

}netlistfunc_t;

typedef struct netlist_param_st
{
  struct netlist_param_st *next;
  char str[4];
}netlist_param_t;



typedef enum netprint_en
{
  netprint_none,
  netprint_name,
  netprint_index,
  netprint_ptr,
  netprint_nname,
  netprint_debug
}netprint_t;




/******* netlist.c ********/
void netlist_init(netlist_t *nl);
int netlist_register_entity (netlist_t *nl, int id, int qterms, int qvals, int qrest, char *sym, int othersize);
int netlist_print_nodep(netlist_t *n, netprint_t grp, void *filefp, void *gn, char *str);
netlist_input_t netlist_parse_input(scanner_t *scan, char *type, netlist_param_t *params);
void netlist_release(netlist_t *nl);
void netlist_free(netlist_t *nl);
int netlist_newdev_fromnode(netlist_t *nl, int devt, dev_input_t parent , termptr_t n[]);
void netlist_debug_input(void *dbg_fp, netlist_input_t p);
void netlist_debug(void *dbg_fp, netlist_t *nl);
void netlist_debug_(void *dbg_fp, netlist_t *nl, int eval);
termptr_t netlist_node(netlist_t *nl, char *str, void *parent);
termptr_t netlist_termptr(int dev, int term, int type);
int netlist_node_termptr(netlist_t *nl, termptr_t t);
int netlist_findnode(netlist_t *nl, char *str);
void netlist_merge(netlistfunc_t *nm);
void netlist_distribute(netlistfunc_t *nm);
void netlist_fixup(netlistfunc_t *nm);
void netlist_eval(netlist_t *n);
void netlist_eqn_begin(netlist_t *nl);
void netlist_eqn_end(netlist_t *nl);

netlist_t *netlist_copyisher(netlist_t *in, int extrasize, int extrasizes[]);
void netlist_copyfree(netlist_t *nl);


/********* netlist_spice.c *********/

struct spice_st *spice_new(scanner_t *scan);
void     spice_release(struct spice_st *sp);

void     spice_build(netlist_t *nl);
void     spice_count(netlist_t *nl);
void     spice_debug(void *dbg_fp, struct spice_st *sp);



#endif

