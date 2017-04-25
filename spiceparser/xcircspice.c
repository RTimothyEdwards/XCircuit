/* xcircspice.c --- implements routine ReadSpice() for xcircuit
 * SPICE import.
 * Conrad Ziesler, Tim Edwards
 */

#include <stdio.h>
#include <ctype.h>

#include "debug.h"
#include "scanner.h"
#include "netlist_spice.h"

#define fprintf tcl_printf

/*--------------------------------------------------------------*/
/* Get the node name from a node structure			*/
/*--------------------------------------------------------------*/

char *node(node_t n)
{
  char *p;
  p=n->str;
  return p;
}

/*--------------------------------------------------------------*/
/* Make a pass through the internal representation of the	*/
/* SPICE input file and translate to the internal		*/
/* representation of the SPAR ASG code.				*/
/*--------------------------------------------------------------*/

void generate_asg(spice_t *spice)
{
   subckt_t *ckt = NULL, **cktpp;
   list_t subckts;
   int i, j;
   int uniq = 1;

   /* Iterate over all subcircuits */
   subckts = spice_list_subckt(spice->ckt);
   list_iterate(&subckts, i, cktpp) {
      ckt = *cktpp;

do_ckt:
      assert(ckt != NULL);
      fprintf(stdout, "spiceparser: cell(%i) is \"%s\"\n", i, ckt->name);

      for (j = 0; j < ckt->ndefn; j++) {
	 char *nn;
	 nn = ckt->defn[j]->str;
	 fprintf(stdout, "pin: %s\n",nn);
      }

      /* Loop through circuit MOSFET devices */
      /* (to be done---use model information to determine if this is a	*/
      /* pMOS or nMOS device).						*/

      for (j = 0; j < ckt->nm; j++) {

	 /* For now, ignoring substrate node ckt->m[j].nodes[3] 	*/
	 /* We should have two devices;  if substrate is VDD or GND,	*/
	 /* then use the 3-terminal device; otherwise, use the 4-	*/
	 /* terminal device.						*/

	 AddNTermModule(ckt->m[j].deck->card->str, "MSFET", 4,
		"D", node(ckt->m[j].nodes[0]),
		"G", node(ckt->m[j].nodes[1]),
		"S", node(ckt->m[j].nodes[2]),
		"B", node(ckt->m[j].nodes[3]));
      }

      /* Loop through circuit capacitor devices */
      for(j = 0; j < ckt->nc; j++) {
	 AddNTermModule(ckt->c[j].deck->card->str, "CAPC", 2,
		"1", node(ckt->c[j].nodes[0]),
		"2", node(ckt->c[j].nodes[1]));
      }

      /* Loop through circuit resistor devices */
      for (j = 0; j < ckt->nr; j++)
      {
	 AddNTermModule(ckt->r[j].deck->card->str, "RESTR", 2,
		"1", node(ckt->r[j].nodes[0]),
		"2", node(ckt->r[j].nodes[1]));
      }

      /* Loop through circuit inductor devices */
      for (j = 0; j < ckt->nl; j++)
      {
	 AddNTermModule(ckt->l[j].deck->card->str, "INDR", 2,
		"1", node(ckt->l[j].nodes[0]),
		"2", node(ckt->l[j].nodes[1]));
      }

      /* Loop through circuit voltage sources */
      for (j = 0; j < ckt->nv; j++)
      {
	 AddNTermModule(ckt->v[j].deck->card->str, "VAMP", 2,
		"1", node(ckt->v[j].nodes[0]),
		"2", node(ckt->v[j].nodes[1]));
      }

      /* Loop through circuit current sources */
      for (j = 0; j < ckt->ni; j++)
      {
	 AddNTermModule(ckt->i[j].deck->card->str, "IAMP", 2,
		"1", node(ckt->i[j].nodes[0]),
		"2", node(ckt->i[j].nodes[1]));
      }

      /* Loop through circuit subcircuit calls, where those	*/
      /* calls are not defined within the file and are assumed	*/
      /* to be primitives (this may require more checking---may	*/
      /* need to remove the x[] records when they are expanded	*/

      for (j = 0; j < ckt->nx; j++)
      {
	 AddNTermModule(ckt->x[j].deck->card->str,
		ckt->x[j].rest->str, 0);

	 for (i = 0; i < ckt->x[j].nn; i++)
	     AddModuleTerm(ckt->x[j].rest->str, node(ckt->x[j].nodes[i]), i,
			ckt->x[j].nn);
      }

   }

   /* kludge for getting the top-level circuit */
   if (ckt == spice->ckt) return;
   else {
      ckt = spice->ckt;
      goto do_ckt;
   }
}
  
/*--------------------------------------------------------------*/
/* Top-level call used in xcircuit (files.c) to read in a SPICE */
/* file.							*/
/*--------------------------------------------------------------*/

int ReadSpice(FILE *fp)
{
   scanner_t scan;
   spice_t *spice;

   scanner_init(&scan);
   scanner_input_newfp(&scan, fp);
   spice = spice_new(&scan);
   generate_asg(spice);
   spice_release(spice);
   return 0;
}

