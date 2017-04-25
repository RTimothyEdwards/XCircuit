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
/* netlist_dev.c, netlist functions for netlists of circuital devices (ie fets)
   Conrad Ziesler
*/


/* our goal here is to provide all the netlist convenience functions 
   which are dependent on the particular definitions for the devices (ie hard coded device types)
   by device netlists we refer to netlists to be fed to spice-like simulators.

   this was split off the core device-independent netlist routines
*/

#include <stdio.h>
#include "debug.h"

#include "netlist_dev.h"


/* make a new device netlist, using client sizes as given 
   ni is ptr returned by netlist_parse_input
*/


netlist_t *netlist_devnew_inplace(netlist_input_t ni, int extrasize, int extrasizes[DEVT_MAX])
{
  int i,r;
  int terms[]=DEVT_TERMS;
  int vals[]=DEVT_VALS;
  char *syms[]=DEVT_SYMS;
  int others[]=DEVT_OTHERS;
  netlist_t *nl;

  nl=malloc(sizeof(netlist_t)+extrasize);
  assert(nl!=NULL);
  netlist_init(nl);

  for(i=0;i<DEVT_MAX;i++)
    {
      r=netlist_register_entity(nl,i,terms[i],vals[i],others[i],syms[i],extrasizes[i]);
      assert(r==i);
    }
  
  nl->input=ni;
  if(ni.p!=NULL)
    {
      switch(ni.generic->magic)
	{
	case SPICE_MAGIC:
	  spice_count(nl);
	  
	  for(i=0;i<DEVT_MAX;i++)
	    if(nl->e[i].qcount>0)
	      list_hint(&(nl->e[i].l),nl->e[i].qcount);

	  spice_build(nl);
	  break;
	case EXTRACT_MAGIC:
	  /*
	    extract_count(nl);  
	    for(i=0;i<DEVT_MAX;i++)
	    if(nl->e[i].qcount>0)
	    list_hint(&(nl->e[i].l),nl->e[i].qcount);
	    extract_build(nl);
	  */
	  break;
	}
    }
  return nl;
}

netlist_t *netlist_devnew(netlist_input_t ni)
{
  int sz[DEVT_MAX];
  memset(sz,0,sizeof(sz));
  return netlist_devnew_inplace(ni, 0, sz);
}


/**** output spice_like file  ****/

void netlist_o_spice(void  *of_fp, netlist_t *nl, int n_start)
{
  FILE *of=of_fp;
  int i,j,k;
  int m_i=1, c_i=1, l_i=1, i_i=1, v_i=1;
  
  devall_p p;
  entity_t *e;
  int nodes[32];
  float vals[32];
  eqn_t *v;

  if(nl==NULL) return;  
  eqnl_evaldep(&(nl->eqnl));
  
  fprintf(of,"*** netlist output \n");
  for(i=0;i<DEVT_MAX;i++) 
    {
      e=nl->e+i;
      assert(e->qterms<(sizeof(nodes)/sizeof(int)));
      assert(e->qvals<(sizeof(vals)/sizeof(float)));

      list_iterate(&(e->l),j,p.p)
	{
	  for(k=0;k<e->qterms;k++)
	    nodes[k]=n_start+netlist_node_termptr(nl,p.genp->n[k]);
	  
	  v=NETLIST_VALS(nl,i,j);
	  for(k=0;k<e->qvals;k++)
	    vals[k]=eqnl_eval(&(nl->eqnl),v[k]);
	  	  
	  switch(e->id)
	    {
	    case DEVT_NODE:  /* output a mini node-name directory */	
	      fprintf(of,"* %i %s\n",j+n_start,names_lookup(nl->names,j));
	      break;
	      
	    case DEVT_FET:
	      fprintf(of,"m%i %i %i %i %i %s L=%g W=%g AS=%g AD=%g PS=%g PD=%g\n",m_i++,
			  nodes[DEVFET_S],nodes[DEVFET_G],nodes[DEVFET_D], nodes[DEVFET_B],
			  (p.fetp->type==DEVFET_nmos)?"n":"p",
			  vals[DEVFET_l],vals[DEVFET_w], vals[DEVFET_as],
			  vals[DEVFET_ad], vals[DEVFET_ps], vals[DEVFET_pd]
			  );
	      break;
	    case DEVT_CAP:
	      fprintf(of,"c%i %i %i %g\n",c_i++,
			  nodes[DEV2TERM_P], nodes[DEV2TERM_N], vals[DEVCAP_c]
			  );
	      break;
	    case DEVT_RES:
	      fprintf(of,"c%i %i %i %g\n",c_i++,
			  nodes[DEV2TERM_P], nodes[DEV2TERM_N], vals[DEVRES_r]
			  );
	      break;
	    case DEVT_IND:
	      fprintf(of,"l%i %i %i %g\n",l_i++,
			  nodes[DEV2TERM_P], nodes[DEV2TERM_N], vals[0]
			  );
	      break;
	    case DEVT_VSRC:
	      fprintf(of,"v%i %i %i %g\n",v_i++,
			  nodes[DEV2TERM_P], nodes[DEV2TERM_N], vals[0]
			  );
	      break;
	    case DEVT_ISRC:
	      fprintf(of,"i%i %i %i %g\n",i_i++,
			  nodes[DEV2TERM_P], nodes[DEV2TERM_N], vals[0]
			  );
	      break;

	    default:
	      break;
	    }
	}
    }
  fprintf(of,"*** end netlist output \n");  
}



netlist_t *netlist_copyish(netlist_t *in, int size, int sizes[])
{
  int i;
  int extras[TERMPTR_MAX_DEVT];
  int defsize[DEVT_MAX]=NETLIST_DEFSIZES;
  for(i=0;i<TERMPTR_MAX_DEVT;i++)
    {
      if(i<DEVT_MAX)
	extras[i]=sizes[i]-defsize[i];
      else extras[i]=0;

      if(extras[i]<0)extras[i]=0;
      assert(extras[i]<2048);
    }

  return netlist_copyisher(in,size-sizeof(netlist_t),extras);
}
