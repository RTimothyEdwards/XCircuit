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
/* netlist.c, generalized netlist functions 
Conrad Ziesler
*/
#include <stdio.h>
/* note: 

   we try to store a netlist in a form which is as compact as possible, while being extensible
   by clients, in addition to being general enough to handle all possible netlist input formats

   the primary memory saving device is the termptr_t bitfield, where we make a complex
   pointer which represents the type of node it points to.

   also we statically allocate memory for the devices using list_t blocks.
   and allocate extra space per device as defined by the client structure
   
*/

#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

#define __NETLIST_PRIVATE__
#include "netlist.h"
#include "mergedup.h"


/* call these at entry and exit of all netlist functions that potentially modify equations */
void netlist_eqn_begin(netlist_t *nl)
{
  assert(nl->eqn_mem!=NULL);
  eqn_mem_push(nl->eqn_mem); 
}

void netlist_eqn_end(netlist_t *nl)
{
  if(eqn_mem_pop()!=nl->eqn_mem)
    assert(0);
}


/* some assumptions we make about bitfield encodings */
int netlist_machine(void)
{
 assert(sizeof(termptr_t)==sizeof(void*));
 assert(sizeof(unsigned int) == sizeof(void*));
 assert(sizeof(unsigned int) == 4 );
 return 0;
}

termptr_t netlist_termptr_null_t(termptr_t from)
{
  from.nonnull=0;
  return from;
}

termptr_t netlist_termptr_null(void)
{
  termptr_t v;
  v.nonnull=0;
  v.termi=0;
  v.devi=0;
  v.devt=0;
  return v;
}

termptr_t netlist_termptr_nonnull(termptr_t from)
{
  from.nonnull=1;
  return from;
}


termptr_t netlist_termptr(int dev, int term, int type)
{
  termptr_t t;
  t.termi=term;
  t.devi=dev;
  t.devt=type;
  t.nonnull=1;
  return t;
}

int netlist_termptr_devi(termptr_t t)
{ return t.devi; }

int netlist_termptr_termi(termptr_t t)
{ return t.termi; }

int netlist_termptr_devt(termptr_t t)
{ return t.devt; }

termptr_t netlist_termptr_deref(netlist_t *nl, termptr_t t)
{
  assert(t.devt>=0);
  assert(t.devt<nl->qe);
  assert(t.termi>=0);
  assert(t.termi<nl->e[t.devt].qterms);
  assert(t.nonnull==1);
  return NETLIST_TERM(nl,t.devt,t.devi,t.termi);
}

int netlist_node_termptr(netlist_t *nl, termptr_t t)
{
  while(t.devt!=DEVT_NODE)
    t=netlist_termptr_deref(nl,t);
  return t.devi;
}


int netlist_add_dev_(netlist_t *nl, int devt, void *p)
{
  int i;
  assert(devt>=0);
  assert(devt<nl->qe);
  i=list_add(&(nl->e[devt].l),p);
  return i;
}

int netlist_add_dev(netlist_t *nl, int devt)
{
  int i;
  assert(devt>=0);
  assert(devt<nl->qe);
  i=list_nextz(&(nl->e[devt].l));
  return i;
}

/* merge duplicate entities summing results 
   the user function tocompare returns a ptr to some static area
   which is filled with data that should be sorted and matched to

   the user function sum takes the indices of the two entities
   and performs the equivilent of [a]=[a]+[b]
   if sum returns 1 we remove item b
   so sum should frees any private data associated with [b] on 1
*/

void netlist_merge(netlistfunc_t *nm)
{
  mergedup_t *md;
  int i,ib,ia;
  void *data;
  char *buffer;
  list_t *entity=&(nm->entity->l);
  list_t newl;

  if(nm->tocompare==NULL)return;
  if(nm->sum==NULL)return;
  if(nm->qcmp<=0)return;
  buffer=malloc(nm->qcmp);
  assert(buffer!=NULL);
  memset(buffer,0,nm->qcmp);

  md=mergedup_alloc(list_qty(entity),nm->qcmp);
  
  list_iterate(entity,i,data)
    {
      nm->tocompare(nm,i,buffer);
      mergedup_fill(md,buffer,i);
    }
  free(buffer);
  buffer=NULL;
  mergedup_sort(md);
  ib=-1;
  ia=-1;
  while(1)
    {
      ib=mergedup_visit(md,0); /* get next same */
      if(ib==-1)
	{
	  i=ia;
	  ia=mergedup_visit(md,1); /* get next different */
	  if(ia==-1)break;
	}
      else
	{
	  if(nm->sum(nm,ia,ib))
	    {
	      /* flag delete item */
	      mergedup_setbit(md,ib);
	    }
	}
      
    }

  ia=0; /* count number of remaining elements */
  for(i=0;i<list_qty(entity);i++)
    if(!mergedup_testbit(md,i))ia++;

  list_copyfrom(&newl,entity); /* setup new list */
  list_hint(&newl,ia); /* pre-alloc data */ 
  list_iterate(entity,i,data)  /* fill new list */
    {
      if(!mergedup_testbit(md,i))
	list_add(&newl,data);
    }
  mergedup_free(md); /* free mergedup */
  list_empty(entity); /* free old list data */
  *entity=newl; /* copy over new pointers into old list */
}


/* distribute some attribute across entity 

   network of entities, connected via nodes.
   some attribute associated with an entity terminal
   first accumulate properties per node
   then distribute to all entities connected to that node
   
   ie: extract programs lump diffusion on one fet instead of distributing it as per width
   for resizing
*/

void netlist_distribute(netlistfunc_t *nm)
{
  int i,j;
  netlist_t *nl=nm->nl;
  list_t *entity=&(nm->entity->l);
  entity_t *e=nm->entity;
  int qterm=nm->entity->qterms;
  float *node_sums;

  if(nm->qproperties<=0)return;

  node_sums=malloc(sizeof(float)*list_qty(&(nl->e[DEVT_NODE].l))*nm->qproperties);
  assert(node_sums!=NULL);

  for(i=0;i< (list_qty(&(nl->e[DEVT_NODE].l))*nm->qproperties);i++)
    node_sums[i]=0.0;

  list_iterate_(entity,i)
    {
      for(j=0;j<qterm;j++)
	{
	  termptr_t t;
	  float *node_data;
	  t=NETLIST_E_TERM(e,i,j);
	  node_data=node_sums+ (netlist_termptr_devi(t)*nm->qproperties) ;
	  if(nm->accumulate!=NULL)
	    nm->accumulate(nm,i,j,node_data);
	}
    }

  list_iterate_(entity,i)
    {
      for(j=0;j<qterm;j++)
	{
	  termptr_t t;
	  float *node_data;
	  t=NETLIST_E_TERM(e,i,j);
	  node_data=node_sums+ (netlist_termptr_devi(t)*nm->qproperties) ;
	  if(nm->distribute!=NULL)
	    nm->distribute(nm,i,j,node_data);
	}
    }
  free(node_sums);
}



/*  fixup netlist node pointers

    when we first build a netlist, we have a many to one relationship
    for the entity terminals, ie. they all point to the associated node

    we also want to be able to enumerate the list of entity terminals on 
    a given net, without going through all entities and comparing.
    
    thus we have to fixup the netlist, making a linked list out of the terminal ptrs
    
    so we can walk through all the terminals on a net.
    also we put the actual node on the head of that list.

    we call a user function for each terminal on the net (to store back pointer if necessary)
*/

void netlist_fixup(netlistfunc_t *nm)
{
  netlist_t *nl=nm->nl;
  int i,j,k;
  int qterms;
  termptr_t t,nt;

  void *dp;
  void *np;

  /* init tail ptrs, to point to the head ptr */
  list_iterate(&(nl->e[DEVT_NODE].l),i,np)
    {
      termptr_t t;
      NETLIST_TERM(nl,DEVT_NODE,i,1)=t=netlist_termptr(i,0,DEVT_NODE);
      NETLIST_TERM(nl,DEVT_NODE,i,0)=netlist_termptr_null_t(t);
    }
  
  for(i=0;i<nl->qe;i++)
    {
      qterms=nl->e[i].qterms;
      list_iterate(&(nl->e[i].l),j,dp)
	{
	  for(k=0;k<qterms;k++)
	    {
	      termptr_t npn1;
	      t=NETLIST_TERM(nl,i,j,k);
	      assert(t.devt==DEVT_NODE);
	      nt=NETLIST_TERM(nl,t.devt,t.devi,1); /* tail ptr */
	      npn1=NETLIST_TERM(nl,t.devt,t.devi,1)=netlist_termptr(j,k,i);
	      NETLIST_TERM(nl,nt.devt,nt.devi,nt.termi)=npn1; /* also links n[0] */
	      /* call user routine if not avail */
	      if(nm->fixup!=NULL)
		nm->fixup(nm,t);
	    }
	}
    }
}



int netlist_findnode(netlist_t *nl, char *str)
{
  int pindex;
  pindex=names_check(nl->names,str);
  return pindex;
}


termptr_t netlist_node(netlist_t *nl, char *str, void *parent)
{
  int pindex;
  termptr_t tp;
  if(0)fprintf(stderr,"\nnode %s %p\n",str,parent);
  pindex=names_check(nl->names,str);
  if(pindex==-1)
    {
      pindex=netlist_add_dev(nl,DEVT_NODE);
      names_add(nl->names,pindex,str);
      tp=netlist_termptr(pindex,0,DEVT_NODE);
      NETLIST_TERM(nl,DEVT_NODE,pindex,0)=tp;
      NETLIST_TERM(nl,DEVT_NODE,pindex,1)=tp;
    }
  else
    {
      tp=netlist_termptr(pindex,0,DEVT_NODE);
    }
  return tp;
}



int netlist_newdev_fromnode(netlist_t *nl, int devt, dev_input_t parent , termptr_t n[])
{
  int q,qv,i,index;
  
  qv =NETLIST_QVALS  (nl,devt);
  q  =NETLIST_QTERMS (nl,devt);
  
  index=netlist_add_dev(nl,devt);
  NETLIST_PARENT(nl,devt,index)=parent;

  for(i=0;i<q;i++)
    NETLIST_TERM(nl,devt,index,i)=n[i];

  for(i=0;i<qv;i++)
    NETLIST_VAL(nl,devt,index,i)=eqn_empty();
      
  return index;
}


void netlist_init(netlist_t *nl)
{
  int i;
  entity_t e=ENTITY_INVALID;
  netlist_machine();
  for(i=0;i<TERMPTR_MAX_DEVT;i++)
    nl->e[i]=e;
  nl->qe=0;
  nl->names=names_new();
  nl->input.p=NULL;
  nl->eqn_mem=eqn_mem_new();
  nl->iscopy=0;
  eqnl_init(&(nl->eqnl));
}


int netlist_register_entity (netlist_t *nl, int id, int qterms, int qvals, int qrest, char *sym, int othersize)
{
  entity_t *ep;
  
  if(nl->qe>=TERMPTR_MAX_DEVT)
    return -1;
  
  ep=&(nl->e[nl->qe]);
  nl->qe++;
  ep->id=id;
  ep->qterms=qterms;
  ep->qvals=qvals;
  strncpy(ep->sym,sym,8);
  ep->sym[7]=0;
  ep->qcount=0;
  ep->qother=qrest;
  list_init(&(ep->l),NETLIST_OFFSET_OTHER((*ep))+othersize,LIST_DEFMODE);
  return nl->qe-1;
}

netlist_input_t netlist_parse_input(scanner_t *scan, char *type, netlist_param_t *params)
{
  netlist_input_t p;
  
  netlist_machine();
  p.p=NULL;

  if(strcmp(type,"spice")==0)
    {
      p.spice=spice_new(scan);
    }
  if(strcmp(type,"extract")==0)
    {
      /* p.ext=extract_new(scan); */
    }
  if(p.p==NULL)
    parse_error(scan, "Netlist: can't parse input of type %s",type);
  return p;
}



void netlist_debug_input(void * dbg_fp, netlist_input_t p)
{
  FILE *dbg=dbg_fp;
  netlist_machine();
  
  if(p.p==NULL)
    fprintf(dbg, "can't debug netlist input, null ptr");
  
  if(p.generic->magic==SPICE_MAGIC)
    spice_debug(dbg,p.spice);
  
  else if(p.generic->magic==EXTRACT_MAGIC)
    /* p.ext=extract_debug(dbg,p.extract) */ ;

}

void netlist_debug(void * dbg_fp, netlist_t *nl)
{
  netlist_debug_(dbg_fp,nl,0);
}


void netlist_debug_(void * dbg_fp, netlist_t *nl, int eval)
{
  FILE *dbg=dbg_fp;
  int i;

  if(nl==NULL)
    fprintf(dbg,"can't debug netlist, null ptr");
  
  if(eval)
    {
      if(0)fprintf(stderr,"\n\nbegin evaldep \n\n");
      eqnl_evaldep(&(nl->eqnl));
      if(0)fprintf(stderr,"\n\ndone evaldep \n\n");
    }
  
  fprintf(dbg,"debugging netlist eval=%i,  %p\n",eval,nl);
  for(i=0;i<nl->qe;i++)
    {
      entity_t *e;
      int j;
      eqn_t *v;
      
      e=nl->e+i;
      fprintf(dbg,"   e %i %s, qterm=%i qcount=%i l.q,m=%i,%i\n",
		  i,e->sym,e->qterms,e->qcount,e->l.q,e->l.m
		  );
      list_iterate_(&(e->l),j)
	{
	  int k;
	  fprintf(dbg,"%i:  ",j);
	  
	  for(k=0;k<e->qterms;k++)
	    {
	      termptr_t t=NETLIST_E_TERM(e,j,k);
	      fprintf(dbg,"  [%i %s.%i]",t.devi,nl->e[t.devt].sym,t.termi);
	      if(i==DEVT_NODE)
		fprintf(dbg,"=%s ",names_lookup(nl->names,j));
	    }
	  fprintf(dbg,"\n  ");
	  
	  v=NETLIST_VALS(nl,i,j);
	  for(k=0;k<e->qvals;k++)
	    {
	      if(!eval)
		{
		  char buf[32];
		  sprintf(buf," %i",k);
		  debug_eqn(dbg,buf,v+k);
		  fprintf(dbg," ");
		}
	      else
		{
		  float f;
		  f=eqnl_eval(&(nl->eqnl),v[k]);
		  fprintf(dbg," ev %g ",f);
		}
	    }
	  fprintf(dbg,"\n");
	}
    }
}





/* release input from netlist (free structures if possible) 
   after this call, all parent ptrs and input ptrs are set to null
*/
void netlist_release(netlist_t *nl)
{
  int i,j;

  /* reset all the parent ptrs to null */
  for(i=0;i<nl->qe;i++)
    {
      list_iterate_(&(nl->e[i].l),j)    
	NETLIST_PARENT(nl,i,j).p=NULL;
    }
  
  /* call low level free function */
  switch(nl->input.generic->magic)
    {
    case SPICE_MAGIC:
      spice_release(nl->input.spice);
      break;
      
    case EXTRACT_MAGIC:
      /* extract_release(nl->input.ext); */
      break;
    }
  nl->input.p=NULL;  
}



/* this releases input if not already done 
   and then frees up the memory associated with the netlist
   any additional client memory should already have been freed
*/

void netlist_free(netlist_t *nl)
{
  int i;
  if(nl!=NULL)
    {
      if(nl->input.p!=NULL)
	netlist_release(nl);
      
      for(i=0;i<TERMPTR_MAX_DEVT;i++)
	list_empty(&(nl->e[i].l));
      
      eqn_mem_free(nl->eqn_mem);
      nl->eqn_mem=NULL;
      if(!nl->iscopy)
	{
	  eqnl_free(&(nl->eqnl));
	  names_free(nl->names);
	}
      free(nl);
      nl=NULL;
    }
}



int netlist_print_nodep(netlist_t *n, netprint_t grp, void * file_fp, void *gn, char *str)
{
  FILE *file=file_fp;
  int index=-1;
  const char *err="node_error";
  char *name="error";
  int i;

  fprintf(file,"%s",str);
  if(n!=NULL)
    index=list_index(&(n->e[DEVT_NODE].l),gn);
  
  switch(grp)
    {
    case netprint_none:
      break;
    case netprint_name:
      if(index!=-1)
	fprintf(file,"%s",names_lookup(n->names,index));
      else fprintf(file,err);
      break;
    case netprint_index:
      if(index!=-1)
	fprintf(file,"n%i",index);
      else fprintf(file,err);
    case netprint_ptr:
      fprintf(file,"N_%p",gn);
    case netprint_nname:
      if(index!=-1)
	name=names_lookup(n->names,index);
      i=strlen(name)-8;
      if(i<0)i=0;
      fprintf(file,"N%i_%s",index,name+i);
      break;

    case netprint_debug:
      if(index!=-1)
	fprintf(file,"[Node %p %i %s]",gn,index,names_lookup(n->names,index));
      else
	fprintf(file,"[Node %p]",gn);
      break;
    }
  return 0;
}

void netlist_eval(netlist_t *n)
{
  eqnl_evaldep(&(n->eqnl));
}



/* this is somewhat inefficient, please don't rely on it  */
netlist_t *netlist_copyisher(netlist_t *in, int extrasize, int extrasizes[])
{
  char buffer[4096];
  int i,j,sin;
  void *p;
  netlist_t *nl;
  netlist_machine();
  nl=malloc(sizeof(netlist_t)+extrasize);
  assert(nl!=NULL);
  netlist_init(nl);
  nl->iscopy=1;
  for(i=0;i<in->qe;i++)
    {
      entity_t *ep;
      int index;
      ep=in->e+i;
      index=netlist_register_entity(nl,ep->id,ep->qterms,ep->qvals,ep->qother,ep->sym,extrasizes[i]);
      assert(sizeof(buffer)> (extrasizes[i]+NETLIST_OFFSET_OTHER((*ep))));
      assert(index>=0);
      memset(buffer,0,sizeof(buffer));
      sin=list_sizeof(&(in->e[i].l));
      list_hint(&(nl->e[index].l),sin);
      list_iterate(&(ep->l),j,p)
	{
	  memcpy(buffer,p,sin); /* yes, we copy twice. this is required, think about it */
	  list_add(&(nl->e[index].l),buffer);
	}
    }

  /* POTENTIAL MEMORY LEAK HERE */
  nl->names=in->names;  /* share name database */
  nl->eqnl=in->eqnl;    /* share eqnl database */
  nl->input.p=NULL;     /* flag this to null   */

  return nl;
}


