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
/* eqnlist.c, routines for equation dependency lists, part of package eqn
   Conrad Ziesler
*/

#include <stdio.h>
#include "debug.h"

#include "names.h"
#include "list.h"
#include "eqn.h"
#include "bitlist.h"



int __debug_eqnlist__=1;

#ifndef STRIP_DEBUGGING
#define D(level,a) do { if(__debug_eqnlist__>(level)) a; } while(0)
#else
#define D(level,a) 
#endif

void eqnl_init(eqnlist_t *nl)
{
  nl->params=names_new();
  list_init(&(nl->eqnlist),sizeof(eqn_t),LIST_DEFMODE);
  list_init(&(nl->cache),sizeof(float),LIST_DEFMODE);
}
void eqnl_free(eqnlist_t *nl)
{
  list_empty(&(nl->eqnlist));
  list_empty(&(nl->cache));
  names_free(nl->params);
}

int eqnl_find(eqnlist_t *nl, char *str)
{
  int ind;
  ind=names_check(nl->params,str);
  return ind;
}

eqn_t eqnl_eqn(eqnlist_t *nl, int index)
{
  eqn_t *ep;
  ep=list_data(&(nl->eqnlist),index);
  if(ep!=NULL)
    return *ep;
  return eqn_empty();
}


int eqnl_add(eqnlist_t *nl, eqn_t e, char *str)
{
#ifdef NO_EQUATIONS
  return -1;
#else
  int i;
  eqn_t ed;
  float fv=0.0;

  ed=eqn_copy(e);
  i=list_add(&(nl->eqnlist),&ed);
  if(names_check(nl->params,str)>=0)
    { fprintf(stderr,"eqnlist equations: you are doing something wierd w/ hier. (%s) \n",str);   assert(0); }  
  names_add(nl->params,i,str);
  list_add(&(nl->cache),&fv);
  D(10,fprintf(stderr,"eqnl_add: adding %s as index %i\n",str,i));
  return i; 
#endif
}

/* here we evaluate terminals for the user, that is equations 
   for which no other equation depends,
   
   we first lookup in the parameter cache any dependencies
   and then call eqntok_eval to do the dirty work 
*/
   
float eqnl_eval(eqnlist_t *nl, eqn_t e)
{
#ifdef NO_EQUATIONS
  return e.val;
#else
  float *fp;
  float res;
  int r;
  eqntoken_t *et;
  int index;

  if(e.eqn==NULL)return 0.0;
  for(et=e.eqn;!OP_ISEND(et->op);et=eqntoken_next(et))
    {
      if((et->op==OPlit) || (et->op==OPlitm))
	assert(0);
      
      if((et->op==OPlitv) || (et->op==OPlitvm))
	{
	  if((index=et->z.lit.ref)==eqn_litref_INIT)
	    assert(0);
	  
	  fp=list_data(&(nl->cache),index);
	  if(fp!=NULL)
	    et->z.lit.cache=*fp;
	  else assert(0);
	}
    }
  res=0.0;
  r=eqntok_eval(&res,e.eqn);
  assert(r==0);
  return res;
#endif
}


/*
  any parameter is stored in our cache.
  here we resolve parameter dependecies 
  and evaluate all parameters.
  
*/

void eqnl_evaldep(eqnlist_t *nl)
{
#ifdef NO_EQUATIONS
  return;
#else
  eqntoken_t *et;
  int i,sweep,qleft=0,qdep,index;
  eqn_t *e;
  bitlist_t *bl;
  
  qleft=list_qty(&(nl->eqnlist));
  bl=bitlist_new(qleft);
  
  for(sweep=0;sweep<32;sweep++) /* limit heirarchy depth to stop cycles */
    {
      /* now check dependencies of all equations until done */
      list_iterate(&(nl->eqnlist),i,e)
	{
	  if(!bitlist_test(bl,i)) /* quick check for done */
	    {
	      qdep=0; 
	      /* go through eqntokens and find dependencies */
	      for(et=e->eqn;!OP_ISEND(et->op);et=eqntoken_next(et))
		{
		  if((et->op==OPlit) || (et->op==OPlitm))
		    assert(0);
		  
		  if((et->op==OPlitv) || (et->op==OPlitvm))
		    {
		      if((index=et->z.lit.ref)==eqn_litref_INIT)
			assert(0);
		      
		      if(bitlist_test(bl,index))
			{ /* dependency is already computed, get its value */
			  float *fp;
			  
			  fp=list_data(&(nl->cache),index);
			  if(fp!=NULL)
			    et->z.lit.cache=*fp;
			  else assert(0);
			}
		      else qdep++; /* indicate dependency not done */
		    }
		}

	      if(qdep==0) /* all dependencies resolved, mark as done */
		{
		  int r=100;
		  float *fp;

		  fp=list_data(&(nl->cache),i);

		  if(fp!=NULL) /* compute new value */
		    {
		      r=eqntok_eval(fp,e->eqn);
		      D(5,fprintf(stderr,"Evaldep: recomputed equation %i -> %s == %g\n",i,eqnl_lookup(nl,i),*fp));
		    }
		  assert(r==0);
		  bitlist_set(bl,i);
		  qleft--;
		}
	      
	    }
	}
      if(qleft==0)break;
    }
  if(sweep==32)
    {
      assert(0);
    }

  bitlist_free(bl);
#endif
}

/* do eqntoken_depend looking up unresolved symbols in eqnlist 
   that is, a single predefined list of parameters 
 */

static eqn_litref_t eqnl_depend_lookup(struct plookup_st *user, char *str)
{
  eqn_litref_t r;
  r=eqnl_find(user->user, str);
  if(r<0)
    {
      fprintf(stderr,"eqnl_depend: unresolved symbol %s\n",str);
      assert(0);
    }
  return r;
}


void eqnl_depend(eqnlist_t *nl, eqn_t e)
{
  plookup_t plu;  
  if(e.eqn==NULL)return;
  plu.lookup=eqnl_depend_lookup;
  plu.user=nl;
  eqntok_depend(e.eqn,&plu);
}


/* do eqnl_depend on all predefined parameters in eqnlist */
void eqnl_autodepend(eqnlist_t *nl)
{
  int i;
  eqn_t *e;

  list_iterate(&(nl->eqnlist),i,e)
    {
      eqnl_depend(nl,*e);
    }
}

/* check if an entry in our eqn table is undefined,
   that is it should be defined by the client 
   before it tries to call evaleqns 
*/
int eqnl_is_undefined(eqnlist_t *nl, int index)
{
  eqn_t *ep;
  ep= list_data(&(nl->eqnlist),index);
  if(ep==NULL)return -1;
  return eqn_is_undefined(*ep);
}


int eqnl_define(eqnlist_t *nl, int index, float *valp)
{
  eqn_t *ep;
  ep= list_data(&(nl->eqnlist),index);
  if(ep==NULL)return -1;
  eqn_setvalp(ep,valp);
  return 0;
}

int eqnl_qty(eqnlist_t *nl)
{
  int a,b;
  a=list_qty(&(nl->eqnlist));
  b=list_qty(&(nl->cache));
  assert(a==b);
  return a;
}

char *eqnl_lookup(eqnlist_t *nl, int index)
{
  return names_lookup(nl->params,index);
}
