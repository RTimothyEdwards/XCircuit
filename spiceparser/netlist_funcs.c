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
/* netlist_funcs.c:  table of usefull netlistfunc_t's for standard devtypes 
   Conrad Ziesler
*/

#include <stdio.h>
#include "debug.h"
#include "netlist_dev.h"

#define EVALfull(n,a)  eqnl_eval(&((n)->nl->eqnl),(a)) 
#define EVALquick(n,a) eqn_getval_(&(a))
#define EVALnm(a)  EVALquick(nm,a)

/* accumulate fet properties (areas, perimeters add per fraction of total width) */

static int nlfd_props[]={ DEVFET_as, DEVFET_ad, DEVFET_ps, DEVFET_pd };
static int nlfd_merge_same[]= { DEVFET_w, DEVFET_as, DEVFET_ad, DEVFET_ps, DEVFET_pd};
static int nlfd_merge_flip[]= { DEVFET_w, DEVFET_ad, DEVFET_as, DEVFET_pd, DEVFET_ps};

static void nlfd_fet_acc(netlistfunc_t *nm, int devi, int termi, float *nodeprops)
{
  devfet_t *f;
  int w,i;
  
  f=list_data(&(nm->entity->l),devi);
  assert(f!=NULL);
  w= (f->type==DEVFET_nmos)?0:1;
  
  if((termi==DEVFET_S)||(termi==DEVFET_D))
    {
      nodeprops[w]+=EVALnm(f->v[DEVFET_w]);
      for(i=0;i<4;i++)
	nodeprops[2+i]+=EVALnm(f->v[nlfd_props[i]]);
    }
}

static void nlfd_fet_dist(netlistfunc_t *nm, int devi, int termi, float *nodeprops)
{
  devfet_t *f;
  int w,i;
  float fraction;

  f=list_data(&(nm->entity->l),devi);
  assert(f!=NULL);
  w= (f->type==DEVFET_nmos)?0:1;
  
  if((termi==DEVFET_S)||(termi==DEVFET_D))
    {
      if(nodeprops[w]!=0)
	fraction=EVALnm(f->v[DEVFET_w])/nodeprops[w];
      else fraction=0;
      
      for(i=0;i<4;i++)
	eqn_setval(&(f->v[nlfd_props[i]]),nodeprops[i+2]*fraction);
    }
}

/* source and drain we do separately by in nlfd_fet_sum, since they are symmetrical */
typedef struct nlfd_cmp_st
{
  termptr_t g,b; 
  float l;
  char type;
}nlfd_cmp_t;


static void nlfd_fet_cmp(netlistfunc_t *nm, int index, void *data)
{
  devfet_t *f;
  nlfd_cmp_t *cmp=data;
  
  f=list_data(&(nm->entity->l),index);
  assert(f!=NULL);
  cmp->type=f->type;
  cmp->l=EVALnm(f->v[DEVFET_l]);
  cmp->g=f->n[DEVFET_G];
  cmp->b=f->n[DEVFET_B];
}

static int nlfd_fet_sum(struct netlistfunc_st *nm,int ia, int ib)
{
  devfet_t *fa,*fb;
  int i;
  fa=list_data(&(nm->entity->l),ia);
  fb=list_data(&(nm->entity->l),ib);
  

  assert(fa!=NULL);
  assert(fb!=NULL);


  if(    /* these are checked in mergedup, and should be ok */
     (!memcmp(&(fa->n[DEVFET_G]),&(fb->n[DEVFET_G]),sizeof(termptr_t))) && 
     (!memcmp(&(fa->n[DEVFET_B]),&(fb->n[DEVFET_B]),sizeof(termptr_t))) &&
     (EVALnm(fa->v[DEVFET_l]) == EVALnm(fb->v[DEVFET_l]))
     )	  
      {
	if((!memcmp(&(fa->n[DEVFET_S]),&(fb->n[DEVFET_S]),sizeof(termptr_t))) && 
	   (!memcmp(&(fa->n[DEVFET_D]),&(fb->n[DEVFET_D]),sizeof(termptr_t))) )
	  {
	    /* merge devices, same orientation */
	    for(i=0;i<(sizeof(nlfd_merge_same)/sizeof(int));i++)
	      eqn_setval(
			 &(fa->v[nlfd_merge_same[i]]),
			 EVALnm(fa->v[nlfd_merge_same[i]])+
			 EVALnm(fb->v[nlfd_merge_same[i]])
			 );
	    return 1;
	  }
	if((!memcmp(&(fa->n[DEVFET_S]),&(fb->n[DEVFET_D]),sizeof(termptr_t))) && 
	   (!memcmp(&(fa->n[DEVFET_D]),&(fb->n[DEVFET_S]),sizeof(termptr_t))) )
	  {
	    /* merge devices, opposite orientation */
	    for(i=0;i<(sizeof(nlfd_merge_same)/sizeof(int));i++)
	      eqn_setval(
			 &(fa->v[nlfd_merge_same[i]]),
			 EVALnm(fa->v[nlfd_merge_same[i]])+
			 EVALnm(fb->v[nlfd_merge_flip[i]])
			 );
	    return 1;
	  }
      }
  else
    {
      fprintf(stderr,"mergedup compare failure\n"); 
    } 
  
  return 0; /* fail request */
}

netlistfunc_t * netlist_devfet_funcs(netlist_t *nl, netlistfunc_t *fp)
{
  netlistfunc_t f;
  memset(&f,0,sizeof(f));

  f.nl=nl;
  f.entity=nl->e+DEVT_FET;
  f.qproperties=6; /* n/p width + as/ad/ps/pd */
  f.accumulate=nlfd_fet_acc;
  f.distribute=nlfd_fet_dist;
  f.qcmp=sizeof(nlfd_cmp_t);
  f.sum=nlfd_fet_sum;
  f.tocompare=nlfd_fet_cmp;
  f.fixup=NULL; /* this is not needed, only if user wants it */

  if(fp!=NULL)
    *fp=f;

  return fp;
}
