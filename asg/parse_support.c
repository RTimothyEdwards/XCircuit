/************************************************************
**
**       COPYRIGHT (C) 2004 GANNON UNIVERSITY
**                  ALL RIGHTS RESERVED
**
**        This software is distributed on an as-is basis
**        with no warranty implied or intended.  No author
**        or distributor takes responsibility to anyone 
**        regarding its use of or suitability.
**
**        The software may be distributed and modified 
**        freely for academic and other non-commercial
**        use but may NOT be utilized or included in whole
**        or part within any commercial product.
**
**        This copyright notice must remain on all copies 
**        and modified versions of this software.
**
************************************************************/


/* file parse_support.c */
/* ------------------------------------------------------------------------
 * Routines for integrating ASG into a language parser, such as the 
 * hpice parser in XCircuit 3.2.7+. 
 * ------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "network.h"

/*---------------------------------------------------------------
 * Global Variable Definitions
 *---------------------------------------------------------------
 */

module *curobj_module;

/***********************************************************************************
 Add2TermModule - This method is used to add a new module found in the parse tree of a language. E.g., this can be used in HSPICE
             parser to add a module. 
             

**************************************************************************************/

void Add2TermModule( name, type, inNode, outNode )
char* name;		// Parser-supplied module name (e.g., CAP32)
char* type;		// Parser-supplied identification type (e.g., CAPACITOR)
char* inNode;		// Net name for input net
char* outNode;		// Net name for output net
{
   net* pNet;

   //Create a new Module( Capacitor or Resistor or Inductor or Buffer, Inverter gate etc)
   curobj_module = newobject(name, type);  

   //Add Inputports
   pNet = get_net(inNode);
   
   //If Input Node does not exist in the global database
   if( pNet == NULL )
   {
      newnode(inNode);
   }
   addin(inNode, 0, 1);

   //Add output ports
   pNet = get_net(outNode);
   
   //If output Node does not exist in the global database
   if( pNet == NULL )
   {
      newnode(outNode);
   }
   addout(outNode, 0, 1);
}

/***********************************************************************************
 Add3TermModule - This method is used to add a new module. This can be used in HSPICE
             parser to add a module. 
             

**************************************************************************************/

void Add3TermModule( name, type, inNode, outNode, inoutNode)
char* name;		// Parser-supplied module name (e.g., CAP32)
char* type;		// Parser-supplied identification type (e.g., CAPACITOR)
char* inNode;		// Net name for input (left) net
char* outNode;		// Net name for output (right) net
char* inoutNode;	// Net name for inout (top) net
{
   net* pNet;

   //Create a new Module( Capacitor or Resistor or Inductor or Buffer, Inverter gate etc)
   curobj_module = newobject(name, type);  

   //Add Inputports
   pNet = get_net(inNode);
   
   //If Input Node does not exist in the global database
   if( pNet == NULL )
   {
      newnode(inNode);
   }
   addin(inNode, 0, 1);

   //Add output ports
   pNet = get_net(outNode);
   
   //If output Node does not exist in the global database
   if( pNet == NULL )
   {
      newnode(outNode);
   }
   addout(outNode, 0, 1);

   //Add inout ports
   pNet = get_net(inoutNode);
   
   //If inout Node does not exist in the global database
   if( pNet == NULL )
   {
      newnode(inoutNode);
   }
   addinout(inoutNode, 0, 1);
}

/*----------------------------------------------------------------------*/
/* AddNTermModule ---							*/
/*	Add a module of N terminals to the database			*/
/*									*/
/* name	= Parser-supplied module name (e.g., CAP32)			*/
/* type	= Parser-supplied identification type (e.g., CAPACITOR)		*/
/* N    = Number of nodes, followed by list of node names.		*/
/* followed by pairs of char * types declaring the name of the pin	*/
/* (expected in the xcircuit object) followed by the name of the net.	*/
/*----------------------------------------------------------------------*/

void AddNTermModule(char *name, char *type, int N, ...)
{
   va_list args;
   int i;
   net *pNet;
   char *pinName;
   char *netName;
   objectptr pobj;

   // Create a new Module (Capacitor, Resistor, Inductor or Buffer, Inverter gate etc)
   curobj_module = newobject(name, type);  

   // Get the corresponding XCircuit object and get the object dimensions
   pobj = NameToObject(type, NULL, FALSE);
   if (pobj != NULL) {
      curobj_module->x_size = pobj->bbox.width;
      curobj_module->y_size = pobj->bbox.height;
   }
   else {
      error("AddNTermModule: No such object", type);
   }

   va_start(args, N);

   for (i = 0; i < N; i++) {
      // Add port
      pinName = va_arg(args, char *);
      netName = va_arg(args, char *);
      pNet = get_net(netName);
   
      // If Input Node does not exist in the global database
      if (pNet == NULL) {
	 newnode(netName);
      }
      add_xc_term(type, pinName, netName, i, N);
   }
   va_end(args);
}

/*--------------------------------------------------------------*/
/* Handle modules with an abitrary number of terminals in the	*/
/* following way:  AddNTermModule() is called with N = 0 to	*/
/* create curobj_module (which is global).  Then, for each pin,	*/
/* the routine AddModuleTerm() is called to associate a net	*/
/* with each pin, in turn.					*/
/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* For the moment, pins will be named "1", "2", etc.  To do:	*/
/* look up the object having a name matching "type".  Confirm	*/
/* a match in the number of pins N, and then pick up the pin	*/
/* names from the object itself.  Presumably this is done by	*/
/* assuming that the object has an info label "spice:..." and	*/
/* using that.  It is the responsibility of the user to ensure	*/
/* that the library used to represent primitives in the spice	*/
/* deck contains the correct number and order of pins.		*/
/*--------------------------------------------------------------*/

void AddModuleTerm(char *type, char *netName, int portnum, int allports)
{
   net *pNet;

   pNet = get_net(netName);
   
   // If Input Node does not exist in the global database
   if (pNet == NULL) {
	 newnode(netName);
   }
   add_xc_term(type, NULL, netName, portnum, allports);
}

/*---------------------------------------------------------------
 * END OF FILE
 *---------------------------------------------------------------
 */
