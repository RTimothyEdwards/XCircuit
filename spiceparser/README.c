/* subckt_translator.c : quick hack to enable subckt files to work in standard "magic" characterization flow
   Conrad Ziesler
*/


#include <stdio.h>
#include <ctype.h>

#include "makeutils.lib/debug.h"
#include "parsers.lib/scanner.h"
#include "netlist.lib/netlist_spice.h"



      /*
      cell: c2 INVX1
area: 10.368
input: i_a
out: o_y
vss: i_vss
vdd: i_vdd
      */


int is_power(char *name)
{ 
  if(strcasecmp(name,"vdd")==0) return 1;
  return 0;
}

int is_ground(char *name)
{
  if(strcasecmp(name,"gnd")==0) return 1;
  if(strcasecmp(name,"vss")==0) return 1;
  return 0;
}

int is_input(char *name)
{
  if(!is_ground(name))
    if(!is_power(name))
      if(tolower(name[0])<='f') return 1;
  return 0;
}

int is_output(char *name)
{
  char x;
  x=tolower(name[0]);
  if(!is_ground(name))
    if(!is_power(name))
      if(x>='q') return 1;
  return 0;
}

char *node(node_t n)
{
  char *p;
  p=n->str;
  return p;
}

void dumprest(FILE *fp, card_t *cp)
{
  for(;cp!=NULL;cp=cp->next)
    { /* just dump the rest verbatim */
      fprintf(fp," %s",cp->str);
      if(cp->val!=NULL) fprintf(fp,"=%s",cp->val);
    }
}

void generate_stuff(spice_t *spice, FILE *fp_pins, FILE *fp_flat)
{
  subckt_t *ckt,**cktpp;
  list_t subckts;
  int i,j;
  int uniq=1;


  subckts=spice_list_subckt(spice->ckt);
  list_iterate(&subckts,i,cktpp)
    {
      ckt=*cktpp;
      assert(ckt!=NULL);
      fprintf(fp_pins,"\n\n");
      fprintf(fp_pins,"cell: c%i %s\n",i,ckt->name);
      fprintf(fp_pins,"area: %i\n",ckt->nm); /* estimate area by transitor count */
      for(j=0;j<ckt->ndefn;j++)
	{
	  char *nn;
	  nn=ckt->defn[j]->str;
	  if(is_input(nn))
	    {
	      fprintf(fp_pins,"input: %s\n",nn);
	    }
	  else if(is_output(nn))
	    {
	      fprintf(fp_pins,"out: %s\n",nn);
	    }
	  else if(is_power(nn))
	    {
	      fprintf(fp_pins,"vdd: %s\n", nn);
	    }
	  else if(is_ground(nn))
	    {
	      fprintf(fp_pins,"vss: %s\n", nn);
	    }	
	  else fprintf(fp_pins,"#error pin %s\n",nn);
	}

      if(fp_flat!=NULL)
	{
	  fprintf(fp_flat,"\n\n*\n*\n* subckt %s \n\n",ckt->name);
	  for(j=0;j<ckt->nm;j++)
	    {
	      fprintf(fp_flat,"M%i c%i_%s c%i_%s c%i_%s c%i_%s ",
		      uniq++,i,node(ckt->m[j].nodes[0]),i,node(ckt->m[j].nodes[1]),
		      i,node(ckt->m[j].nodes[2]),i,node(ckt->m[j].nodes[3]));
	      
	      dumprest(fp_flat,ckt->m[j].rest);
	      fprintf(fp_flat,"\n");
	      /* fprintf(fp_flat," l=%g w=%lf pd=%lf ps=%lf ad=%lf as=%lf \n", */
	    }
	  for(j=0;j<ckt->nc;j++)
	    {
	      fprintf(fp_flat,"C%i c%i_%s c%i_%s ",
		      uniq++,i,node(ckt->c[j].nodes[0]),i,node(ckt->c[j].nodes[1]));
	      dumprest(fp_flat,ckt->c[j].rest);
	      fprintf(fp_flat,"\n");
	    }
      
	  for(j=0;j<ckt->nr;j++)
	    {
	      fprintf(fp_flat,"R%i c%i_%s c%i_%s ",
		      uniq++,i,node(ckt->r[j].nodes[0]),i,node(ckt->r[j].nodes[1]));
	      dumprest(fp_flat,ckt->r[j].rest);
	      fprintf(fp_flat,"\n");
	    }
	  
	}
    }
}
  



int main(int argc, char **argv)
{
  char *subckt_filename=NULL;
  char *pins_output=NULL;
  char *flat_spice=NULL;
  int i;
  scanner_t scan;
  FILE *fp,*fp_pins,*fp_flat;
  spice_t *spice;

  for(i=1;i<argc;i++)
    {
      char *cmd=argv[i];
      char *arg=(i<argc)?(argv[i+1]):NULL;
      if(cmd[0]=='-')
	switch(cmd[1])
	  {
	  case 'i':
	    if(arg!=NULL)
	      { subckt_filename=arg; i++; continue; }
	    break;
	  case 'p':
	    if(arg!=NULL)
	      { pins_output=arg; i++; continue; }
	    break;
	  case 'o':
	    if(arg!=NULL)
	      { flat_spice=arg; i++; continue; }
	    break;
	  }
      fprintf(stderr,"Invalid command line option %s %s, skipping\n",cmd,arg);
    }
  if((subckt_filename==NULL))
    {
      fprintf(stderr,"Usage: translator -i input_subckt_file"
			" -o output_flat_file -p output_pins_file\n");
      exit(1);
    }
 
  
  scanner_init(&scan);
  fp=fopen(subckt_filename,"rt");
  scanner_input_newfp(&scan,fp);
  spice=spice_new(&scan);

  if(pins_output!=NULL)
    fp_pins=fopen(pins_output,"w");
  else fp_pins=stdout;
  
  if(flat_spice!=NULL)
    fp_flat=fopen(flat_spice,"w");
  else fp_flat=NULL;


  generate_stuff(spice,fp_pins,fp_flat);
  if(fp_pins!=NULL)fclose(fp_pins);
  if(fp_flat!=NULL)fclose(fp_flat);
    
  return 0;
}

