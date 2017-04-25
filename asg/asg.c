/*----------------------------------------------------------------------*/
/* asg.c --- Glue code between xcircuit and SPAR			*/
/*----------------------------------------------------------------------*/

#ifdef ASG

#include <stdio.h>
#include "externs.h"	 	/* Local ASG definitions */

extern void spar_destructor();
extern void spar_constructor();

/*--------------------------------------------------------------------- */

int xc_print_sym(XCWindowData *areastruct, char *sym, char*name,
    int x, int y, int x_dis, int y_dis, int rot)
{
    asg_make_instance(areastruct, sym, x, y, x_dis, y_dis);
    asg_make_label(areastruct, NORMAL, x, y, name);

    return(1);
}


/*----------------------------------------------------------------------*/
/* This function is used to draw an ASG module as an xcircuit object	*/
/* instance.								*/
/*----------------------------------------------------------------------*/

int asg_make_instance(XCWindowData *clientData, char *keyword, int x,
	int y, int x_size, int y_size)
{
   objectptr pobj;
   objinstptr pinst, *newinst;
   XPoint newpos;
   
   // Set Position for newly created object
   newpos.x = (short)(x * 15.0);
   newpos.y = (short)(y * 15.0);

   // Create Space for Object Instances...
   NEW_OBJINST(newinst, clientData->topinstance->thisobject);
   
   // Find Object instance in the library...
   pobj = NameToObject(keyword, &pinst, FALSE);

   // If object specified by keyword, it must exist in a loaded library.
   // (need to develop this code)

   if (pobj == NULL) {
      /* sprintf(_STR, "Cannot Find %s in any existing library.", keyword); */
      /* Wprintf(_STR);							    */
      return -1;
   }
  
   // Copy values from the library instance into the new instance
   instcopy(*newinst, pinst); 
   (*newinst)->scale = 1.0;

   // Rotated objects should be using the rotated instances.
   // Instances at rotation = 0 in the asg_spice.lps library should
   // match the orientations of devices in the original SPAR
   // definitions, or vice versa.

   if (!strcmp(keyword, "arrow")) (*newinst)->rotation += 90;
   else if (!strcmp(keyword, "RESTR")) (*newinst)->rotation += 270;
   else if (!strcmp(keyword, "INDR")) (*newinst)->rotation += 270;
   else if (!strcmp(keyword, "CAPC")) (*newinst)->rotation += 270;
   else if (!strcmp(keyword, "IAMP")) (*newinst)->rotation += 270;

   // Eventually all drawing parameters need to be overridden.
   // Assign just position for now 

   (*newinst)->position = newpos;
   return 0;
}

/*----------------------------------------------------------------------*/
/* This function draws the path connecting the object on the screen	*/
/*----------------------------------------------------------------------*/

void asg_make_polygon(XCWindowData *clientData, int npoints, int x[], int y[])
{
   polyptr *newpoly;
   int j;
   
   NEW_POLY(newpoly, clientData->topinstance->thisobject);
   polydefaults(*newpoly, npoints, 0, 0);
   (*newpoly)->style = UNCLOSED;
   for (j = 0; j < npoints; j++) {
      (*newpoly)->points[j].x = x[j];
      (*newpoly)->points[j].y = y[j];
   }
   /* singlebbox((genericptr *)newpoly); */
   incr_changes(clientData->topinstance->thisobject);
}

/*----------------------------------------------------------------------*/
/* This function helps print the label of the object on the screen	*/
/*----------------------------------------------------------------------*/

void asg_make_label(XCWindowData *clientData, u_char dopin, int x, int y, char *data)
{
   labelptr *newlab;
   stringpart *strptr;	

   NEW_LABEL(newlab, clientData->topinstance->thisobject);
   labeldefaults(*newlab, dopin, x - 20, y - 30);
	
   // (*newlab)->justify = PINVISIBLE;
   strptr = makesegment(&((*newlab)->string), NULL);
   strptr->type = TEXT_STRING;
   strptr->data.string = data;
   singlebbox((genericptr *)newlab);
   incr_changes(clientData->topinstance->thisobject);
}

/*----------------------------------------------------------------------*/
/* Free memory for SPAR structures					*/
/*----------------------------------------------------------------------*/

void spar_destructor()
{
   free_list(modules);
   free_list(nets);
}

/*----------------------------------------------------------------------*/
/* Generate SPAR structures						*/
/*----------------------------------------------------------------------*/

void spar_constructor()
{
   newnode("$0");
   newnode("$1");
   newnode("_DUMMY_SIGNAL_");
}

#endif /* ASG */
