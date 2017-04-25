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
/* netlist_lib.h, definitions for netlist library routines
   Conrad Ziesler
*/

#ifndef __EQN_H__
#include "eqn.h"
#endif

#ifndef __NETLIST_H__
#include "netlist.h"
#endif

#ifndef __BITLIST_H__
#include "bitlist.h"
#endif

#define NLIBIO_MAXNODES 4

/* convenience structures */
typedef struct nlibiodef_st
{
  char *encoding;
  int wires;
  int tokens;
  int digitize;
}nlibiodef_t;


/* input definition */
typedef struct nlibio_st
{
  int   tokens;
  int   qnodes;
  int   digitize;
  int   nodes[NLIBIO_MAXNODES];
}nlibio_t;

typedef struct nlibiol_st
{
  list_t iol;
  int qtokens; /* product (iol[k].tokens) */
}nlibiol_t;

/* parameter range */
typedef struct nlibpm_st 
{
  int index; /* index into netlist parameter list */
  float min, max;
  int qty;
}nlibpm_t;

typedef struct nlibfunc_st
{
  int p_delay;    /* phases of delay before we begin computations     */
  int p_cycle;    /* phases per cycle (compute each n phases)         */
  int p_offset;   /*  phases between inputs sampled and outputs valid */
  int qin;
  int qout;
  int map[2]; /* alloc in place, size = qin*sizeof(int) */  
}nlibfunc_t;

/* function reference */
typedef struct nlibfref_st
{
  int index;
}nlibfref_t;



/* reference nodes and associated skew parameters */
typedef enum nlibreftype_en
{
  RTrseries=0,
  RTcshunt=1,
  RTdelay=2,
  RTqty
}nlibreftype_t;

#define MAXSKEW_PARAMS 3
typedef struct nlibrefnode_st
{
  eqn_t skew[MAXSKEW_PARAMS]; /* RTxxx indices */
  int vrefi;  /* index into vref structure */
  int devnodei; /* node index in netlist */
}nlibrefnode_t;

/* database object */
typedef struct nlibdb_st
{
  netlist_t *nl;
  nlibiol_t in,out;
  nlibfref_t map; /* mapping between input and output tokens */
  list_t params;
  list_t refnodes; 
  float *pvals;
}nlibdb_t;


typedef struct nlibflatdb_st
{
  int lib_index;  /* index from process */
  int db_index;  /* index into db list */
  int qvals; /* cache db->params.q */
  float *pvals; /* copy db pvals to here */
  list_t uservals;  /* array of user gate physical parameters */
}nlibflatdb_t;

/* container of database objects */
typedef struct nlib_st
{
  list_t iodefs;
  list_t db;      /* our database of nlibdb objects */
  names_t *dbref; /* database gate reference names */
  names_t *vref;  /* voltage reference node names */
  int qvref;
  list_t funcs; /* ptrs */
  names_t *funcnames;
  eqnlist_t eqnl; /* top level parameters we can use anywhere in eqns */
  bitlist_t *flags;  /* user can select set of elements to use */
  list_t flatdb; /* database of flat info */
  void *eqn_mem;
}nlib_t;

#define NLIB_MAXPHASES 4

typedef struct nlib_icall_st
{
  int dbi;
  int flatdbi;
  float *in[NLIBIO_MAXNODES];
  float *out[NLIBIO_MAXNODES];
  int itokens[NLIB_MAXPHASES], otokens[NLIB_MAXPHASES], ctoken;
  int ver;
  int err;
  int idigitize, odigitize;
  int inode,onode;
}nlib_icall_t;

#define NLIB_MAXFREF 64
typedef struct nlib_ifref_st
{
  float *val;
  float vals[NLIB_MAXPHASES];
  char name[NLIB_MAXFREF];
}nlib_ifref_t;

typedef enum nlib_refid_en
{
  NLIBref_vss, /* the vdd/vss values are used by cmos digitalize */
  NLIBref_vdd, 
  NLIBref_clk, /* clk is generic refering to some clock signal */
  NLIBref_firstunused, /* user can allocate any ref ids after this point */
  NLIBref_max=512
}nlib_refid_e;

typedef struct nlib_iface_st
{
  nlib_t *nlib;
  list_t icalls;
  list_t frefs;
  int ref_map[NLIBref_max];
  int phase;
  float * (*lookup_node_f)(void *user, char *name);
  void *user;
}nlib_iface_t;

typedef enum nlib_tokens_en
{
  Knlib_start=1024,  Knlib_nlib,Knlib_input,  Knlib_output,
  Knlib_param,  Knlib_netlist,  Knlib_spice,
  Knlib_encoding,    Knlib_wires,
  Knlib_tokens,  Knlib_digitize,  Knlib_digital, Knlib_qtyin, Knlib_qtyout,
  Knlib_end_digital, Knlib_min, Knlib_max, Knlib_qty,
  Knlib_function, Knlib_end_netlist, Knlib_define,
  Knlib_reference, Knlib_name, Knlib_rseries, Knlib_cshunt, Knlib_delay,
  Knlib_p_delay, Knlib_p_offset, Knlib_p_cycle,
  Knlib_end
}nlib_tokens_t;

#define NLIB_TOKENS  { 				\
  { Knlib_nlib, ".nlib" },			\
  { Knlib_input, ".input" },			\
  { Knlib_output, ".output" },			\
  { Knlib_define, ".define" },        \
  { Knlib_param, ".param" },			\
  { Knlib_netlist, ".netlist" },		\
  { Knlib_end_netlist, ".end_netlist" },        	\
  { Knlib_spice, ".spice" },		        	\
  { Knlib_encoding, ".encoding" },      		\
  { Knlib_digital, ".digital" },		        \
  { Knlib_end_digital, ".end_digital" },		\
  { Knlib_qtyout, ".qtyout" },		        \
  { Knlib_qtyin, ".qtyin" },		        \
  { Knlib_digitize, ".digitize" },      		\
  { Knlib_min, ".min" },		                \
  { Knlib_max, ".max" },		                \
  { Knlib_qty, ".qtysteps" },		                \
  { Knlib_function, ".function" },                      \
  { Knlib_tokens, ".tokens" }, \
  { Knlib_wires, ".wires" }, \
  { Knlib_name, ".name" }, \
  { Knlib_rseries, ".rseries" }, \
  { Knlib_cshunt, ".cshunt" }, \
  { Knlib_delay, ".delay" }, \
  { Knlib_reference, ".reference" }, \
  { Knlib_p_delay,   ".p_delay" },  \
  { Knlib_p_offset,  ".p_offset" },  \
  { Knlib_p_cycle,   ".p_cycle"  },  \
  { Knlib_end, NULL }                            }


typedef struct nlib_process_st
{
  /* filled in by user */
  void (*process)(struct nlib_process_st *proc);
  void *user;
  nlib_t *nlib;

  /* filled in by nlib_process before user->process() is called */
  int lib_index;
  int db_index;
  netlist_t *nl;
}nlib_process_t; 
 

int nlib_parse(nlib_t *nlib, scanner_t *scan);
int nlib_add_io(nlib_t *nlib, int dbindex, nlibiol_t *io, int tokens, int nodes, int dig, char **names);
int nlib_init(nlib_t *nlib);
int nlib_add_pm(nlib_t *nlib, int dbi, char *str, float min, float max, int qty);
int nlib_new_db(nlib_t *nlib, char *str);
int nlib_db_netlist(nlib_t *nlib, int index, netlist_t *nl);
int nlib_db_func(nlib_t *nlib, int index, char *str);
void nlib_process(nlib_process_t *user, int screened);
void nlib_fixup(nlib_t *nlib);
void nlib_disable(nlib_t *nlib, int index);
int nlib_isdisabled(nlib_t *nlib, int index);

void nlib_release(nlib_t *nlib);

void *nlib_flat(nlib_process_t *user, int s_uservals, int q_uservals);
void nlib_o_spice_subckts(nlib_t *nlib, void * of_fp);
void nlib_o_spice_call(nlib_t *nlib, int index, int i_node, int o_node, int *skt, void * of);
void nlib_o_spice_refs(nlib_t *nlib, void * of_fp);
int nlib_flatdb_write(nlib_t *nlib, void * file_fp);
int nlib_flatdb_read(nlib_t *nlib, void * file_fp);

/* interface routines */

nlib_ifref_t *nlib_iface_getref(nlib_iface_t *iface, int id);
void nlib_iface_phase(nlib_iface_t *iface);
int nlib_i_spice_calls(nlib_iface_t *iface, void * cktf_fp);
void nlib_iface_init(nlib_iface_t *iface, nlib_t *nlib);
void nlib_iface_release(nlib_iface_t *iface);


