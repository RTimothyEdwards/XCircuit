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
/* netlist_dev.h, definitions for a netlist of circuital devices 
   Conrad Ziesler
*/
#ifndef __NETLIST_DEV_H__
#define __NETLIST_DEV_H__
#ifndef __NETLIST_H__
#include "netlist.h"
#endif

#define DEVNODE_HEAD 0
#define DEVNODE_TAIL 1

#define DEVFET_S 0
#define DEVFET_D 1
#define DEVFET_G 2
#define DEVFET_B 3
#define DEVFET_w 0
#define DEVFET_l 1
#define DEVFET_as 2
#define DEVFET_ad 3
#define DEVFET_ps 4
#define DEVFET_pd 5
#define DEVFET_V  6

#define DEV2TERM_P 1
#define DEV2TERM_N 0

#define DEVCAP_c 0
#define DEVCAP_V 1

#define DEVRES_r 0
#define DEVRES_V 1

/* #define DEVT_NODE   0   defined in netlist.h */
#define DEVT_FET    1
#define DEVT_CAP    2
#define DEVT_RES    3
#define DEVT_IND    4
#define DEVT_DIODE  5
#define DEVT_VSRC   6
#define DEVT_ISRC   7
#define DEVT_BJT    8
#define DEVT_MAX    9
/*                   0  1  2  3  4  5  6  7  8  */
#define DEVT_TERMS { 2, 4, 2, 2, 2, 2, 2, 2, 4 }
#define DEVT_SYMS  { "NODE", "FET", "CAP", "RES", "IND", "DIODE", "VSRC", "ISRC", "BJT" }
#define DEVT_VALS  { 0, DEVFET_V, DEVCAP_V, DEVRES_V, 0 , 0, 0, 0 , 0 }
#define DEVT_OTHERS { 0, sizeof(unsigned), 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0 , 0 } 


#define DEVP_NODE(nl,t)  list_data(&((nl)->e[DEVT_NODE].l),(t).devi)
#define DEVP_NODEt(nl,t) ((devnode_t *)DEVP_NODE((nl),(t)))

#define DEVP_FET(nl,t)  list_data(&((nl)->e[DEVT_FET].l),(t).devi)
#define DEVP_FETt(nl,t) ((devfet_t *)DEVP_NODE((nl),(t)))

#define DEVP_CAP(nl,t)  list_data(&((nl)->e[DEVT_CAP].l),(t).devi)
#define DEVP_CAPt(nl,t) ((devcap_t *)DEVP_NODE((nl),(t)))

#define DEVGEN_V 8

typedef struct devgen_st
{
  dev_input_t parent;
  termptr_t n[TERMPTR_MAX_TERMI];
  eqn_t v[DEVGEN_V];
}devgen_t;

#define DEVFET_nmos 1
#define DEVFET_pmos 2

typedef struct devfet_st
{
  dev_input_t parent;
  termptr_t n[4];
  eqn_t v[DEVFET_V]; 
  unsigned type; /* n/p */
}devfet_t;


typedef struct devcap_st
{
  dev_input_t parent;
  termptr_t n[2];
  eqn_t v[1];
}devcap_t;


typedef struct devres_st
{
  dev_input_t parent;
  termptr_t n[2];
  eqn_t v[1];
}devres_t;


typedef struct devind_st
{
  dev_input_t parent;
  termptr_t n[2];
  eqn_t v[1];
}devind_t;


typedef struct devvsrc_st
{
  dev_input_t parent;
  termptr_t n[2];
  eqn_t v[1];
}devvsrc_t;


typedef struct devisrc_st
{
  dev_input_t parent;
  termptr_t n[2];
  eqn_t v[1];
}devisrc_t;


typedef struct devdiode_st
{
  dev_input_t parent;
  termptr_t n[2];
  eqn_t v[4];
}devdiode_t;


typedef struct devbjt_st
{
  dev_input_t parent;
  termptr_t n[4];
  eqn_t v[4];
}devbjt_t;

typedef struct dev2term_st
{
  dev_input_t parent;
  termptr_t n[2];
  eqn_t v[1];
}dev2term_t;

typedef struct dev4term_st
{
  dev_input_t parent;
  termptr_t n[4];
  eqn_t v[1];
}dev4term_t;

typedef union devall_un
{
  void *p;
  devgen_t *genp;
  devnode_t *nodep;
  devfet_t *fetp;
  devcap_t *capp;
  devind_t *indp;
  devbjt_t *bjtp;
  devvsrc_t *vsrcp;
  devisrc_t *isrcp;
}devall_p;


#define NETLIST_DEFSIZES \
  {  \
     sizeof(devnode_t) , sizeof(devfet_t),  sizeof(devcap_t), sizeof(devres_t),  \
     sizeof(devind_t),  sizeof(devdiode_t), sizeof(devvsrc_t),    \
     sizeof(devisrc_t) , sizeof(devbjt_t)   \
  }




netlist_t *netlist_copyish(netlist_t *in, int size, int sizes[]);
void netlist_o_spice(void *of_fp, netlist_t *nl, int n_start);
netlist_t *netlist_devnew_inplace(netlist_input_t ni, int extrasize, int extrasizes[DEVT_MAX]);
netlist_t *netlist_devnew(netlist_input_t ni);
/********* netlist_funcs.c *********/
netlistfunc_t * netlist_devfet_funcs(netlist_t *nl, netlistfunc_t *fp);

#endif

