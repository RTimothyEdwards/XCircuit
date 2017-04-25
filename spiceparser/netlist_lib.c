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
/* netlist_lib.c, routines for building a library of netlists 
   Conrad Ziesler
*/



/* the goal here is to be able to read/write a file composed of segments
   each segment has a header describing 

   1. reference name 
   2. equation parameters to iterate through
   3. logic function
   4. output ports, output tokens
   5. input ports,  input tokens
   6. spice format netlist block to be parsed by netlist_spice

   we then maintain this library of each version of each gate

   in addition we would like to procedurally add gates to our library.
   (ie. automatically generate library from templates)
   
   
*/
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "debug.h"
#include "netlist_lib.h"
#include "netlist_dev.h"

int __debug_nlib__=1;

#ifndef STRIP_DEBUGGING
#define D(level,a) do { if(__debug_nlib__>(level)) a; } while(0)
#else
#define D(level,a) 
#endif



/* add a new param, to a db */
int nlib_add_pm(nlib_t *nlib, int dbindex, char *str, float min, float max, int qty)
{
  nlibpm_t template;
  nlibdb_t *db;
  int index;
  template.min=min;
  template.max=max;
  template.qty=qty;
  db=list_data(&(nlib->db),dbindex);
  if(db==NULL)assert(0);
  if(str!=NULL)
    template.index=eqnl_find(&(db->nl->eqnl),str);
  else template.index=-1;

  if(template.index<0)
    { fprintf(stderr,"couldn't find parameter %s used in netlist, assuming ignoring\n",str); return -1; }

  index=list_add(&(db->params),&template);
  return index;
}

/* add a new io definition to a db */
int nlib_add_io(nlib_t *nlib, int dbindex, nlibiol_t *io, int tokens, int nodes, int dig, char **names)
{
  int index,ni;
  int i;
  nlibio_t *p,template;
  nlibdb_t *db;
  db=list_data(&(nlib->db),dbindex);
  assert(db!=NULL);

  memset(&template,0,sizeof(template));
  index=list_add(&(io->iol),&template);
  p=list_data(&(io->iol),index);

  assert((nodes<NLIBIO_MAXNODES) && "Compile time node limit exceeded, change and recompile");

  io->qtokens= (io->qtokens>0)?io->qtokens*tokens : tokens;
  p->tokens=tokens;
  p->qnodes=nodes;
  p->digitize=dig;
  for(i=0;i<nodes;i++)
    {
      ni=names_check(db->nl->names,names[i]);
      if(ni<0) 
	{ 
	  fprintf(stderr,"NLIB: could not find i/o node %s in circuit\n",names[i]);
	  assert(0);
	}
      p->nodes[i]=ni;
    }
  return index;
}


static void nlib_init_io(nlibiol_t *io)
{
  io->qtokens=0;
  list_init(&(io->iol),sizeof(nlibio_t),LIST_UNITMODE);
}


int nlib_db_netlist(nlib_t *nlib, int index, netlist_t *nl)
{
  nlibdb_t *db;
  db=list_data(&(nlib->db),index);
  assert(db!=NULL);
  db->nl=nl;
  return index;
}



/* makes a new db */
int nlib_new_db(nlib_t *nlib, char *str)
{
  int index;
  nlibdb_t template,*db;
  
  memset(&template,0,sizeof(template));
  index=list_add(&(nlib->db),&template);
  names_add(nlib->dbref,index,str);
  db=list_data(&(nlib->db),index);
  list_init(&(db->params),sizeof(nlibpm_t),LIST_DEFMODE);
  nlib_init_io(&(db->in));
  nlib_init_io(&(db->out));
  list_init(&(db->refnodes),sizeof(nlibrefnode_t),LIST_UNITMODE);
  return index;  
}


nlibfref_t nlib_func_lookup(nlib_t *nlib, char *str)
{
  nlibfref_t f;
  f.index=names_check(nlib->funcnames,str);
  return f;
} 

/* fixme:  should look up name of function in nlib->functions and 
   use that index 
*/
int nlib_db_func(nlib_t *nlib, int index, char *str)
{
  nlibdb_t *db;
  db=list_data(&(nlib->db),index);
  assert(db!=NULL);
  db->map=nlib_func_lookup(nlib,str);
  return 0;
}

/* makes empty function of specified variables */
nlibfunc_t *nlib_new_func(nlib_t *nlib, char *str, int qin, int qout,int p_delay, int p_offset, int p_cycle)
{
  int l;
  nlibfunc_t *fp;
  fp=malloc((l=sizeof(nlibfunc_t)+(sizeof(int)*qin)));
  assert(fp!=NULL);
  memset(fp,0,l);
  fp->qin=qin;
  fp->qout=qout;
  fp->p_delay=p_delay;
  fp->p_offset=p_offset;
  fp->p_cycle=p_cycle;
  l=list_add(&(nlib->funcs),&fp);
  names_add(nlib->funcnames,l,str);
  return fp;
}
/* specify one mapping */
void nlib_func_addmap(nlib_t *nlib, nlibfunc_t *fp, int in, int out)
{
  assert(in<fp->qin);
  assert(out<fp->qout);
  assert(in>=0);
  assert(out>=0);
  fp->map[in]=out;
}



/* inits a nlib, call before any action on nlib */
int nlib_init(nlib_t *nlib)
{
  list_init(&(nlib->db),sizeof(nlibdb_t),LIST_DEFMODE);
  nlib->dbref=names_new();
  list_init(&(nlib->iodefs),sizeof(nlibiodef_t), LIST_DEFMODE);
  eqnl_init(&(nlib->eqnl));
  list_init(&(nlib->funcs), sizeof(nlibfunc_t *), LIST_DEFMODE); /* list of pointers */
  list_init(&(nlib->flatdb), sizeof(nlibflatdb_t), LIST_DEFMODE);
  nlib->funcnames=names_new();
  nlib->qvref=0;
  nlib->vref=names_new();
  nlib->flags=NULL;
  nlib->eqn_mem=eqn_mem_new();
  return 0;
}  

void nlib_release_flatdb(nlib_t *nlib)
{
  int i;
  nlibflatdb_t *fdbp;
  list_iterate(&(nlib->flatdb),i,fdbp)
    {
      list_empty(&(fdbp->uservals));
      if(fdbp->pvals!=NULL)free(fdbp->pvals);
      fdbp->pvals=NULL;
    }
  list_empty(&(nlib->flatdb));
  bitlist_free(nlib->flags);
  nlib->flags=NULL;
}

void nlib_release_io(nlib_t *nlib, nlibiol_t *io)
{
  list_empty(&(io->iol));
}

void nlib_release_db(nlib_t *nlib)
{
 int i;
 nlibdb_t *dbp;
   
 list_iterate(&(nlib->db),i,dbp)
   {
     if(dbp->pvals!=NULL)free(dbp->pvals);
     dbp->pvals=NULL;
     list_empty(&(dbp->refnodes));
     list_empty(&(dbp->params));
     nlib_release_io(nlib,&(dbp->in));
     nlib_release_io(nlib,&(dbp->out));
     netlist_free(dbp->nl);
     dbp->nl=NULL;
   }
 list_empty(&(nlib->db));
 names_free(nlib->dbref);
 nlib->dbref=NULL;
}

void nlib_release_funcs(nlib_t *nlib)
{
  int i;
  void **p;
  names_free(nlib->funcnames);
  nlib->funcnames=NULL;
  list_iterate(&(nlib->funcs),i,p)
    {
      if((*p)!=NULL)
	free((*p));
    }
  list_empty(&(nlib->funcs));
}

void nlib_release_refs(nlib_t *nlib)
{
  names_free(nlib->vref);
  nlib->vref=NULL;
  nlib->qvref=0;
}

void nlib_release_iodefs(nlib_t *nlib)
{
  int i;
  nlibiodef_t *p;
  list_iterate(&(nlib->iodefs),i,p)
    {
      if(p->encoding!=NULL)free(p->encoding);
      p->encoding=NULL;
    }
 list_empty(&(nlib->iodefs));

}
/* frees all data in nlib */
void nlib_release(nlib_t *nlib)
{
  nlib_release_flatdb(nlib);
  nlib_release_db(nlib);
  nlib_release_refs(nlib);
  nlib_release_funcs(nlib);
  eqnl_free(&(nlib->eqnl));
  nlib_release_iodefs(nlib);
  eqn_mem_free(nlib->eqn_mem);
  nlib->eqn_mem=NULL;
}



/* post processing of db structure */
int nlib_db_fixup(nlib_t *nlib, nlibdb_t *db)
{
  int i,q,j;
  nlibpm_t *p;
  nlibrefnode_t *rnp;
  int l=list_qty(&(db->params));
  db->pvals=malloc((sizeof(float)*l));
  memset(db->pvals,0,sizeof(float)*l);
  list_shrink(&(db->params));
  list_shrink(&(db->refnodes));

  list_iterate(&(db->params),i,p)
    {
      if(0)fprintf(stderr,"nlib_db_fixup: trying to define eqn: %i->%s (%p)\n",p->index,eqnl_lookup(&(db->nl->eqnl),p->index),db->pvals+i);
      eqnl_define(&(db->nl->eqnl),p->index,db->pvals+i);
    }
  q=eqnl_qty(&(db->nl->eqnl));

  /* setup equations in our reference nodes */
  list_iterate(&(db->refnodes),i,rnp)
    {
      for(j=0;j<RTqty;j++)
	eqnl_depend(&(nlib->eqnl), rnp->skew[j]);
    }

  return 0;
}


/* step through library, 
   (in terms of param qty's, in order)
   substitute in, and call client process function
   for each instance of each cell
*/
void nlib_process(nlib_process_t *user, int screened)
{
  int i,j,k,lib_index;
  nlib_t *nlib=user->nlib;
  nlibdb_t *dbp;
  nlibpm_t *dbpm;
  int li[64]; /* max 64 parameters */
  int lq[64];
  int qp=0;

  assert(nlib!=NULL);
  memset(li,0,sizeof(li));
  memset(lq,0,sizeof(lq));
  lib_index=0;
  list_iterate(&(nlib->db),i,dbp)
    {
      qp=0;
      list_iterate(&(dbp->params),j,dbpm)
	{
	  lq[j]=dbpm->qty;
	  li[j]=0; 
	  dbp->pvals[j]= dbpm->min;
	  qp++;
	  if(qp>=64)assert(0);
	}
      k=0;
      fprintf(stderr,"\n\nstarting db index %i\n",i);
      while(1)
	{
	  if(0)
	    {
	      fprintf(stderr,"iteration:");
	      for(j=0;j<qp;j++)
		fprintf(stderr," (%i,%i)",li[j]+1,lq[j]); 
	      fprintf(stderr,"\n");
	    }

	  if(nlib_isdisabled(nlib,lib_index)!=1)
	    {
	      /* eval each parameters */
	      for(j=0;j<qp;j++) 
		{
		  dbpm=list_data(&(dbp->params),j);
		  
		  dbp->pvals[j]=dbpm->min+
		    li[j]*(dbpm->max-dbpm->min)/dbpm->qty;
		  
		  if(0)fprintf(stderr,"setting %i (%p) val[%i]=%g\n",j,dbp->pvals+j,dbpm->index,dbp->pvals[j]);
		}	  
	      
	      user->nl=dbp->nl;
	      user->lib_index=lib_index;
	      user->db_index=i;
	      user->process(user);
	    }
	  lib_index++;
	  /* increment loop */
	  for(j=0;j<qp;j++)
	    if(li[j]<(lq[j]-1)) 
	      {
		li[j]++;
		for(j--;j>=0;j--) /* roll over rest of indices */
		    li[j]=0;
		break; 
	      }
	    if(j==qp)break;
	}
    }
  
}


static void nlib_fixup_process(nlib_process_t *user)
{
  int *ip=user->user;
  ip[0]++;
}

void nlib_fixup(nlib_t *nlib)
{
  int count=0;
  nlib_process_t pr;
  
  eqnl_autodepend(&(nlib->eqnl));
  eqnl_evaldep(&(nlib->eqnl));
  if(nlib->flags!=NULL) /* in case we were called multiple times */
    bitlist_free(nlib->flags);
  nlib->flags=NULL;
  if(0)
    {
      pr.user=&count;
      pr.nlib=nlib;
      pr.process=nlib_fixup_process;
      nlib_process(&pr,0);
      nlib->flags=bitlist_new(count+1);
    }
}

/* this does the dirty work of updating configurable db netlists from flat db */
void nlib_doflat(nlib_t *nlib, int index)
{
  nlibflatdb_t *fdb;
  nlibdb_t *db;
  assert(nlib!=NULL);
  fdb=list_data(&(nlib->flatdb),index);
  assert(fdb!=NULL);
  db=list_data(&(nlib->db),fdb->db_index);
  assert(db!=NULL);
  if(fdb->pvals!=NULL)
    memcpy(db->pvals,fdb->pvals,sizeof(float)*fdb->qvals);
  else
    assert(fdb->qvals==0);  
}

/* call this from user process for each gate we want to keep */
void *nlib_flat(nlib_process_t *user, int s_uservals, int q_uservals)
{
  nlib_t *nlib;
  nlibdb_t *db;
  int index;
  nlibflatdb_t *fdb;
  
  assert(user!=NULL);
  nlib=user->nlib;
  assert(nlib!=NULL);
  index=list_nextz(&(nlib->flatdb));
  fdb=list_data(&(nlib->flatdb),index);
  fdb->lib_index=user->lib_index;
  fdb->db_index=user->db_index;
  db=list_data(&(nlib->db),fdb->db_index);
  assert(db!=NULL);
  fdb->qvals=list_qty(&(db->params));
  
  list_init(&(fdb->uservals),s_uservals,LIST_UNITMODE);
  list_hint(&(fdb->uservals),q_uservals);
  
  if(fdb->qvals>0)
    {
      fdb->pvals=malloc(sizeof(float)*fdb->qvals);
      assert(fdb->pvals!=NULL);
      memcpy(fdb->pvals,db->pvals,sizeof(float)*fdb->qvals);
    }
  else fdb->pvals=NULL;
  
  return list_block_next(&(fdb->uservals),q_uservals);
}


void nlib_disable(nlib_t *nlib, int index)
{
  if(nlib->flags!=NULL)
    bitlist_set(nlib->flags,index);
}

int nlib_isdisabled(nlib_t *nlib, int index)
{
  if(nlib->flags!=NULL)
    return bitlist_test(nlib->flags,index);
  return -1;
}


static void nlib_o_spice_subckt_(nlib_t *nlib, int db_index, int lib_index, void * of_fp)
{
  FILE *of=of_fp;
  nlibdb_t *dbp;
  nlibrefnode_t *rnp;
  nlibio_t *iop;
  int i,j;
  int z_index=1;
  int n_start=1;

  dbp=list_data(&(nlib->db),db_index);
  assert(dbp!=NULL);
  fprintf(of,"\n.subckt sc%i", lib_index);
  
  /* reference nodes */
  list_iterate(&(dbp->refnodes),i,rnp)
    fprintf(of," rn%i",rnp->vrefi);
    
  /* input nodes */
  list_iterate(&(dbp->in.iol),i,iop)
    for(j=0;j<iop->qnodes;j++)
      fprintf(of," %i",iop->nodes[j]+n_start);
  
  /* output nodes */
  list_iterate(&(dbp->out.iol),i,iop)
    for(j=0;j<iop->qnodes;j++)
      fprintf(of," on%i_%i",i,j);
  
  fprintf(of,"\n");

  /* call netlist_o_spice to output subckt netlist */
  netlist_o_spice(of, dbp->nl, n_start);

  /* add additional circuitry for output buffers */
  list_iterate(&(dbp->out.iol),i,iop)
    for(j=0;j<iop->qnodes;j++)
      fprintf(of,"Ez%i on%i_%i 0 VCVS PWL(1) %i 0 -10v,-10v 10v,10v\n",z_index++,i,j, iop->nodes[j]+n_start);
  
  /* add aditional circuitry for delay and rc of references */

  list_iterate(&(dbp->refnodes),i,rnp)
    {
      float f[4]={0.0, 0.0, 0.0, 0.0 };
      int ud[4];
      assert(RTqty<=3);
      for(j=0;j<RTqty;j++)
	{
	  ud[j]=(eqn_is_undefined(rnp->skew[j])!=0);
	  f[j]=eqnl_eval(&(nlib->eqnl),rnp->skew[j]);
	}
      
      if( ud[RTdelay] && ud[RTrseries] ) 
	fprintf(of,"rz%i rn%i %i 1e-3\n", z_index++,rnp->vrefi,rnp->devnodei+n_start);
	
      else if (!ud[RTdelay] && ud[RTrseries])
	fprintf(of,"Ez%i %i 0 VCVS DELAY rn%i 0 TD=%g SCALE=1\n",
		    z_index++, rnp->devnodei+n_start, rnp->vrefi, f[RTdelay]
		    );
      
      else if ( ud[RTdelay] && !ud[RTrseries])
	fprintf(of,"rz%i rn%i %i %g\n",z_index++, rnp->vrefi, rnp->devnodei+n_start, f[RTrseries]);
      
      else
	{
	  fprintf(of,"dz%i rn%i rn%ia %g\nrz%i rn%ia %i %g\n",
		    z_index, rnp->vrefi, rnp->vrefi, f[RTdelay],
		    z_index+1, rnp->vrefi, rnp->devnodei+n_start, f[RTrseries]
		    );
	  z_index+=2;
	}
      if(!ud[RTcshunt])
	fprintf(of,"cz%i %i 0 %g\n",z_index++,rnp->devnodei+n_start, f[RTcshunt] );
    }
  fprintf(of,".ends sc%i\n",lib_index);

}


void nlib_o_spice_refs(nlib_t *nlib, void * of_fp)
{
  FILE *of=of_fp;
  int i;
  char *p;
  for(i=0;(p=names_lookup(nlib->vref,i))!=NULL;i++)
    {
      fprintf(of,"*nref %s\n",p);
    }
}


static void nlib_o_spice_subckt_process(nlib_process_t *user)
{  nlib_o_spice_subckt_(user->nlib,user->db_index,user->lib_index, user->user); }

void nlib_o_spice_subckts(nlib_t *nlib, void * of_fp)
{
  FILE *of=of_fp;
  /* if we built a flat db, use it */
  if(list_qty(&(nlib->flatdb))>0)
    {
      int i,j; 
      nlibflatdb_t *fdb;
      float *dp;

      list_iterate(&(nlib->flatdb),i,fdb)
	{
	  /* print header with some usefull info */
	  fprintf(of,"****** index=%i  db_index=%i  lib_index=%i ***",
		      i,fdb->db_index,fdb->lib_index
		      );
	  list_iterate(&(fdb->uservals),j,dp)
	    fprintf(of,"%s%g",((j%5)==0)?"\n***  ":"  ",*dp);
	  fprintf(of,"\n");
	  
	  nlib_doflat(nlib,i); /* update the netlist */
	  nlib_o_spice_subckt_(nlib,fdb->db_index,fdb->lib_index, of); 
	}
    }
 else
    {
      nlib_process_t proc;
      proc.nlib=nlib;
      proc.user=of;
      proc.process=nlib_o_spice_subckt_process;
      nlib_process(&proc,1);
    }
}





/************ TOFIX:  NEED to add multiple input nodes
	      probably the easiest is to pass a list ptr of i_nodes

tree.c needs some helper function in netlist_lib.c which gives the number of inputs
a given gate needs, so it can figure out it's proper wiring topology.
perhaps we should allocate the list of input nodes within the flatdb structure
and then tree would only have to pass *which* node to change in the list?
or at the very least, tree.c won't have to allocate a bunch of random lists and keep them 
current.

************/




/* output spice style subckt call, mapping i_node and o_node */
void nlib_o_spice_call(nlib_t *nlib, int index, int i_node, int o_node, int *skt, void * of_fp)
{
  FILE *of=of_fp;
  nlibflatdb_t *fdb;
  nlibdb_t *db;
  nlibio_t *iop;
  nlibrefnode_t *rnp;
  int i,j;
  char tmp[64];

  assert(nlib!=NULL);
  fdb=list_data(&(nlib->flatdb),index);
  assert(fdb!=NULL);
  db=list_data(&(nlib->db),fdb->db_index);
  assert(db!=NULL);
  
  fprintf(of,"X%i ",(*skt)++);
  
  /* reference nodes */
  list_iterate(&(db->refnodes),i,rnp)
    fprintf(of," %s",names_lookup(nlib->vref,rnp->vrefi));
  
  if(i_node==-1)sprintf(tmp,"root");
  else sprintf(tmp,"n%i",i_node);

  /* input nodes, for multi-input gate, tie inputs together  */
  list_iterate(&(db->in.iol),i,iop)
    for(j=0;j<iop->qnodes;j++)
      fprintf(of," %s_%i",tmp,j);
  
  /* output nodes, for multi-output gate, tie outputs together  */
  list_iterate(&(db->out.iol),i,iop)
    for(j=0;j<iop->qnodes;j++)
      fprintf(of," n%i_%i",o_node,j);
  
  fprintf(of," sc%i",fdb->lib_index);
  fprintf(of,"\n");
  fprintf(of,"*nlib %i %i %i %i\n",i_node,o_node,fdb->db_index,index);  
}

void nlib_iface_release(nlib_iface_t *iface)
{
  list_empty(&(iface->icalls));
  list_empty(&(iface->frefs));
}

void nlib_iface_init(nlib_iface_t *iface, nlib_t *nlib)
{
  memset(iface->ref_map,0,sizeof(iface->ref_map));
  list_init(&(iface->icalls),sizeof(nlib_icall_t),LIST_DEFMODE);
  list_init(&(iface->frefs),sizeof(nlib_ifref_t),LIST_DEFMODE);
  iface->nlib=nlib;
  iface->phase=0;
}

nlib_ifref_t *nlib_iface_getref(nlib_iface_t *iface, int id)
{
  nlib_ifref_t *rp;
  assert(id<NLIBref_max);
  assert(id>=0);
  rp=list_data(&(iface->frefs),iface->ref_map[id]);
  assert(rp!=NULL);
  return rp;
}

static int nlib_digitize(float *vals[], int digitize, int tokens[], nlib_iface_t *iface)
{
  int i;
  int token=-1;
  float dv;
  float vdd,vss;
  
  switch(digitize)
    {
    case 0: /* default */
      assert(0);
      break;
    case 1: /* delta v */
      assert(vals[0]!=NULL);
      assert(vals[1]!=NULL);
      dv=(*vals[0])-(*vals[1]);
      if(dv>(1e-3))token=1;
      if(dv<(-1e-3)) token=0;
      break;
    case 2:  /* cmos */
      assert(vals[0]!=NULL);
      dv=*vals[0];          
      vss=nlib_iface_getref(iface,NLIBref_vss)->val[0];
      vdd=nlib_iface_getref(iface,NLIBref_vdd)->val[0];
      if((2*dv)>(vdd-vss)) token=1;
      else token=0;
      break;
    default:
      assert(0);
      break;
    }

  /* update tokens[phases] current phase is always tokens[0].. */
  if(tokens!=NULL)
    {
      for(i=(NLIB_MAXPHASES-1);i>=1;i--)
	tokens[i]=tokens[i-1];
      tokens[0]=token;
    }
  return token;
}

int nlib_func_compute(nlibfunc_t *map, int input)
{
  assert(map!=NULL);
  if(input<0)return -1;
  assert(input<map->qin);
  return map->map[input];
}


int nlib_icall_digitize(nlib_iface_t *iface, int calli, int phase)
{
  nlib_icall_t *cd;
  nlibdb_t *dp;
  nlibfunc_t *fp,**fpp;

  cd=list_data(&(iface->icalls),calli);
  assert(cd!=NULL);
  dp=list_data(&(iface->nlib->db),cd->dbi);
  assert(dp!=NULL);
  fpp=list_data(&(iface->nlib->funcs),dp->map.index);
  assert(fpp!=NULL);
  fp=*fpp;
  assert(fp!=NULL);
  
  nlib_digitize(cd->in,  cd->idigitize, cd->itokens, iface);
  nlib_digitize(cd->out, cd->odigitize, cd->otokens, iface);
  
  if(phase> (fp->p_delay+fp->p_offset)) /* wait at least this long before checking valididy */
    if( (phase%fp->p_cycle)== (fp->p_delay%fp->p_cycle) ) /* are we on the correct phase */
	{
	  cd->ctoken=nlib_func_compute(fp,cd->itokens[(fp->p_offset%fp->p_delay)]);
	  if(cd->ctoken!=cd->otokens[0]) cd->err++;
	  else cd->ver++;
	}
  return cd->err;
}

void nlib_iface_phase(nlib_iface_t *iface)
{
  int i;
  nlib_icall_t *cp;
  iface->phase++;
  
  list_iterate(&(iface->icalls),i,cp)
    nlib_icall_digitize(iface,i,iface->phase);
}

int nlib_iface_do_call(nlib_iface_t *iface, int in, int on, int dbi, int fdbi)
{
  nlib_t *nlib=iface->nlib;
  nlibdb_t *db;
  char nn[64],*nnp;
  int j;
  nlib_icall_t *cd;
  nlibio_t *iop;
  int z;

  db=list_data(&(nlib->db),dbi);
  if(db==NULL)return 0;

  z=list_qty(&(iface->icalls));
  cd=list_next(&(iface->icalls));
  cd->dbi=dbi;
  cd->flatdbi=fdbi;
  cd->err=0;
  cd->ver=0;
  for(j=0;j<NLIBIO_MAXNODES;j++)
    {
      cd->in[j]=NULL;
      cd->out[j]=NULL;
    }
  for(j=0;j<NLIB_MAXPHASES;j++)
    cd->itokens[j]=cd->otokens[j]=-1;

  cd->ctoken=-1;
  cd->inode=in;
  cd->onode=on;

  /*** lookup input nodes from tracefile interface *****/

  if(in>-1)
    sprintf(nn,"n%i_",in);
  else sprintf(nn,"root_");
  
  nnp=nn+strlen(nn);
  
  iop=list_data(&(db->in.iol),0); /* only handle inverters right now */
  assert(iop!=NULL);
  for(j=0;j<iop->qnodes;j++)
    {
      sprintf(nnp,"%i",j);
      cd->in[j]=iface->lookup_node_f(iface->user, nn);
      if(cd->in[j]==NULL)
	fprintf(stderr,"ERROR: did not find trace node %s\n",nn);
    }
  cd->idigitize=iop->digitize;
  
  /*** lookup output nodes from tracefile interface  ****/

  sprintf(nn,"n%i_",on);
  nnp=nn+strlen(nn);
  
  iop=list_data(&(db->out.iol),0); /* only handle inverters right now */
  assert(iop!=NULL);
  for(j=0;j<iop->qnodes;j++)
    {
      sprintf(nnp,"%i",j);
      cd->out[j]=iface->lookup_node_f(iface->user, nn);
      if(cd->out[j]==NULL)
	fprintf(stderr,"ERROR: did not find trace node %s\n",nn);
    }
  cd->odigitize=iop->digitize;

  return z;
}

int nlib_i_spice_calls(nlib_iface_t *iface, void * cktf_fp)
{
  FILE *cktf=cktf_fp;
  int dbi,fdbi;
  int in,on;
  char line[4096];
  int q=0;
  int i;
  nlib_ifref_t ifref;

  memset(line,0,sizeof(line));
  while(!feof(cktf))
    {
      line[0]=0;
      line[1]=0;
      line[sizeof(line)-1]=0;
      line[sizeof(line)-2]=0;
      fgets(line,sizeof(line-2),cktf);
      i=strlen(line)-1;
      while(i>=0)  { if(isspace(line[i])) line[i]=0; else break; i--; }
 
      if( ( line[0]=='*') && (line[1]=='n') && (line[2]=='l') && (line[3]=='i') && (line[4]=='b') )
	{
	  dbi=-1;
	  fdbi=-1;
	  in=-2;
	  on=-2;
	  sscanf(line+5," %i %i %i %i",&in,&on,&dbi,&fdbi);
	  if(fdbi==-1)continue;
	  q+=nlib_iface_do_call(iface,in,on,dbi,fdbi);
	  continue;
	}
      
      if( (line[0]=='*') && (line[1]=='n') && (line[2]=='r') && (line[3]=='e') && (line[4]=='f'))
	{
	  if(line[5]!=' ')continue;
	  memset(&ifref,0,sizeof(ifref));
	  
	  for(i=6;line[i]!=0;i++)
	    {
	      if(isspace(line[i]))break;
	      ifref.name[i-6]=line[i];
	    }
	  
	  for(i=0;i<NLIB_MAXPHASES;i++) ifref.vals[i]=0.0;
	  ifref.val=iface->lookup_node_f(iface->user,ifref.name);
	  dbi=list_add(&(iface->frefs),&ifref);
	  /* this is all just a big consistancy check */
	  fdbi=names_check(iface->nlib->vref,ifref.name);
	  if(dbi!=fdbi)
	    {
	      fprintf(stderr,"FATAL ERROR: this database file does not seem to match given tracefile\n");
	      exit(0);
	    }
	}
      continue;
    }
  
  return q;
}


/* saving and restoring the flat db 

   these are non human readable formatted files because i am lazy 
   note: architecture float format and endianess dependence in user parameters 
*/

static char nlib_flatdb_ver[]="NLIB_FLATDB_a";

int nlib_flatdb_write(nlib_t *nlib, void * file)
{
  nlibflatdb_t *p;
  int i,j,q;
  unsigned char *c;
  fprintf(file,"%s %i\n",nlib_flatdb_ver,list_qty(&(nlib->flatdb)));
  list_iterate(&(nlib->flatdb),i,p)
    {
      q=list_sizeof(&(p->uservals))*list_qty(&(p->uservals));
      c=list_data(&(p->uservals),0);
      fprintf(file,"lib %i db %i qvals %i user %i %i\n",p->lib_index,p->db_index,p->qvals,
		  list_sizeof(&(p->uservals)),list_qty(&(p->uservals))
		  );
      for(j=0;j<p->qvals;j++)
	fprintf(file,"%e%c",p->pvals[j],(j==(p->qvals-1))?'\n':' ');
      for(j=0;j<q;j++)
	{
	  fprintf(file,"%i%c",(int)c[j],(j==(q-1))?'\n':' ');
	  if(0)fprintf(stderr,"%i ",c[j]);
	}
    }
  fprintf(file,"NLIB_FLATDB_END\n");
  return i;
}


int nlib_flatdb_read(nlib_t *nlib, void * file_fp)
{
  FILE *file=file_fp;
  nlib_process_t up;
  nlibflatdb_t *p;
  int i,j,q,qty,p_qvals,ua,ub,index;
  unsigned char *c;
  char line[4096],*lp;
  line[0]=0;
  line[sizeof(line)-1]=0;
  fgets(line,sizeof(line)-2,file);
  if(memcmp(line,nlib_flatdb_ver,strlen(nlib_flatdb_ver))!=0)return -1;
  qty=atoi(line+sizeof(nlib_flatdb_ver));
  if(qty==0)return -2;
  list_hint(&(nlib->flatdb),qty);
  i=0;
  while(!feof(file))
    {
      if(i==qty)break;
      line[0]=0;
      fgets(line,sizeof(line)-2,file);
      memset(&up,0,sizeof(up));
      up.nlib=nlib;
      up.lib_index=-1;
      up.db_index=-1;
      p_qvals=-1;
      ua=0;
      ub=0;
      if(sscanf(line,"lib %i db %i qvals %i user %i %i",&up.lib_index,&up.db_index,&p_qvals,&ua,&ub)!=5)
	{ assert(0); continue; }
      index=list_qty(&(nlib->flatdb));
      c=nlib_flat(&up,ua,ub);
      p=list_data(&(nlib->flatdb),index);
      assert(p!=NULL);
      assert(p_qvals==p->qvals);
      line[0]=0;
      fgets(line,sizeof(line)-2,file);
      for(lp=line,j=0;j<p->qvals;j++)
	{
	  float f=0.0;
	  while(isspace(lp[0]))lp++;
	  if(sscanf(lp,"%f",&f)!=1)assert(0);
	  while(!isspace(lp[0]))lp++;
	  p->pvals[j]=f;
	}
      assert(c!=NULL);
      q=ua*ub;
      line[0]=0;
      fgets(line,sizeof(line)-2,file);

      for(lp=line,j=0;j<q;j++)
	{
	  unsigned x=0;
	  while(isspace(lp[0]))lp++;
	  if(sscanf(lp,"%i",&x)!=1)assert(0);
	  while(!isspace(lp[0]))lp++;
	  c[j]=x;
	  if(0)fprintf(stderr,"%i ",c[j]);
	}
      i++;
    }
  return i;
}

/* parse a library file format 

.netlist = name
.reference=ref_name name=spice_name delay=time rseries=res cshunt=cap
.reference= ...
.input=encoding spice_name[0] .... spice_name_[qnodes-1]
.input=encoding ...
.output spice_nam.....
.output ...
.param=spice_name  .min=xxx .max=xxx .qtysteps=xxx
.param ..
.function=fname
.end_netlist

.encoding=encodingname .wires=number .tokens=number .digitize=methodname

.digital=functionname 
 0=0 1=0 0011=.. ... input_token = output_token
.end_digital 


.spice

# spice netlist format

.end_section  # end of spice netlist 

*/


static tokenmap_t nlib_tokens[]=NLIB_TOKENS;

/* assume nlib aready initted, this adds to it */
int nlib_parse(nlib_t *nlib, scanner_t *scan)
{
  netlist_t *nl=NULL;
  int db_index=-1;
  deck_t *db_deck=NULL, *digital_deck=NULL;
  nlibiodef_t encoding_default;
  nlibfunc_t *digital_func=NULL;
  deck_t *dplast=NULL;
  scanner_def_t defs;
  
  assert(nlib->eqn_mem!=NULL);
  eqn_mem_push(nlib->eqn_mem);

  defs=*scanner_def_spice();
  strcpy(defs.line_stop,".end_nlib");
  encoding_default.encoding="dual_rail";
  encoding_default.wires=2;
  encoding_default.tokens=2;
  encoding_default.digitize=0;
  scanner_sect_new(scan,&defs,nlib_tokens);
  while(scanner_parse_line(scan))
    {
      card_t *cp=NULL;
      int flag=0;

      if(scan->sectp!=NULL)
	if(scan->sectp->dp!=NULL)
	  if((cp=scan->sectp->dp->card)!=NULL)
	    flag++;
      if(flag&&(scan->sectp->dp!=dplast))
	{
	  dplast=scan->sectp->dp;
	  scan->errdeck=dplast;
	  scan->errcard=dplast->card;
	  if(0)scanner_debug_all(scan,stderr);
	  switch(cp->token)
	    {
	     
	    case Knlib_digital:
	      if(cp->val!=NULL)
		{
		  card_t *p;
		  int z;
		  int qtyin=-1, qtyout=-1, p_delay=-1, p_offset=-1, p_cycle=-1;
		  for(p=cp->next;p!=NULL;p=p->next)
		    {
		      if(p->val!=NULL)
			{
			  z=atoi(p->val);
			  switch(p->token)
			    {
			    case  Knlib_qtyin:
			      qtyin=z;
			      break;
			    case Knlib_qtyout:
			      qtyout=z;
			      break;
			    case Knlib_p_delay:
			      p_delay=z;
			      break;
			    case Knlib_p_offset:
			      p_offset=z;
			      break;
			    case Knlib_p_cycle:
			      p_cycle=z;
			      break;
			    default: 
			      scan->errcard=p;
			      scan->errdeck=dplast;
			      parse_warn(scan,"unknown word in function definition %s\n",p->val);
			    }
			}
		    }
		  
		  if((qtyin!=-1) && (qtyout!=-1) && (p_delay!=-1) && (p_offset!=-1) && (p_cycle!=-1))
		    {
		      digital_deck=scan->sectp->dp;
		      digital_func=nlib_new_func(nlib,cp->val, qtyin, qtyout, p_delay,p_offset,p_cycle);
		      break;
		    }
		}
	      parse_error(scan,"mangled function definition");
	      break;

	    case Knlib_end_digital:
	      if((digital_deck!=NULL) && (digital_func!=NULL))
		{
		  deck_t *dp;
		  for(dp=digital_deck->next;dp!=NULL;dp=dp->next)
		    {
		      card_t *p;
		      if(dp==scan->sectp->dp)break;
		      for(p=dp->card;p!=NULL;p=p->next)
			{
			  unsigned int a=0,b=0;
			  char *sp=p->str;
			  scan->errcard=p;
			  scan->errdeck=dp;
			  if(sp!=NULL)
			    {
			      a=parse_binary(&sp);
			      sp=p->val;
			      if(sp!=NULL)
				{
				  b=parse_binary(&sp);
				  nlib_func_addmap(nlib, digital_func, a, b);
				}
			      else parse_warn(scan,"ignoring mangled mapping %s=%s\n",cp->str,cp->val);
			    }
			  else parse_warn(scan,"ignoring mangled mapping %s=%s\n",cp->str,cp->val);
			}
		    }
		}
	      else
		parse_error(scan,"mangled function definition");
	      digital_deck=NULL;
	      digital_func=NULL;
	      break;
	      
	    case Knlib_encoding: /* io def */
	      if(cp->val!=NULL)
		{
		  int tokens=0, wires=0, h_tokens=0, h_wires=0;
		  char *digitize=NULL;
		  card_t *p;
		  for(p=cp->next;p!=NULL;p=p->next)
		    {
		      D(20,fprintf(stderr,"encoding: %s=[%s]\n",p->str,p->val));
		      if(p->val!=NULL)
			{
			  switch(p->token)
			    {
			    case Knlib_wires:
			      wires=atoi(p->val);
			      h_wires++;
			      break;
			    case Knlib_tokens:
			      tokens=atoi(p->val);
			      h_tokens++;
			      break;
			    case Knlib_digitize:
			      digitize=p->val;
			      break;
			    }
			}
		    }
		  if( h_tokens && h_wires && (digitize!=NULL) )
		    {
		      int index;
		      nlibiodef_t *iop;
		      index=list_add(&(nlib->iodefs),&encoding_default);			      
		      iop=list_data(&(nlib->iodefs),index);
		      iop->encoding=strdup(cp->val);
		      
		      if(strcmp(digitize,"deltav")==0)
			iop->digitize=1;
		      else if(strcmp(digitize,"cmos")==0)
			iop->digitize=2;
		      else
			iop->digitize=0; 
		      
		      iop->tokens=tokens;
		      iop->wires=wires;
		      D(20,fprintf(stderr,"adding encoding %s, %i %i\n",cp->val,tokens,wires));
		      break;
		    }
		}
	      
	      parse_error(scan,"mangled encoding line ");
	      break;
	      
	    case Knlib_define:
	      if(cp->val==NULL)
		{
		  card_t *p;
		  for(p=cp->next;p!=NULL;p=p->next)
		    {
		      if(p->val!=NULL)
			{
			  eqnl_add(&(nlib->eqnl),eqn_parse(p->val),p->str);
			}		  
		    }
		}
	      break;

	    case Knlib_netlist:
		if(cp->val!=NULL)
		  {
		    db_index=nlib_new_db(nlib,cp->val);
		    db_deck=scan->sectp->dp; /* store location in deck where netlist occured */
		  }
		else
		  parse_error(scan,".netlist=name expected");
		break;
		
	    case Knlib_spice:
	      if(db_index>=0)
		{
		  nl=netlist_devnew(netlist_parse_input(scan,"spice",NULL));
		  assert(nl!=NULL);
		  
		  if(1)netlist_release(nl); /* release spice parse structures */
		  /* why does it crash with this ???? */
		}
	      else 
		parse_error(scan,".spice section must be preceded by .netlist section");  
	      break;
	      
	    case Knlib_end_netlist:
	      if(db_index<0)
		parse_error(scan,".end netlist without .netlist");
	      else
		{
		  deck_t *dp;
		  card_t *cp;
		  nlibdb_t *dbp;		      
		  nlib_db_netlist(nlib,db_index,nl);
		  dbp=list_data(&(nlib->db),db_index);
		  nl=NULL;
		  for(dp=db_deck;dp!=NULL;dp=dp->next) /* walk through from db_deck to here */
		    {
		      if(dp->card==NULL)continue;
		      cp=dp->card;
		      scan->errdeck=dp;
		      scan->errcard=cp;

		      switch(cp->token)
			{
			  case Knlib_param:
			    if(cp->val!=NULL)
			      {
				card_t *p;
				float min=0,max=0,qty=0,v;
				int h_min=0, h_max=0, h_qty=0;
				
				for(p=cp->next;p!=NULL;p=p->next)
				  {
				    if(p->val!=NULL)
				      {
					v=parse_float(p->val);
					switch(p->token)
					  {
					  case Knlib_min:
					    min=v; h_min++;
					    break;
					  case Knlib_max:
					    max=v; h_max++;
					    break;
					  case Knlib_qty:
					    qty=v; h_qty++;
					      break;
					  }
				      }
				  }
				if(h_min&&h_max&&h_qty)
				  {
				    nlib_add_pm(nlib,db_index, cp->val, min, max, floorf(qty));
				    break;
				    }
			      }
			    scan->errdeck=dp;
			    scan->errcard=dp->card;
			    parse_error(scan,"mangled param statement\n");
			    break;
			    
			  case Knlib_input:
			  case Knlib_output:
			    if(cp->val!=NULL)
			      {
				card_t *p;
				char *names[64]; /* max limit */
				int i,qnames=0;
				nlibiodef_t *iop;
				int tokens=0, wires=0, digital=0, h_stuff=0;
				  
				for(i=0,p=cp->next;(p!=NULL)&&(i<64);p=p->next,i++)
				  names[i]=p->str;
				
				qnames=i;
				list_iterate(&(nlib->iodefs),i,iop)
				  {
				    if(strcmp(cp->val,iop->encoding)==0)
				      {
					tokens=iop->tokens;
					wires=iop->wires;
					digital=iop->digitize;
					h_stuff=1;
					if(0)fprintf(stderr,"got stuff %s %i %i \n",cp->val,wires,qnames);
				      }
				  }
				
				if(h_stuff && (wires==qnames)&& (dbp!=NULL) )
				  {
				    
				    if(cp->token==Knlib_input)
				      nlib_add_io(nlib,db_index,&(dbp->in),tokens,wires,digital,names);
				    if(cp->token==Knlib_output)
				      nlib_add_io(nlib,db_index,&(dbp->out),tokens,wires,digital,names);
				    break;
				  }
			      }
			    scan->errdeck=dp;
			    scan->errcard=dp->card;
			    parse_error(scan,"mangled input/output statement \n");
			    break;
			  case Knlib_function:
			    if(cp->val!=NULL)
			      {
				nlib_db_func(nlib,db_index,cp->val);
				break;
				}
			    parse_error(scan,"mangled function line \n");
			    break;

			  case Knlib_reference:
			    if(cp->val!=NULL)
			      {
				card_t *p;
				char *str_name=NULL;
				eqn_t cshunt,rseries,delay;
				int h_cshunt=0, h_rseries=0, h_delay=0;
				int nl_index=-1, top_index=-1;

				for(p=cp->next;p!=NULL;p=p->next)
				  {
				    if(p->val!=NULL)
				      switch(p->token)
					{
					case Knlib_name:
					  str_name=p->val;
					  break;
					case Knlib_cshunt:
					  cshunt=eqn_parse(p->val);
					  h_cshunt++;
					  break;
					case Knlib_rseries:
					  rseries=eqn_parse(p->val);
					  h_rseries++;
					  break;
					case Knlib_delay:
					  delay=eqn_parse(p->val);
					  h_delay++;
					  break;
					default: 
					  scan->errdeck=dp;
					  scan->errcard=p;
					  parse_warn(scan,"reference: unknown word %s\n",cp->str);
					}
				  }
				if(str_name!=NULL)
				  nl_index=netlist_findnode(dbp->nl,str_name);
				
				if(nl_index>=0)
				  {
				    nlibrefnode_t rn;
				    int l;

				    top_index=names_check(nlib->vref,cp->val);
				    if(top_index<0)
				      {
					top_index=nlib->qvref;
					names_add(nlib->vref,top_index,cp->val);
					nlib->qvref++;
				      }

				    for(l=0;l<MAXSKEW_PARAMS;l++)
				      rn.skew[l]=eqn_empty();
				    
				    rn.vrefi=top_index;
				    rn.devnodei=nl_index;
				    if(h_delay)
				      rn.skew[RTdelay]=delay;
				    if(h_rseries)
				      rn.skew[RTrseries]=rseries;
				    if(h_cshunt)
				      rn.skew[RTcshunt]=cshunt;
				    
				    list_add(&(dbp->refnodes),&rn);
				    break;
				  }
				else { parse_warn(scan,"reference netlist node %s not found, ignoring\n",str_name); break; }
			      }
			    parse_error(scan,"mangled reference line cp=%s val=%s\n",cp->str,cp->val);
			    break;
			  case Knlib_netlist:
			    break;
			  case Knlib_spice:
			    break;
			  case Knlib_end_netlist:
			    break;
			  default:
			    scan->errdeck=dp;
			    scan->errcard=cp;
			    parse_warn(scan,"ignoring unknown card %s\n",cp->str);
			    break;
			  }
		      }
		    if(dbp!=NULL)
		      nlib_db_fixup(nlib,dbp);
		    else assert(0);
		    dbp=NULL;
		    db_index=-1;
		    break;
		  }
	      }
	}
      

    }
  /* check memory after parse */  free(malloc(32));
  nlib_fixup(nlib);
  if(eqn_mem_pop()!=nlib->eqn_mem)assert(0);
  return list_qty(&(nlib->db));
}
