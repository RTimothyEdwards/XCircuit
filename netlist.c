/*----------------------------------------------------------------------*/
/* netlist.c --- xcircuit routines specific to schematic capture and	*/
/*		 netlist generation					*/
/*----------------------------------------------------------------------*/
/*  Copyright (c) 2004 Tim Edwards, Johns Hopkins University,		*/
/*	MultiGiG, Inc., and Open Circuit Design, Inc.			*/
/*  Copyright (c) 2005 Tim Edwards, MultiGiG, Inc.			*/
/*									*/
/*  Written April 1998 to January 2004				   	*/
/*  Original version for Pcb netlisting 3/20/98 by Chow Seong Hwai,	*/
/*			Leeds University, U.K.				*/
/*									*/
/*  Greatly modified 4/2/04 to handle bus notation; net identifier	*/
/*  changed from a listing by net to two listings, by polygon and by	*/
/*  label, with nets and subnets identifiers attached to each element,	*/
/*  rather than vice versa.						*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>	/* For preventing multiple file inclusions, use stat() */
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef HAVE_PYTHON
#include <Python.h>
#endif

#ifndef _MSC_VER
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#ifdef TCL_WRAPPER 
#include <tk.h>
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include "xcircuit.h"
#include "colordefs.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

#ifdef HAVE_PYTHON
extern PyObject *PyGetStringParts(stringpart *);
#endif
#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
extern Tcl_Obj *TclGetStringParts(stringpart *);
#endif

/*----------------------------------------------------------------------*/
/* Externally declared global variables					*/
/*----------------------------------------------------------------------*/

extern Display  *dpy;

/*----------------------------------------------------------------------*/

extern char _STR[150];
extern char _STR2[250];
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern Boolean load_in_progress;
extern int number_colors;
extern colorindex *colorlist;

LabellistPtr global_labels;

typedef struct _flatindex *fidxptr;

typedef struct _flatindex {
   char *devname;
   u_int index;
   fidxptr next;
} flatindex;	/* count for labeling devices in a flattened file */

flatindex *flatrecord = NULL;

static char *spice_devname = "X";   /* SPICE subcircuit device name */
Boolean spice_end = True;	    /* whether or not to write a .end statement */
ino_t *included_files = NULL;	/* Files included with "%F" escape	*/

#define EndPoint(n)	(((n == 1) ? 1 : (int)(n - 1)))
#define NextPoint(n)	(((n == 1) ? 0 : 1))

/* For bus matching:  whether to match by number of nets, by		*/
/* exact match of sub-bus numbers, or an exact match of net IDs.	*/

#define MATCH_EXACT	0
#define MATCH_SUBNETS	1
#define MATCH_SIZE	2
#define MATCH_OVERLAP	3

/*----------------------------------------------------------------------*/
/* Check whether two line segments attach to or cross each other	*/
/* Return 1 if attached, 0 if not (INLINE code)				*/
/* int onsegment(XPoint *a, XPoint *b, XPoint *x) {}			*/
/*----------------------------------------------------------------------*/

#define ONDIST 4  /* "slack" in algorithm for almost-touching lines */
#define onsegment(a, b, x) (finddist(a, b, x) <= ONDIST)

/*--------------------------------------------------------------*/
/* d36a:  Base 36 to string conversion				*/
/*--------------------------------------------------------------*/

char *d36a(int number)
{
   static char bconv[10];
   int i, locn, rem;
   

   bconv[9] = '\0';
   i = 8;
   locn = number;
   while ((locn > 0) && (i >= 0)) {
      rem = locn % 36;
      locn /= 36;
      bconv[i--] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A');
   }
   return &bconv[i + 1];
}

/*--------------------------------------------------------------*/
/* Translate a pin name to a position relative to an object's	*/
/* point of origin.  This is used by the ASG module to find	*/
/* the placement of pins based on names from a netlist.		*/
/*								*/
/* returns 0 on success and fills x_ret and y_ret with the	*/
/* pin position coordinates.  Returns -1 if the pin name did	*/
/* not match any label names in the object.			*/
/*--------------------------------------------------------------*/

int NameToPinLocation(objinstptr thisinst, char *pinname, int *x_ret, int *y_ret)
{
   objectptr thisobj = thisinst->thisobject;
   genericptr *pgen;
   labelptr plab;

   if (thisobj->schemtype == SECONDARY)
      thisobj = thisobj->symschem;

   for (pgen = thisobj->plist; pgen < thisobj->plist + thisobj->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 plab = TOLABEL(pgen);
	 if (plab->pin != False && plab->pin != INFO) {
	    if (!textcomp(plab->string, pinname, thisinst)) { 
	       *x_ret = (int)plab->position.x;
	       *y_ret = (int)plab->position.y;
	       return 0;
	    }
	 }
      }
   }
   return -1;
}

/*--------------------------------------------------------------*/
/* Translate a point position back to the calling object's	*/
/* coordinate system.						*/
/* Original value is passed in "thispoint"; Translated value	*/
/* is put in "refpoint".					*/
/*--------------------------------------------------------------*/

void ReferencePosition(objinstptr thisinst, XPoint *thispoint, XPoint *refpoint)
{
  /* objectptr thisobj = thisinst->thisobject; (jdk) */
   Matrix locctm;

   /* Translate pin position back to originating object */

   UResetCTM(&locctm);
   UPreMultCTM(&locctm, thisinst->position, thisinst->scale,
                        thisinst->rotation);
   UTransformbyCTM(&locctm, thispoint, refpoint, 1);
}

/*--------------------------------------------------------------*/
/* Variant of NameToPinLocation():  Given a port number, find	*/
/* the pin associated with that port.				*/
/*--------------------------------------------------------------*/

labelptr PortToLabel(objinstptr thisinst, int portno)
{
   labelptr plab;
   objectptr thisobj = thisinst->thisobject;
   PortlistPtr ports;

   if ((thisobj->schemtype == SYMBOL) && (thisobj->symschem != NULL))
      ports = thisobj->symschem->ports;
   else
      ports = thisobj->ports;

   for (; ports != NULL; ports = ports->next) {
      if (ports->portid == portno) {
	 plab = NetToLabel(ports->netid, thisobj);	/* in the symbol! */
	 return plab;
      }
   }
   return NULL;
}

/*--------------------------------------------------------------*/
/* This function is the same as PortToLocalPosition() but	*/
/* returns the point in the coordinate system of the parent.	*/
/*--------------------------------------------------------------*/

Boolean PortToPosition(objinstptr thisinst, int portno, XPoint *refpoint)
{
   labelptr plab = PortToLabel(thisinst, portno);
   if (plab)
      ReferencePosition(thisinst, &(plab->position), refpoint);

   return (plab != NULL) ? TRUE : FALSE;
}

/*----------------------------------------------------------------------*/
/* Get the hierarchy of the page stack and return as a string		*/
/*----------------------------------------------------------------------*/

Boolean getnexthier(pushlistptr stack, char **hierstr, objinstptr callinst,
	Boolean canonical)
{
   objectptr topobj, thisobj;
   CalllistPtr calls;
   /* char numstr[12]; (jdk) */
   int hierlen, devlen;
   char *devstr;

   if (!stack) return False;

   /* Recurse so we build up the prefix string from top down */
   if (stack->next != NULL) {
      if (getnexthier(stack->next, hierstr, stack->thisinst, canonical) == False)
	 return False;
   }
   else {
      topobj = stack->thisinst->thisobject;

      if (topobj->schemtype != PRIMARY && topobj->symschem != NULL)
	 topobj = topobj->symschem;

      if (topobj->calls == NULL) {
	 if (topobj->schemtype == FUNDAMENTAL) {
	    /* Top level is a fundamental symbol */
	    return True;
	 }
	 else if ((updatenets(stack->thisinst, FALSE) <= 0) || (topobj->calls == NULL)) {
	    Wprintf("Error in generating netlists!");
	    return False;
	 }
      }
   }

   thisobj = stack->thisinst->thisobject;

   /* When we have "traveled" through both a symbol and its	*/
   /* associated schematic, only append the prefix for the	*/
   /* first one encountered.					*/

/*
   if (thisobj->symschem != NULL && stack->next != NULL &&
		thisobj->symschem == stack->next->thisinst->thisobject)
      return True;
*/

   if ((thisobj->calls == NULL) && (thisobj->schemtype != PRIMARY)
		&& (thisobj->symschem != NULL))
      thisobj = thisobj->symschem;

   /* Check for resolved device indices and generate them if necessary */

   for (calls = thisobj->calls; calls != NULL; calls = calls->next)
      if (calls->callinst == callinst)
	 if (calls->devindex == -1) {
	    cleartraversed(thisobj);
	    resolve_indices(thisobj, FALSE);
	    break;
	 }

   /* Add this level of the hierarchy to the prefix string */

   for (calls = thisobj->calls; calls != NULL; calls = calls->next) {
      if (calls->callinst == callinst) {	
	 devlen = (canonical || calls->devname == NULL) ?
		strlen(callinst->thisobject->name) : strlen(calls->devname);
	 devstr = d36a(calls->devindex);
	 devlen += strlen(devstr) + 1;
	 if (*hierstr == NULL) {
	    *hierstr = malloc(devlen);
	    hierlen = 0;
	 }
	 else {
	    hierlen = strlen(*hierstr) + 2;
	    *hierstr = realloc(*hierstr, hierlen + devlen);
	 }
	 if (canonical)
	    sprintf(*hierstr + hierlen, "%s%s(%s)",
		((hierlen > 0) ? "/" : ""),
		callinst->thisobject->name, devstr);
	 else
	    sprintf(*hierstr + hierlen, "%s%s%s",
		((hierlen > 0) ? "/" : ""),
		(calls->devname == NULL) ?  callinst->thisobject->name
			: calls->devname, devstr);
	 break;
      }
   }
   return True;
}

/*----------------------------------------------------------------------*/

char *GetHierarchy(pushlistptr *stackptr, Boolean canonical)
{
   Boolean pushed_top = FALSE;
   char *snew = NULL;

   if ((*stackptr) && ((*stackptr)->thisinst != areawin->topinstance)) {
      pushed_top = TRUE;
      push_stack(stackptr, areawin->topinstance, NULL);
   }

   getnexthier(*stackptr, &snew, NULL, canonical);

   if (pushed_top) pop_stack(stackptr);

   return snew;
}

/*----------------------------------------------------------------------*/
/* Invalidate a netlist.  The invalidation must be referred to the	*/
/* master schematic, if the object is a secondary schematic.		*/
/*----------------------------------------------------------------------*/

void invalidate_netlist(objectptr thisobject)
{
   if (thisobject->schemtype != NONETWORK) {
      if (thisobject->schemtype == SECONDARY)
         thisobject->symschem->valid = False;
      else
         thisobject->valid = False;
   }
}

/*--------------------------------------------------------------*/
/* Check if the selected items are relevant to the netlist.	*/
/* Only invalidate the current netlist if they are.		*/
/*--------------------------------------------------------------*/

void select_invalidate_netlist()
{
   int i;
   Boolean netcheck = FALSE;

   for (i = 0; i < areawin->selects; i++) {
      genericptr gptr = SELTOGENERIC(areawin->selectlist + i);
      switch (gptr->type) {
	 case POLYGON:
	    if (!nonnetwork(TOPOLY(&gptr)))
	       netcheck = TRUE;
	    break;
	 case LABEL:
            if ((TOLABEL(&gptr))->pin == LOCAL || (TOLABEL(&gptr))->pin == GLOBAL)
	       netcheck = TRUE;
	    break;
	 case OBJINST:
	    if ((TOOBJINST(&gptr))->thisobject->schemtype != NONETWORK)
	       netcheck = TRUE;
	    break;
      }
   }
   if (netcheck) invalidate_netlist(topobject);
}

/*------------------------------------------------------------------*/
/* Check proximity of two points (within roundoff tolerance ONDIST) */
/*------------------------------------------------------------------*/

Boolean proximity(XPoint *point1, XPoint *point2)
{
   int dx, dy;

   dx = point1->x - point2->x;
   dy = point1->y - point2->y;

   if ((abs(dx) < ONDIST) && (abs(dy) < ONDIST)) return True;
   else return False;
}

/*----------------------------------------------------------------------*/
/* createnets(): Generate netlist structures				*/
/*									*/
/* Result is the creation of three linked lists inside each object in   */
/* the circuit hierarchy:						*/
/*									*/
/*    1) the netlist:  assigns a number to each network of polygons and	*/
/*	 	pin labels on the object.				*/
/*    2) the ports:  a list of every network which is connected from	*/
/*		objects above this one in the hierarchy.		*/
/*    3) the calls: a list of calls made from the object to all sub-	*/
/*		circuits.  Each calls indicates the object and/or	*/
/*		instance being called, and a port list which must match */
/*		the ports of the object being called (the port lists */
/*		are not reflexive, as the caller does not have to call	*/
/*		all of the instance's ports, but the instance must have	*/
/*		a port defined for every connection being called).	*/ 
/*    (see structure definitions in xcircuit.h).	   		*/
/*----------------------------------------------------------------------*/

void createnets(objinstptr thisinst, Boolean quiet)
{
   objectptr thisobject = thisinst->thisobject;

   if (!setobjecttype(thisobject)) {

      /* New in 3.3.32:  Generating a netlist from a symbol is	*/
      /* okay if the symbol has an associated netlist.		*/

      if (thisobject->schemtype == SYMBOL && thisobject->symschem != NULL)
	 thisobject = thisobject->symschem;
      else {
	 if (!quiet)
            Wprintf("Error:  attempt to generate netlist for a symbol.");
	 return;
      }
   }
	 

   /* Wprintf("Generating netlists"); */ /* Diagnostic */
   gennetlist(thisinst);
   gencalls(thisobject);
   cleartraversed(thisobject);
   resolve_devnames(thisobject);
   /* Wprintf("Finished netlists"); */

}

/*----------------------------------------------------------------------*/
/* Free all memory associated with the netlists.			*/
/*----------------------------------------------------------------------*/

void destroynets(objectptr thisobject)
{
   objectptr pschem;

   pschem = (thisobject->schemtype == SECONDARY) ? thisobject->symschem :
		thisobject;

   freetemplabels(pschem);
   freenets(pschem);
   freeglobals();
}

/*----------------------------------------------------------------------*/
/* Polygon types which are ignored when considering if a polygon	*/
/* belongs to a circuit network:					*/ 
/* 	Closed polygons (boxes)						*/
/*	Bounding box polygons						*/
/*	Filled polygons							*/
/*	Dashed and dotted line-style polygons				*/
/*----------------------------------------------------------------------*/

Boolean nonnetwork(polyptr cpoly)
{
   if (!(cpoly->style & UNCLOSED)) return True;
   if (cpoly->style & (DASHED | DOTTED | FILLSOLID | BBOX))
      return True;
   return False;
}

/*----------------------------------------------------------------------*/
/* Return the largest (most negative) net number in the global netlist	*/
/*----------------------------------------------------------------------*/

int globalmax()
{
   LabellistPtr gl;
   int bidx, sbus;
   buslist *lbus;
   int smin = 0;

   for (gl = global_labels; gl != NULL; gl = gl->next) {
      if (!(gl->subnets)) {
	 if (gl->net.id < smin)
	    smin = gl->net.id;
      }
      else {
	 for (bidx = 0; bidx < gl->subnets; bidx++) {
	    lbus = gl->net.list + bidx;
	    sbus = lbus->netid;
	    if (sbus < smin)
	       smin = sbus;
	 }
      }
   }
   return smin;
}

/*----------------------------------------------------------------------*/
/* Return the largest net number in an object's netlist			*/
/*----------------------------------------------------------------------*/

int netmax(objectptr cschem)
{
   PolylistPtr gp;
   LabellistPtr gl;
   int bidx, sbus;
   buslist *lbus;
   int smax = 0;

   for (gp = cschem->polygons; gp != NULL; gp = gp->next) {
      if (!(gp->subnets)) {
	 if (gp->net.id > smax)
	    smax = gp->net.id;
      }
      else {
	 for (bidx = 0; bidx < gp->subnets; bidx++) {
	    lbus = gp->net.list + bidx;
	    sbus = lbus->netid;
	    if (sbus > smax)
	       smax = sbus;
	 }
      }
   }
   for (gl = cschem->labels; gl != NULL; gl = gl->next) {
      if (!(gl->subnets)) {
	 if (gl->net.id > smax)
	    smax = gl->net.id;
      }
      else {
	 for (bidx = 0; bidx < gl->subnets; bidx++) {
	    lbus = gl->net.list + bidx;
	    sbus = lbus->netid;
	    if (sbus > smax)
	       smax = sbus;
	 }
      }
   }
   return smax;
}

/*----------------------------------------------------------------------*/
/* Resolve nets and pins for the indicated object			*/
/*									*/
/* When encountering object instances, call gennetlist() on the object  */
/* if it does not have a valid netlist, then recursively call		*/
/* gennetlist().  Ignore "info" labels, which are not part of the	*/
/* network.								*/
/*									*/
/*----------------------------------------------------------------------*/

void gennetlist(objinstptr thisinst)
{
   genericptr *cgen;
   labelptr olabel, clab;
   polyptr cpoly, tpoly;
   objectptr thisobject, callobj, cschem, pschem;
   objinstptr cinst, labinst;
   int old_parts; /* netid, tmpid, sub_bus, (jdk) */
   stringpart *cstr;
   Boolean visited;

   XPoint *tpt, *tpt2, *endpt, *endpt2;
   int i, j, n, nextnet, lbus;
   buslist *sbus;
   PolylistPtr plist;
   LabellistPtr lseek;
   Genericlist *netlist, *tmplist, *buspins, *resolved_net, newlist;

   newlist.subnets = 0;
   newlist.net.id = -1;

   /* Determine the type of object being netlisted */
   thisobject = thisinst->thisobject;
   setobjecttype(thisobject);

   if (thisobject->schemtype == NONETWORK) return;

   /* Has this object been visited before? */
   visited = ((thisobject->labels != NULL) || (thisobject->polygons != NULL)) ?
		TRUE : FALSE;

   if (!visited && (thisobject->schemtype == SYMBOL)
		&& (thisobject->symschem != NULL)) {

      /* Make sure that schematics are netlisted before their symbols */

      if ((thisobject->symschem->polygons == NULL) &&
			(thisobject->symschem->labels == NULL)) {
	 n = is_page(thisobject->symschem);
	 if (n == -1) {
	    Fprintf(stderr, "Error: associated schematic is not a page!\n");
	    return;
	 }
	 else
	    gennetlist(xobjs.pagelist[n]->pageinst);
      }

      /* Sanity check on symbols with schematics:  Are there any pins? */

      for (i = 0; i < thisobject->parts; i++) {
	 cgen = thisobject->plist + i;
	 if (IS_LABEL(*cgen)) {
	    clab = TOLABEL(cgen);
	    if (clab->pin != False && clab->pin != INFO)
	       break;
	 }
      }
      if (i == thisobject->parts)
	 /* Don't warn if schematic has infolabels */
	 if (thisobject->symschem == NULL || thisobject->symschem->infolabels == 0)
	    Fprintf(stderr, "Warning:  Symbol %s has no pins!\n", thisobject->name);
   }

   /* If this is a secondary schematic, run on the primary (master) */
   pschem = (thisobject->schemtype == SECONDARY) ? thisobject->symschem :
		thisobject;

   nextnet = netmax(pschem) + 1;

   /* We start the loop for schematics but will modify the loop	*/
   /* variable to execute just once in the case of a symbol.	*/
   /* It's just not worth the trouble to turn this into a	*/
   /* separate subroutine. . .					*/

   for (j = 0; j < xobjs.pages; j++) {
      if (pschem->schemtype != PRIMARY) {
	 j = xobjs.pages;
	 cinst = thisinst;
	 cschem = thisobject;
      }
      else {
         cinst = xobjs.pagelist[j]->pageinst;
         if ((cinst == NULL) ||
		((cinst->thisobject != pschem) &&
		((cinst->thisobject->schemtype != SECONDARY) ||
		(cinst->thisobject->symschem != pschem)))) continue;
         cschem = cinst->thisobject;
      }

      /* Determine the existing number of parts.  We do not want to	*/
      /* search over elements that we create in this routine.		*/

      old_parts = cschem->parts;

      /* Part 1:  Recursion */
      /* Schematic pages and symbols acting as their own schematics are the	*/
      /* two types on which we recurse to find all sub-schematics.		*/

      if ((!visited) && (cschem->schemtype == PRIMARY ||
		cschem->schemtype == SECONDARY ||
		(cschem->schemtype == SYMBOL && cschem->symschem == NULL))) {

         for (i = 0; i < old_parts; i++) {
            cgen = cschem->plist + i;
            if (IS_OBJINST(*cgen)) {
	       objinstptr geninst, callinst;
	       geninst = TOOBJINST(cgen);
	
	       if (geninst->thisobject->symschem != NULL) {
	          callobj = geninst->thisobject->symschem;
		  n = is_page(callobj);
		  if (n == -1) {
		     Fprintf(stderr, "Error: associated schematic is not a page!\n");
		     continue;
		  }
		  else
	             callinst = xobjs.pagelist[n]->pageinst;
	       }
	       else {
	          callobj = geninst->thisobject;
	          callinst = geninst;
	       }
	
	       /* object on its own schematic */
	       if (callobj == pschem) continue;

	       gennetlist(callinst);

	       /* Also generate netlist for pins in the corresponding symbol */
	       if (geninst->thisobject->symschem != NULL)
	          gennetlist(geninst);
            }
	 }
      }

      /* Part 2:  Match pin labels to nets, and add to list of globals	*/
      /* if appropriate.  We do all pins first, before ennumerating	*/
      /* polygons, so that buses can be handled correctly, generating	*/
      /* overlapping subnets.						*/

      for (i = 0; i < old_parts; i++) {
         cgen = cschem->plist + i;
         if (IS_LABEL(*cgen)) {
	    clab = TOLABEL(cgen);
	    if (clab->pin != False && clab->pin != INFO) {

	       /* Check if the label has a non-default parameter. */
	       /* If so, we want to record the instance along	  */
	       /* the other netlist information about the pin.	  */

	       labinst = NULL;
	       for (cstr = clab->string; cstr != NULL; cstr = cstr->nextpart)
		  if (cstr->type == PARAM_START)
		     if (match_instance_param(cinst, cstr->data.string) != NULL) {
			labinst = cinst;
			break;
		     }

	       /* If we have netlisted this object before, and the	*/
	       /* label is already in the netlist, then ignore it.	*/
	       /* This only happens for labels without instanced	*/
	       /* parameter values.					*/
	
	       if (visited && (labinst == NULL)) {
		  for (lseek = pschem->labels; lseek != NULL; lseek = lseek->next)
		     if ((lseek->label == clab) && (lseek->cinst == NULL))
		        break;
		  if (lseek != NULL) continue;
	       }
	       else if (visited)
		  netlist = NULL;
	       else
		  /* Note any overlapping labels. */
	          netlist = pointtonet(cschem, cinst, &clab->position);

               if (pschem->symschem != NULL && pschem->schemtype == SYMBOL) {

		  /* For symbols:  Check that this pin has a		*/
		  /* corresponding pin in the schematic.  The schematic	*/
		  /* is netlisted before the symbol, so the netlist	*/
		  /* should be there.					*/

		  tmplist = pintonet(pschem->symschem, cinst, clab);

	          /* There is always the possibility that the symbol	*/
		  /* pin does not correspond to anything in the		*/
		  /* schematic.  If so, it gets its own unique net	*/
		  /* number.  HOWEVER, this situation usually indicates	*/
		  /* an error in the schematic, so output a warning	*/
		  /* message.						*/

	          if (tmplist == NULL) {
		     char *snew = NULL;
		     snew = textprint(clab->string, NULL),
		     Fprintf(stderr, "Warning:  Pin \"%s\" in symbol %s has no "
				"connection in schematic %s\n",
				snew, pschem->name,
				pschem->symschem->name);
		     free(snew);
		     newlist.net.id = nextnet++;
		     tmplist = &newlist;
	          }
	       }
	       else {
		  /* For schematics:  Relate the pin to a network */
		  tmplist = pintonet(pschem, cinst, clab);

		  /* If only part of a bus has been identified, generate */
		  /* the net ID's of the parts that haven't been.	 */

		  if ((tmplist != NULL) && (tmplist->subnets > 0)) {
		     for (lbus = 0; lbus < tmplist->subnets; lbus++) {
		        sbus = tmplist->net.list + lbus;
		        if (sbus->netid == 0)
			   sbus->netid = nextnet++;
		     }
		  }
	       }

	       if (clab->pin == LOCAL) {
	          if (tmplist != NULL) {
	             addpin(cschem, labinst, clab, tmplist);
	             if (netlist != NULL)
	                mergenets(pschem, netlist, tmplist);
	          }
	          else {
	             if (netlist == NULL) {
		        netlist = &newlist;
		        netlist->net.id = nextnet++;
			buspins = break_up_bus(clab, cinst, netlist);
			if (buspins != NULL) {
			   buslist *sbus = buspins->net.list + buspins->subnets - 1;
			   nextnet = sbus->netid + 1;
	                   netlist = addpin(cschem, labinst, clab, buspins);
			}
			else {
	                   tmplist = addpin(cschem, labinst, clab, netlist);
		           netlist = tmplist;
			}
		     }
		     else {
	                tmplist = addpin(cschem, labinst, clab, netlist);
		        netlist = tmplist;
		     }
	          }
	       }
	       else if (clab->pin == GLOBAL) {
	          if (tmplist == NULL) {
		     tmplist = &newlist;
	             tmplist->net.id = globalmax() - 1;
		  }
	          addglobalpin(cschem, cinst, clab, tmplist);
	          addpin(cschem, labinst, clab, tmplist);
	          if (netlist != NULL)
	             mergenets(pschem, netlist, tmplist);
	       }
	    }
	    else if (clab->pin == INFO) {
	       /* Note the presence of info labels in the	*/
	       /* subcircuit, indicating that it needs to be	*/
	       /* called because it will generate output.	*/
               cschem->infolabels = TRUE;
	    }
         }
      }

      /* Part 3: Polygon enumeration				*/
      /* Assign network numbers to all polygons in the object.	*/
      /* (Ignore symbols, as we only consider pins on symbols)	*/
      /* Where multiple pins occur at a point (bus notation),	*/
      /* polygons will be duplicated in each network.		*/

      if ((!visited) && (cschem->schemtype == PRIMARY ||
		cschem->schemtype == SECONDARY ||
		(cschem->schemtype == SYMBOL && cschem->symschem == NULL))) {

	 for (i = 0; i < old_parts; i++) {
	    cgen = cschem->plist + i;
            if (IS_POLYGON(*cgen)) {
 	       cpoly = TOPOLY(cgen);

	       /* Ignore non-network (closed, bbox, filled) polygons */
	       if (nonnetwork(cpoly)) continue;

	       resolved_net = (Genericlist *)NULL;

	       /* Check for attachment of each segment of this polygon	*/
	       /* to position of every recorded pin label.			*/

	       for (lseek = pschem->labels; lseek != NULL; lseek = lseek->next) {
		  if (lseek->cschem != cschem) continue;
		  else if ((lseek->cinst != NULL) && (lseek->cinst != cinst))
		     continue;
		  olabel = lseek->label;
		  tmplist = (Genericlist *)lseek;
	          for (endpt = cpoly->points; endpt < cpoly->points
		   		+ EndPoint(cpoly->number); endpt++) {
	             endpt2 = endpt + NextPoint(cpoly->number);
		     if (onsegment(endpt, endpt2, &olabel->position)) {

			if (resolved_net != NULL) {
			   if (mergenets(pschem, resolved_net, tmplist))
			      resolved_net = tmplist;
		        }
			if (resolved_net == NULL) {
		           addpoly(cschem, cpoly, tmplist);
			   resolved_net = tmplist;
		        }
		     }
	          }
	          /* if we've encountered a unique instance, then con-	*/
	          /* tinue past all other instances using this label.	*/
	          if (lseek->cinst != NULL)
		     while (lseek->next && (lseek->next->label == lseek->label))
		        lseek = lseek->next;
	       }

	       /* Check for attachment of each segment of this polygon */
	       /* to endpoints of every recorded network polygon.      */

	       for (plist = pschem->polygons; plist != NULL; plist = plist->next) { 
		  if (plist->cschem != cschem) continue;
	          else if ((tpoly = plist->poly) == cpoly) continue;
		  tpt = tpoly->points;
		  tpt2 = tpoly->points + tpoly->number - 1;
		  tmplist = (Genericlist *)plist;

	          for (endpt = cpoly->points; endpt < cpoly->points
		   		+ EndPoint(cpoly->number); endpt++) {
	             endpt2 = endpt + NextPoint(cpoly->number);

		     if (onsegment(endpt, endpt2, tpt) ||
				onsegment(endpt, endpt2, tpt2)) {

		        /* Nets previously counted distinct have    */
		        /* been connected together by this polygon. */
			if (resolved_net != NULL) {
			   if (mergenets(pschem, resolved_net, tmplist))
		   	      resolved_net = tmplist;
		        }
		        if (resolved_net == NULL) {
		           addpoly(cschem, cpoly, tmplist);
		           resolved_net = tmplist;
		        }
		     }
		  }

	          /* Check for attachment of the endpoints of this polygon */
	          /* to each segment of every recorded network polygon.	   */

		  endpt = cpoly->points;
		  endpt2 = cpoly->points + cpoly->number - 1;
	          for (tpt = tpoly->points; tpt < tpoly->points
			   + EndPoint(tpoly->number); tpt++) {
		     tpt2 = tpt + NextPoint(tpoly->number);

	     	     if (onsegment(tpt, tpt2, endpt) ||
			   	onsegment(tpt, tpt2, endpt2)) {

		        /* Nets previously counted distinct have    */
		        /* been connected together by this polygon. */
			if (resolved_net != 0) {
			   if (mergenets(pschem, resolved_net, tmplist))
		              resolved_net = tmplist;
		        }
		        if (resolved_net == 0) {
		           addpoly(cschem, cpoly, tmplist);
		           resolved_net = tmplist;
		        }
		     }
	          }
	       }
	       if (resolved_net == 0) {

	          /* This polygon belongs to an unvisited	*/
	          /* network.  Give this polygon a new net	*/
	          /* number and add to the net list.		*/

		  newlist.net.id = nextnet++;
	          addpoly(cschem, cpoly, &newlist);
	       }
	    }
         }
      }
   }
}

/*--------------------------------------------------------------*/
/* Search a sibling of object instance "cinst" for connections	*/
/* between the two.  Recurse through the schematic hierarchy of	*/
/* the sibling object.						*/
/*--------------------------------------------------------------*/

void search_on_siblings(objinstptr cinst, objinstptr isib, pushlistptr schemtop,
	short llx, short lly, short urx, short ury)
{
   XPoint *tmppts, sbbox[2];
   int i;
   labelptr olabel;
   polyptr tpoly;
   PolylistPtr pseek;
   LabellistPtr lseek;

   genericptr *iseek;
   objinstptr subsibinst;
   pushlistptr psearch, newlist;
   objectptr  sibling = isib->thisobject;
   
   tmppts = (XPoint *)malloc(sizeof(XPoint));

   /* If the sibling is a symbol or fundamental or trivial object, then */
   /* we just look at pin labels from the parts list and return.	*/

   if (sibling->symschem != NULL || sibling->schemtype == FUNDAMENTAL
		|| sibling->schemtype == TRIVIAL) {
      for (lseek = sibling->labels; lseek != NULL; lseek = lseek->next) {
	 olabel = lseek->label;

	 tmppts = (XPoint *)realloc(tmppts, sizeof(XPoint));
	 UTransformPoints(&(olabel->position), tmppts, 1,
			isib->position, isib->scale, isib->rotation);

	 /* Transform all the way up to the level above cinst */
	 for (psearch = schemtop; psearch != NULL; psearch = psearch->next) {
	    subsibinst = psearch->thisinst;
	    UTransformPoints(tmppts, tmppts, 1, subsibinst->position,
				subsibinst->scale, subsibinst->rotation);
	 }
	 searchconnect(tmppts, 1, cinst, lseek->subnets);
      }
   }

   /* If the sibling is a schematic, we look at connections to pins and */
   /* polygons, and recursively search subschematics of the sibling.	*/

   else {

      /* Look for pins connecting to pins in the object */

      for (lseek = sibling->labels; lseek != NULL; lseek = lseek->next) {
	 olabel = lseek->label;
	 tmppts = (XPoint *)realloc(tmppts, sizeof(XPoint));
	 UTransformPoints(&(olabel->position), tmppts, 1,
			isib->position, isib->scale, isib->rotation);

	 /* Transform all the way up to the level above cinst */
	 for (psearch = schemtop; psearch != NULL; psearch = psearch->next) {
	    subsibinst = psearch->thisinst;
	    UTransformPoints(tmppts, tmppts, 1, subsibinst->position,
			subsibinst->scale, subsibinst->rotation);
	 }
	 searchconnect(tmppts, 1, cinst, lseek->subnets);
      }

      /* Look for polygon ends connecting into the object */

      for (pseek = sibling->polygons; pseek != NULL; pseek = pseek->next) {
	 tpoly = pseek->poly;
	 tmppts = (XPoint *)realloc(tmppts, tpoly->number * sizeof(XPoint));
	 UTransformPoints(tpoly->points, tmppts, tpoly->number,
				isib->position, isib->scale, isib->rotation);

	 /* Transform all the way up to the level above cinst */
	 for (psearch = schemtop; psearch != NULL; psearch = psearch->next) {
	    subsibinst = psearch->thisinst;
	    UTransformPoints(tmppts, tmppts, tpoly->number, subsibinst->position,
				subsibinst->scale, subsibinst->rotation);
	 }
	 searchconnect(tmppts, tpoly->number, cinst, pseek->subnets);
      }

      /* Recursively search all schematic children of the sibling */

      for (i = 0; i < sibling->parts; i++) {
	 iseek = sibling->plist + i;
	 if (IS_OBJINST(*iseek)) {

	   /* objinstptr iinst = (objinstptr)iseek; (jdk) */

	    /* Don't search this instance unless the bounding box 	*/
	    /* overlaps the bounding box of the calling object.		*/

	    calcinstbbox(iseek, &sbbox[0].x, &sbbox[0].y, &sbbox[1].x, &sbbox[1].y);
	    for (psearch = schemtop; psearch != NULL; psearch = psearch->next) {
	       subsibinst = psearch->thisinst;
	       UTransformPoints(sbbox, sbbox, 2, subsibinst->position,
			subsibinst->scale, subsibinst->rotation);
	    }
	    if ((llx > sbbox[1].x) || (urx < sbbox[0].x) || (lly > sbbox[1].y)
			|| (ury < sbbox[0].y))
	       continue;

	    subsibinst = TOOBJINST(iseek);

	    /* push stack */
	    newlist = (pushlistptr)malloc(sizeof(pushlist));
	    newlist->thisinst = isib;
	    newlist->next = schemtop;
	    schemtop = newlist;

	    search_on_siblings(cinst, subsibinst, schemtop, llx, lly, urx, ury);

	    /* pop stack */

	    newlist = schemtop;
	    schemtop = schemtop->next;
	    free(newlist);
	 }
      }
   }
   free(tmppts);
}

/*--------------------------------------------------------------*/
/* Generate ports from pin labels for all objects:		*/
/* For each pin-label in the subcircuit or symbol, generate	*/
/* a port in the object or instance's object.			*/
/* Translate the pin position to the level above and search	*/
/* for & link in any connecting nets.				*/
/*								*/
/* Generate calls to object instances.				*/
/* Pick up any other nets which might be formed by		*/
/* juxtaposition of object instances on different levels of the */
/* schematic hierarchy (e.g., pin-to-pin connections).		*/
/*--------------------------------------------------------------*/
	
void gencalls(objectptr thisobject)
{
   genericptr *cgen, *iseek;
   Matrix locctm;
   objinstptr cinst, isib, callinst;
   objectptr callobj, callsymb, cschem, pschem;
   XPoint xpos;
   short ibllx, iblly, iburx, ibury, sbllx, sblly, sburx, sbury;
   int i, j, k; /* , lbus; (jdk) */
   /* buslist *sbus; (jdk) */
   labelptr olabel;
   polyptr tpoly;
   PolylistPtr pseek;
   /* CalllistPtr cseek; (jdk) */
   LabellistPtr lseek;
   Genericlist *netfrom, *netto; /* , *othernet; (jdk) */

   /* The netlist is always kept in the master schematic */
   pschem = (thisobject->schemtype == SECONDARY) ? thisobject->symschem :
		thisobject;

   pschem->traversed = True;	/* This object has been dealt with */
   pschem->valid = True;	/* This object has a valid netlist */

   /* We start the loop for schematics but will modify the loop	*/
   /* variable to execute just once in the case of a symbol.	*/
   /* (see gennetlist(), where the same thing is done)		*/

   for (j = 0; j < xobjs.pages; j++) {
      if (pschem->schemtype != PRIMARY) {
	 j = xobjs.pages;
	 cschem = thisobject;
      }
      else {
         cinst = xobjs.pagelist[j]->pageinst;
         if ((cinst == NULL) ||
		((cinst->thisobject != pschem) &&
		((cinst->thisobject->schemtype != SECONDARY) ||
		(cinst->thisobject->symschem != pschem)))) continue;
         cschem = cinst->thisobject;
      }

      for (i = 0; i < cschem->parts; i++) {
         cgen = cschem->plist + i;
         if (IS_OBJINST(*cgen)) {
	    callinst = TOOBJINST(cgen);

	    /* Determine where the hierarchy continues downward */

	    if (callinst->thisobject->symschem != NULL)
	       callobj = callinst->thisobject->symschem;
	    else
	       callobj = callinst->thisobject;

	    /* Ignore any object on its own schematic */

	    if (callobj == pschem) continue;

	    callsymb = callinst->thisobject;

	    /* Note:  callobj is the next schematic in the hierarchy.	*/
	    /* callsymb is the next visible object in the hierarchy, 	*/
	    /* which may be either a schematic or a symbol.		*/

	    /*-------------------------------------------------------------*/
	    /* For object instances which are their own schematics (i.e.,  */
	    /* have netlist elements), don't rely on any pin list but make */
	    /* a survey of how polygons connect into the object.	   */
	    /*-------------------------------------------------------------*/

	    if (callsymb->symschem == NULL
			&& callobj->schemtype != FUNDAMENTAL
			&& callobj->schemtype != TRIVIAL) {

               /* Fprintf(stdout, "*** Analyzing connections from %s"
			" to instance of %s\n", cschem->name,
			callinst->thisobject->name); */

	       /* Look for pins connecting to pins in the object */

	       for (lseek = pschem->labels; lseek != NULL; lseek = lseek->next) {
		  if (lseek->cschem != cschem) continue;
		  else if ((lseek->cinst != NULL) && (lseek->cinst != callinst))
		     continue;
	 	  olabel = lseek->label;
	          searchconnect(&(olabel->position), 1, callinst, lseek->subnets);
	          /* if we've encountered a unique instance, then con-	*/
	          /* tinue past all other instances using this label.	*/
	          if (lseek->cinst != NULL)
		     while (lseek->next && (lseek->next->label == lseek->label))
		        lseek = lseek->next;
	       }

	       /* Look for polygon ends connecting into the object */

	       for (pseek = pschem->polygons; pseek != NULL; pseek = pseek->next) {
		  if (pseek->cschem != cschem) continue;
		  tpoly = pseek->poly;
	          searchconnect(tpoly->points, tpoly->number, callinst, pseek->subnets);
	       }

	       /* For each call to a schematic or symbol which is NOT the	*/
	       /* one under consideration, see if it touches or overlaps	*/
	       /* the object under consideration.  Search for connections	*/
	       /* between the two objects.					*/

	       calcinstbbox(cgen, &ibllx, &iblly, &iburx, &ibury);

	       /* Only need to look forward from the current position. */
   	       for (k = i + 1; k < cschem->parts; k++) {

	          iseek = cschem->plist + k;
	          if (IS_OBJINST(*iseek)) {
		     calcinstbbox(iseek, &sbllx, &sblly, &sburx, &sbury);

		     /* Check intersection of the two object instances;	*/
		     /* don't do a search if they are disjoint.		*/

		     if ((ibllx <= sburx) && (iburx >= sbllx) &&
				(iblly <= sbury) && (ibury >= sblly)) {
		        isib = TOOBJINST(iseek);
		        search_on_siblings(callinst, isib, NULL,	
				ibllx, iblly, iburx, ibury);
		     }
	          }
	       }
	    }

	    /*----------------------------------------------------------*/
	    /* Recursively call gencalls() on the schematic.		*/
	    /*----------------------------------------------------------*/

	    if (callobj->traversed == False)
	       gencalls(callobj);

	    /*----------------------------------------------------------*/
	    /* Create a call to the object callsymb from object cschem	*/
	    /*----------------------------------------------------------*/

	    addcall(cschem, callobj, callinst);

	    /*----------------------------------------------------------*/
	    /* Search again on symbol pins to generate calls to ports. 	*/ 
	    /*----------------------------------------------------------*/

	    UResetCTM(&locctm);
	    UPreMultCTM(&locctm, callinst->position, callinst->scale,
				callinst->rotation);

	    for (lseek = callsymb->labels; lseek != NULL; lseek = lseek->next) {
	      /* LabellistPtr slab; (jdk) */
	      /* labelptr slabel; (jdk) */

	       if (lseek->cschem != callsymb) continue;
	       else if ((lseek->cinst != NULL) && (lseek->cinst != callinst))
		  continue;

	       olabel = lseek->label;
	       netto = (Genericlist *)lseek;
	
	       /* Translate pin position back to object cschem */
	       UTransformbyCTM(&locctm, &(olabel->position), &xpos, 1);  

	       /* What net in the calling object connects to this point? */
	       netfrom = pointtonet(cschem, callinst, &xpos);

	       /* If there's no net, we make one */
	       if (netfrom == NULL)
		  netfrom = make_tmp_pin(cschem, callinst, &xpos, netto);

	       /* Generate a port call for a global signal if the	*/
	       /* global label appears in the symbol (9/29/04).  This	*/
	       /* is a change from previous behavior, which was to not	*/
	       /* make the call.					*/

	       if ((netto->subnets == 0) && (netto->net.id < 0))
		  mergenets(pschem, netfrom, netto);

	       /* Attempt to generate a port in the object.	*/
	       addport(callobj, netto);

	       /* Generate the call to the port */
	       if (addportcall(pschem, netfrom, netto) == FALSE) {

		  // If object is "dot" then copy the bus from the
		  // net to the dot.  The dot takes on whatever
		  // dimension the bus is.

		  if (strstr(callobj->name, "::dot") != NULL) {
		     copy_bus(netto, netfrom);
		  }
		  else {
		     Fprintf(stderr, "Error:  attempt to connect bus size "
				"%d in %s to bus size %d in %s\n",
				netfrom->subnets, cschem->name,
				netto->subnets, callobj->name);
		  }
	       }

	       /* if we've encountered a unique instance, then continue	*/
	       /* past all other instances using this label.		*/
	       if (lseek->cinst != NULL)
		  while (lseek->next && (lseek->next->label == lseek->label))
		     lseek = lseek->next;
	    }

	    /*----------------------------------------------------------*/
	    /* If after all that, no ports were called, then remove the	*/
	    /* call to this object instance.  However, we should check	*/
	    /* for info labels, because the device may produce output	*/
	    /* for the netlist even if it has no declared ports.  This	*/
	    /* is irrespective of the presence of pin labels---if the	*/
	    /* symbol is "trivial", the netlist connections are		*/
	    /* resolved in the level of the hierarchy above, and there	*/
	    /* may be no ports, but the call list must retain the call	*/
	    /* to ensure that the netlist output is generated.		*/
	    /*----------------------------------------------------------*/

	    if (pschem->calls->ports == NULL)
               if (pschem->infolabels == FALSE)
	          removecall(pschem, pschem->calls);	/* Remove the call */
         }
      }
   }
}

/*----------------------------------------------------------------------*/
/* Translate a net number down in the calling hierarchy			*/
/*----------------------------------------------------------------------*/

int translatedown(int rnet, int portid, objectptr nextobj)
{
   PortlistPtr nport;
   int downnet = 0;

   for (nport = nextobj->ports; nport != NULL; nport = nport->next) {
      if (nport->portid == portid) {
	 downnet = nport->netid;
	 break;
      }
   }
   return downnet;
}

/*----------------------------------------------------------------------*/
/* Translate a netlist up in the calling hierarchy			*/
/*									*/
/* This routine creates a new netlist header which needs to be freed	*/
/* by the calling routine.						*/
/*									*/
/* Note that if the entire netlist cannot be translated up, then the	*/
/* routine returns NULL.  This could be modified to return the part of	*/
/* the network that can be translated up.				*/
/*----------------------------------------------------------------------*/

Genericlist *translateup(Genericlist *rlist, objectptr thisobj,
	objectptr nextobj, objinstptr nextinst)
{
   PortlistPtr nport;
   CalllistPtr ccall;
   int portid = 0;
   int upnet = 0;
   int rnet, lbus;
   buslist *sbus;
   Genericlist *tmplist;

   tmplist = (Genericlist *)malloc(sizeof(Genericlist));
   tmplist->subnets = 0;
   tmplist->net.id = 0;
   copy_bus(tmplist, rlist);

   for (lbus = 0;; ) {
      if (rlist->subnets == 0)
	 rnet = rlist->net.id;
      else {
	 sbus = rlist->net.list + lbus;
	 rnet = sbus->netid;
      }
      for (nport = nextobj->ports; nport != NULL; nport = nport->next) {
         if (nport->netid == rnet) {
	    portid = nport->portid;
	    break;
         }
      }

      upnet = 0;
      for (ccall = thisobj->calls; ccall != NULL; ccall = ccall->next) {
         if (ccall->callinst == nextinst) {
	    for (nport = ccall->ports; nport != NULL; nport = nport->next) {
	       if (nport->portid == portid) {
	          upnet = nport->netid;
	          break;
	       }
	    }
	    if (nport != NULL) break;
         }
      }
      if (upnet == 0) {
	 freegenlist(tmplist);
	 return NULL;
      }
      else {
	 if (tmplist->subnets == 0) {
	    tmplist->net.id = upnet;
	 }
	 else {
	    sbus = tmplist->net.list + lbus;
	    sbus->netid = upnet;
	    sbus->subnetid = getsubnet(upnet, thisobj);
	 }
      }
      if (++lbus >= rlist->subnets) break;
   }
   return tmplist;
}

/*----------------------------------------------------------------------*/
/* Check whether the indicated polygon is already resolved into the	*/
/* netlist of the object hierarchy described by seltop.			*/
/* Return the netlist if resolved, NULL otherwise.  The netlist		*/
/* returned is referred (translated) upward through the calling		*/
/* hierarchy to the topmost object containing that net or those nets.	*/
/* This topmost object is returned in parameter topobj.			*/
/* Note that the netlist returned does not necessarily correspond to	*/
/* any object in the top level.  It is allocated and must be freed by	*/
/* the calling routine.							*/
/*----------------------------------------------------------------------*/

Genericlist *is_resolved(genericptr *rgen, pushlistptr seltop, objectptr *topobj)
{
   objectptr thisobj = seltop->thisinst->thisobject;
   objectptr pschem;
   PolylistPtr pseek;
   LabellistPtr lseek;
   Genericlist *rlist = NULL, *newlist;

   pschem = (thisobj->schemtype == SECONDARY) ? thisobj->symschem : thisobj;

   /* Recursively call self, since we have to back out from the bottom of  */
   /* the stack.							   */

   if (seltop->next != NULL) {
      rlist = is_resolved(rgen, seltop->next, topobj);

      /* Translate network ID up the hierarchy to the topmost object in which */
      /* the network exists.						   */
   
      if (rlist != NULL) {
         newlist = translateup(rlist, pschem, seltop->next->thisinst->thisobject,
			seltop->next->thisinst);
         if (newlist == NULL)
            /* Net does not exist upwards of this object.  Pass net ID	*/
            /* upward with "topobj" unchanged.				*/
	    return rlist;
         else {
	    freegenlist(rlist);
	    rlist = newlist;
	 }
      }
   }
   else {

      /* Find the net ID for the listed object, which should be in the object */
      /* on the bottom of the pushlist stack.				   */

      if (IS_POLYGON(*rgen)) {
	 for (pseek = pschem->polygons; pseek != NULL; pseek = pseek->next) {
            if (pseek->poly == TOPOLY(rgen)) {
 	       rlist = (Genericlist *)pseek;
	       break;
	    }
	 }
      }
      else if (IS_LABEL(*rgen)) {
	 for (lseek = pschem->labels; lseek != NULL; lseek = lseek->next) {
            if (lseek->label == TOLABEL(rgen)) {
 	       rlist = (Genericlist *)lseek;
	       break;
	    }
	 }
      }

      if (rlist != NULL) {
         /* Make a copy of the netlist header from this element */
         newlist = (Genericlist *)malloc(sizeof(Genericlist));
         newlist->subnets = 0;
         copy_bus(newlist, rlist);
         rlist = newlist;
      }
   }

   /* done: (jdk) */
   *topobj = (rlist == NULL) ? NULL : seltop->thisinst->thisobject;
   return rlist;
}

/*--------------------------------------------------------------*/
/* Highlight all the polygons and pin labels in a network	*/
/* (recursively, downward).  Pin labels are drawn only on the	*/
/* topmost schematic object.					*/
/* Returns true if some part of the hierarchy declared a net to	*/
/* be highlighted.						*/
/* mode = 1 highlight, mode = 0 erase				*/
/*--------------------------------------------------------------*/

Boolean highlightnet(objectptr cschem, objinstptr cinst, int netid, u_char mode)
{
   CalllistPtr calls;
   PortlistPtr ports;
   PolylistPtr plist;
   LabellistPtr llist;
   polyptr cpoly;
   labelptr clabel;
   objinstptr ccinst;
   int netto, locnetid, lbus;
   int curcolor = AUXCOLOR;
   Boolean rval = FALSE;
   objectptr pschem;

   SetForeground(dpy, areawin->gc, curcolor);

   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   for (plist = pschem->polygons; plist != NULL; plist = plist->next) {
      if (plist->cschem != cschem) continue;
      cpoly = plist->poly;
      for (lbus = 0;;) {
	 if (plist->subnets == 0)
	    locnetid = plist->net.id;
	 else
	    locnetid = (plist->net.list + lbus)->netid;
	 if (locnetid == netid) {
	    /* Fprintf(stdout, " >> Found polygon belonging to net %d at (%d, %d)\n",
	       		locnetid, cpoly->points[0].x, cpoly->points[0].y); */
            if (mode == 0 && cpoly->color != curcolor) {
	       curcolor = cpoly->color;
	       XTopSetForeground(curcolor);
	    }
            UDrawPolygon(cpoly, xobjs.pagelist[areawin->page]->wirewidth);
	    break;
	 }
	 if (++lbus >= plist->subnets) break;
      }
   } 

   /* Highlight labels if they belong to the top-level object */

   if (cschem == topobject) {
      for (llist = pschem->labels; llist != NULL; llist = llist->next) {
	 if (llist->cschem != cschem) continue;
         else if ((llist->cinst != NULL) && (llist->cinst != cinst)) continue;
	 clabel = llist->label;
         for (lbus = 0;;) {
	    if (llist->subnets == 0)
	       locnetid = llist->net.id;
	    else
	       locnetid = (llist->net.list + lbus)->netid;
	    if (locnetid == netid) {
	       if (clabel->string->type == FONT_NAME) {  /* don't draw temp labels */
		  if ((mode == 0) && (clabel->color != curcolor)) {
		     curcolor = clabel->color;
	             UDrawString(clabel, curcolor, cinst);
		  }
		  else
	             UDrawString(clabel, DOFORALL, cinst);
	          /* Fprintf(stdout, " >> Found label belonging to net "
				"%d at (%d, %d)\n",
	       			locnetid, clabel->position.x,
				clabel->position.y); */
		     
	       }
	       break;
	    }
	    if (++lbus >= llist->subnets) break;
	 }
	 /* if we've encountered a unique instance, then continue	*/
	 /* past all other instances using this label.			*/
	 if (llist->cinst != NULL)
	    while (llist->next && (llist->next->label == llist->label))
	       llist = llist->next;
      }

      /* Highlight all pins connecting this net to symbols */
      
   }

   /* Connectivity recursion */

   for (calls = pschem->calls; calls != NULL; calls = calls->next) {
      if (calls->cschem != cschem) continue;
      for (ports = calls->ports; ports != NULL; ports = ports->next) {
         if (ports->netid == netid) {
	    ccinst = calls->callinst;

	    /* Recurse only on objects for which network polygons are visible	*/
	    /* from the calling object:  i.e., non-trivial, non-fundamental	*/
	    /* objects acting as their own schematics.			 	*/

	    UPushCTM();
	    UPreMultCTM(DCTM, ccinst->position, ccinst->scale, ccinst->rotation);

	    if (ccinst->thisobject->symschem == NULL &&
			ccinst->thisobject->schemtype != FUNDAMENTAL &&
			ccinst->thisobject->schemtype != TRIVIAL) {

	       netto = translatedown(netid, ports->portid, calls->callobj);

	       /* Fprintf(stdout, " > Calling object %s at (%d, %d)\n",
		    calls->callobj->name, ccinst->position.x, ccinst->position.y); */
	       /* Fprintf(stdout, " > Net translation from %d to %d (port %d)\n",
		    netid, netto, ports->portid); */

	       if (highlightnet(calls->callobj, calls->callinst, netto, mode))
		  rval = TRUE;
	    }
	    else {
	       /* Otherwise (symbols, fundamental, trivial, etc., objects), we	*/
	       /* highlight the pin position of the port.			*/
	       if ((clabel = PortToLabel(ccinst, ports->portid)))
		  UDrawXDown(clabel);
	    }
	    UPopCTM();
	 }
      }
   }
   return rval;
}

/*----------------------------------------------------------------------*/
/* Highlight whatever nets are listed in the current object instance,	*/
/* if any.								*/
/*----------------------------------------------------------------------*/

void highlightnetlist(objectptr nettop, objinstptr cinst, u_char mode)
{
   int lbus, netid;
   buslist *sbus;
   Genericlist *netlist = cinst->thisobject->highlight.netlist;
   objinstptr nextinst = cinst->thisobject->highlight.thisinst;

   if (netlist == NULL) return;

   for (lbus = 0;; ) {
      if (netlist->subnets == 0)
         netid = netlist->net.id;
      else {
         sbus = netlist->net.list + lbus;
         netid = sbus->netid;
      }
      highlightnet(nettop, nextinst, netid, mode);
      if (++lbus >= netlist->subnets) break;
   }

   /* If we are erasing, remove the netlist entry from the object */
   if (mode == (u_char)0) {
      freegenlist(netlist);
      cinst->thisobject->highlight.netlist = NULL;
      cinst->thisobject->highlight.thisinst = NULL;
   }
}

/*----------------------------------------------------------------------*/
/* Push the matrix stack for each object (instance) until the indicated	*/
/* object is reached.  This works similarly to highlightnet() above, 	*/
/* but makes calls according to the hierarchy described by the 		*/
/* pushlistptr parameter.						*/
/* Returns the number of stack objects to pop after we're done.		*/
/*----------------------------------------------------------------------*/

int pushnetwork(pushlistptr seltop, objectptr nettop)
{
   pushlistptr cursel = seltop;
   objinstptr sinst;
   int rno = 0;

   while ((cursel->thisinst->thisobject != nettop) && (cursel->next != NULL)) {
      cursel = cursel->next;
      sinst = cursel->thisinst;
      UPushCTM();
      UPreMultCTM(DCTM, sinst->position, sinst->scale, sinst->rotation);
      rno++;
   }

   if (cursel->thisinst->thisobject != nettop) {
      Fprintf(stderr, "Error:  object does not exist in calling stack!\n");
      rno = 0;
   }

   return rno;
}

/*----------------------------------------------------------------------*/
/* Determine if two netlists match.  If "mode" is MATCH_EXACT, the	*/
/* net numbers and subnet numbers must all be the same.  If		*/
/* MATCH_SUBNETS, then the subnet numbers must be the same.  If		*/
/* MATCH_SIZE, then they need only have the same number of subnets.	*/
/*----------------------------------------------------------------------*/

Boolean match_buses(Genericlist *list1, Genericlist *list2, int mode)
{
   int i;
   buslist *bus1, *bus2;

   if (list1->subnets != list2->subnets) {
      // A wire (no subnets) matches a bus of 1 subnet.  All others
      // are non-matching.

      if (list1->subnets != 0 && list2->subnets != 0)
         return FALSE;
      else if (list1->subnets != 1 && list2->subnets != 1)
	 return FALSE;
   }

   if (mode == MATCH_SIZE) return TRUE;

   if (list1->subnets == 0) {
      if (mode == MATCH_SUBNETS) return TRUE;
      if (list2->subnets != 0) {
	 bus2 = list2->net.list + 0;
	 if (list1->net.id != bus2->netid) return FALSE;
      }
      else if (list1->net.id != list2->net.id) return FALSE;
   }
   else if (list2->subnets == 0) {
      if (mode == MATCH_SUBNETS) return TRUE;
      bus1 = list1->net.list + 0;
      if (bus1->netid != list2->net.id) return FALSE;
   }
   else {
      for (i = 0; i < list1->subnets; i++) {
         bus1 = list1->net.list + i;
         bus2 = list2->net.list + i;
	 /* A subnetid of < 0 indicates an unassigned bus */
         if ((bus1->subnetid != -1) && (bus1->subnetid != bus2->subnetid))
	    return FALSE;
      }
      if (mode == MATCH_SUBNETS) return TRUE;

      for (i = 0; i < list1->subnets; i++) {
         bus1 = list1->net.list + i;
         bus2 = list2->net.list + i;
         if (bus1->netid != bus2->netid)
	    return FALSE;
      }
   }
   return TRUE;
}

/*----------------------------------------------------------------------*/
/* Copy the netlist structure from one netlist element to another	*/
/*----------------------------------------------------------------------*/

void copy_bus(Genericlist *dest, Genericlist *source)
{
   buslist *sbus, *dbus;
   int i;

   if (dest->subnets > 0)
      free(dest->net.list);

   dest->subnets = source->subnets;
   if (source->subnets == 0)
      dest->net.id = source->net.id;
   else {
      dest->net.list = (buslist *)malloc(dest->subnets * sizeof(buslist));
      for (i = 0; i < dest->subnets; i++) {
	 sbus = source->net.list + i;
	 dbus = dest->net.list + i;
	 dbus->netid = sbus->netid;
	 dbus->subnetid = sbus->subnetid;
      }
   }
}

/*------------------------------------------------------*/
/* Create a new "temporary" label object for a pin	*/
/* This type of label is never drawn, so it doesn't	*/
/* need font info.  It is identified as "temporary" by	*/
/* this lack of a leading font record.			*/
/*------------------------------------------------------*/

Genericlist *new_tmp_pin(objectptr cschem, XPoint *pinpt, char *pinstring,
		char *prefix, Genericlist *netlist)
{
   labelptr *newlabel;
   stringpart *strptr;

   if (pinpt == NULL) {
      Fprintf(stderr, "NULL label location!\n");
      return NULL;
   }

   NEW_LABEL(newlabel, cschem);
   labeldefaults(*newlabel, LOCAL, pinpt->x, pinpt->y);
   (*newlabel)->anchor = 0;
   (*newlabel)->color = DEFAULTCOLOR;
   strptr = (*newlabel)->string;
   strptr->type = TEXT_STRING;
   if (pinstring != NULL) {
      strptr->data.string = (char *)malloc(strlen(pinstring));
      strcpy(strptr->data.string, pinstring);
   }
   else {
      strptr->data.string = textprintnet(prefix, NULL, netlist);
   }

   /* Add label to object's netlist and return a pointer to the */
   /* netlist entry.						*/

   return (addpin(cschem, NULL, *newlabel, netlist));
}

/*------------------------------------------------------*/
/* Create a label for use in the list of global nets.	*/
/* This label is like a temporary label (new_tmp_pin) 	*/
/* except that it is not represented in any object.	*/
/* The string contains the verbatim contents of any	*/
/* parameter substitutions in the original label.	*/
/*------------------------------------------------------*/

labelptr new_global_pin(labelptr clabel, objinstptr cinst)
{
   labelptr newlabel;

   newlabel = (labelptr) malloc(sizeof(label));
   newlabel->type = LABEL;
   labeldefaults(newlabel, GLOBAL, 0, 0);
   newlabel->anchor = 0;
   newlabel->color = DEFAULTCOLOR;
   free(newlabel->string);
   newlabel->string = stringcopyall(clabel->string, cinst);

   /* Add label to the global netlist and return a pointer to	*/
   /* the netlist entry.					*/

   return newlabel;
}

/*----------------------------------------------------------------------*/
/* Create a temporary I/O pin (becomes part of netlist and also part of	*/
/* the object itself).							*/
/*----------------------------------------------------------------------*/

Genericlist *make_tmp_pin(objectptr cschem, objinstptr cinst, XPoint *pinpt,
		Genericlist *sublist)
{
   LabellistPtr lseek;
   objectptr pschem;
   char *pinstring = NULL;
   /* buslist *sbus; (jdk) */
   /* int lbus; (jdk) */
   Genericlist *netlist, *tmplist, newlist;

   newlist.subnets = 0;
   newlist.net.id = 0;

   /* Primary schematic (contains the netlist) */
   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   /* Determine a netlist for this pin */

   netlist = pointtonet(cschem, cinst, pinpt);
   if (netlist == NULL) {
      newlist.net.id = netmax(pschem) + 1;
      netlist = &newlist;
   }

   /* If there is any other pin at this location, don't	make another	*/
   /* one.  If there is already a temporary pin associated with the	*/
   /* net, use its name.						*/

   else {
      for (lseek = pschem->labels; lseek != NULL; lseek = lseek->next) {
	 if (lseek->cschem != cschem) continue;
	 else if ((lseek->cinst != NULL) && (lseek->cinst != cinst)) continue;
	 tmplist = (Genericlist *)lseek;
	 if (match_buses(netlist, tmplist, MATCH_EXACT)) {
	    if (proximity(&(lseek->label->position), pinpt))
	       return (Genericlist *)lseek;
	    else if (lseek->label->string->type == TEXT_STRING)
	       pinstring = lseek->label->string->data.string;
	 }
	 /* if we've encountered a unique instance, then continue past	*/
	 /* all other instances using this label.			*/
	 if (lseek->cinst != NULL)
	    while (lseek->next && (lseek->next->label == lseek->label))
	       lseek = lseek->next;
      }
   }
   return (new_tmp_pin(cschem, pinpt, pinstring, "ext", netlist));
}

/*--------------------------------------------------------------*/
/* Search for connections into a non-symbol subcircuit, based	*/
/* on various combinations of polygon and pin label overlaps.	*/
/*--------------------------------------------------------------*/

int searchconnect(XPoint *points, int number, objinstptr cinst, int subnets)
{
   XPoint *tmppts, *tpt, *tpt2, *endpt, *endpt2, *pinpt, opinpt;
   objinstptr tinst;
   genericptr *cgen;
   polyptr tpoly;
   labelptr tlab;
   objectptr tobj, cobj = cinst->thisobject;
   LabellistPtr tseek;
   PolylistPtr pseek;
   int i; /* , lbus, sub_bus; (jdk) */
   int found = 0;

   /* Generate temporary polygon in the coordinate system of	*/
   /* the object instance in question			 	*/

   tmppts = (XPoint *)malloc(number * sizeof(XPoint));
   InvTransformPoints(points, tmppts, number,
		cinst->position, cinst->scale, cinst->rotation);
   /* Fprintf(stdout, "Info: translated polygon w.r.t. object %s\n",	*/
   /* 		cinst->thisobject->name);				*/

   /* Recursion on all appropriate sub-schematics. */
   /* (Use parts list, not call list, as call list may not have created yet) */

   for (i = 0; i < cobj->parts; i++) {
      cgen = cobj->plist + i;
      if (IS_OBJINST(*cgen)) {
	 tinst = TOOBJINST(cgen);
	 if (tinst->thisobject->symschem == NULL) {
            tobj = tinst->thisobject;
	    if (tobj->schemtype != FUNDAMENTAL && tobj->schemtype != TRIVIAL)
	       found += searchconnect(tmppts, number, tinst, subnets);
	 }
      }
   }

   for (endpt = tmppts; endpt < tmppts + EndPoint(number); endpt++) {
      endpt2 = endpt + NextPoint(number);
      for (i = 0; i < cobj->parts; i++) {
	 cgen = cobj->plist + i;
         if (!IS_OBJINST(*cgen)) continue;
	 tinst = TOOBJINST(cgen);

	 /* Look at the object only (symbol, or schematic if it has no symbol) */
         tobj = tinst->thisobject;

	 /* Search for connections to pin labels */

	 for (tseek = tobj->labels; tseek != NULL; tseek = tseek->next) {
	    tlab = tseek->label;
	    UTransformPoints(&(tlab->position), &opinpt, 1, tinst->position,
				tinst->scale, tinst->rotation);
	    if (onsegment(endpt2, endpt, &opinpt)) {
	       /* Fprintf(stdout, "%s connects to pin %s of %s in %s\n",	*/
	       /*	((number > 1) ? "Polygon" : "Pin"),			*/
	       /*	tlab->string + 2, tinst->thisobject->name,	*/
	       /* 	cinst->thisobject->name);		*/
	       make_tmp_pin(cobj, cinst, &opinpt, (Genericlist *)tseek);
	       found += (tseek->subnets == 0) ? 1 : tseek->subnets;
	       break;
	    }
	 }
      }

      for (pseek = cobj->polygons; pseek != NULL; pseek = pseek->next) {
	 tpoly = pseek->poly;

	 /* Search for connections from segments passed to this	*/
	 /* function to endpoints of polygons in the netlist.	*/

	 pinpt = NULL;
	 tpt = tpoly->points;
	 tpt2 = tpoly->points + tpoly->number - 1;
	 if (onsegment(endpt2, endpt, tpt)) pinpt = tpt;
	 if (onsegment(endpt2, endpt, tpt2)) pinpt = tpt2;

	 /* Create new pinlabel (only if there is not one already there) */

	 if (pinpt != NULL) {
	    make_tmp_pin(cobj, cinst, pinpt, (Genericlist *)pseek);
	    found += (pseek->subnets == 0) ? 1 : pseek->subnets;
	 }
      }
   }

   endpt = tmppts;
   endpt2 = tmppts + EndPoint(number) - 1;

   /* Search for connections from endpoints passed to this	*/
   /* function to segments of polygons in the netlist.		*/

   for (pseek = cobj->polygons; pseek != NULL; pseek = pseek->next) {

      tpoly = pseek->poly;
      for (tpt = tpoly->points; tpt < tpoly->points
			+ EndPoint(tpoly->number); tpt++) {
	 tpt2 = tpt + NextPoint(tpoly->number);

  	 pinpt = NULL;
	 if (onsegment(tpt2, tpt, endpt)) pinpt = endpt;
	 if (onsegment(tpt2, tpt, endpt2)) pinpt = endpt2;

	 /* Create new pinlabel (only if there is not one already there) */

	 if (pinpt != NULL) {
	    make_tmp_pin(cobj, cinst, pinpt, (Genericlist *)pseek);
	    found += (pseek->subnets == 0) ? 1 : pseek->subnets;
	 }
      }
   }
   free(tmppts);
   return(found);
}

/*----------------------------------------------------------------------*/
/* Associate polygon with given netlist and add to the object's list	*/
/* of network polygons (i.e., wires).					*/
/*----------------------------------------------------------------------*/

Genericlist *addpoly(objectptr cschem, polyptr poly, Genericlist *netlist)
{
   PolylistPtr newpoly;
   objectptr pschem;
   /* buslist *sbus; (jdk) */
   /* int lbus, sub_bus; (jdk) */

   /* Netlist is in the master schematic */
   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   /* If this polygon is already in the list, then add an extra subnet	*/
   /* if necessary.							*/

   for (newpoly = pschem->polygons; newpoly != NULL; newpoly = newpoly->next) {
      if (newpoly->poly == poly) {
	 if (!match_buses((Genericlist *)newpoly, netlist, MATCH_EXACT)) {
	    Fprintf(stderr, "addpoly:  Error in bus assignment\n");
	    return NULL;
	 }
	 return (Genericlist *)newpoly;
      }
   }

   /* Create a new entry and link to polygon list of this object */

   newpoly = (PolylistPtr) malloc(sizeof(Polylist));
   newpoly->cschem = cschem;
   newpoly->poly = poly;
   newpoly->subnets = 0;
   copy_bus((Genericlist *)newpoly, netlist);
   newpoly->next = pschem->polygons;
   pschem->polygons = newpoly;

   return (Genericlist *)newpoly;
}

/*-------------------------------------------------------------------------*/

long zsign(long a, long b)
{
   if (a > b) return 1;
   else if (a < b) return -1;
   else return 0; 
}

/*----------------------------------------------------------------------*/
/* Promote a single net to a bus.  The bus size will be equal to the	*/
/* value "subnets".							*/
/*----------------------------------------------------------------------*/

void promote_net(objectptr cschem, Genericlist *netfrom, int subnets)
{
   Genericlist *netref = NULL;
   CalllistPtr calls;
   PortlistPtr ports;
   PolylistPtr plist;
   LabellistPtr llist;
   int netid, firstid, lbus; /* curid, (jdk) */ 
   buslist *sbus;
   Boolean foundlabel;

   /* If no promotion is required, don't do anything */
   if (netfrom->subnets == subnets) return;

   /* It "netfrom" is already a bus, but of different size than	*/
   /* subnets, then it cannot be changed.			*/

   if (netfrom->subnets != 0) {
      Fprintf(stderr, "Attempt to change the size of a bus!\n");
      return;
   } 

   netid = netfrom->net.id;

   /* If "subnets" is 1, then "netfrom" can be promoted regardless.	*/
   /* Otherwise, if the "netfrom" net is used in any calls, then it	*/
   /* cannot be promoted.						*/

   if (subnets > 1) {
      for (calls = cschem->calls; calls != NULL; calls = calls->next)
         for (ports = calls->ports; ports != NULL; ports = ports->next)
            if (ports->netid == netid) {
	       Fprintf(stderr, "Cannot promote net to bus: Net already connected"
			" to single-wire port\n");
	       return;
	    }
      firstid = netmax(cschem) + 1;
   }

   for (plist = cschem->polygons; plist != NULL; plist = plist->next)
      if ((plist->subnets == 0) && (plist->net.id == netid)) {
	 plist->subnets = subnets;
         plist->net.list = (buslist *)malloc(subnets * sizeof(buslist));
	 for (lbus = 0; lbus < subnets; lbus++) {
	    sbus = plist->net.list + lbus;
	    sbus->netid = (lbus == 0) ? netid : firstid + lbus;
	    sbus->subnetid = lbus; 	/* By default, number from zero */
	 }
	 netref = (Genericlist *)plist;
      }

   /* It's possible for a label without bus notation to be attached	*/
   /* to this net.							*/

   foundlabel = FALSE;
   for (llist = cschem->labels; llist != NULL; llist = llist->next)
      if ((llist->subnets == 0) && (llist->net.id == netid)) {
	 llist->subnets = subnets;
         llist->net.list = (buslist *)malloc(subnets * sizeof(buslist));
	 for (lbus = 0; lbus < subnets; lbus++) {
	    sbus = llist->net.list + lbus;
	    sbus->netid = (lbus == 0) ? netid : firstid + lbus;
	    sbus->subnetid = lbus; 	/* By default, number from zero */
	 }
	 netref = (Genericlist *)llist;
	 foundlabel = TRUE;
      }

   /* We need to create a temp label associated with this net to 	*/
   /* encompass the promoted bus size.  If this bus is later attached	*/
   /* to a known bus, that bus name will be canonical.  If no bus name	*/
   /* is ever assigned to the bus, this temp one will be used.		*/

   if (!foundlabel) {
      XPoint *pinpos;
      pinpos = NetToPosition(netid, cschem);
      new_tmp_pin(cschem, pinpos, NULL, "int", netref);
   }
}

/*----------------------------------------------------------------------*/
/* Change any part of the netlist "testlist" that matches the net IDs	*/
/* of "orignet" to the net IDs of "newnet".   It is assumed that the	*/
/* lengths and sub-bus numbers of "orignet" and "newnet" match.		*/
/*----------------------------------------------------------------------*/

Boolean mergenetlist(objectptr cschem, Genericlist *testlist,
		Genericlist *orignet, Genericlist *newnet)
{
   int obus, onetid, osub, nsub, nnetid, tbus;
   buslist *sbus;
   Boolean rval = FALSE;

   for (obus = 0;;) {
      if (orignet->subnets == 0) {
	 onetid = orignet->net.id;
	 osub = -1;
      }
      else {
	 sbus = orignet->net.list + obus;
	 onetid = sbus->netid;
	 osub = sbus->subnetid;
      }

      if (newnet->subnets == 0) {
	 nnetid = newnet->net.id;
	 nsub = -1;
      }
      else {
	 sbus = newnet->net.list + obus;
	 nnetid = sbus->netid;
	 nsub = sbus->subnetid;
      }

      if (testlist->subnets == 0) {
	 if (testlist->net.id == onetid) {
	    rval = TRUE;
	    if (orignet->subnets == 0) {
	       testlist->net.id = nnetid;
	       return TRUE;
	    }
	    else {
	       /* Promote testlist to a bus subnet of size 1 */
	       testlist->subnets = 1;
	       testlist->net.list = (buslist *)malloc(sizeof(buslist));
	       sbus = testlist->net.list;
	       sbus->netid = nnetid;
	       sbus->subnetid = nsub;
	       return rval;
	    }
	 }
      }

      for (tbus = 0; tbus < testlist->subnets; tbus++) {
	 /* If the sub-bus numbers match, then change the net	*/
	 /* ID to the new net number.  If the sub-bus is	*/
	 /* unnamed (has no associated bus-notation label),	*/
	 /* then it can take on the net and subnet numbers.	*/
	
	 sbus = testlist->net.list + tbus;

	 if (sbus->netid == onetid) {
	    if (sbus->subnetid == osub) {
	       sbus->netid = nnetid; 
	       sbus->subnetid = nsub;
	       rval = TRUE;
	    }
	    else {
	       labelptr blab = NetToLabel(nnetid, cschem);
	       if (blab == NULL) {
		  Fprintf(stderr, "Warning: isolated subnet?\n"); 
		  sbus->netid = nnetid;
		  /* Keep subnetid---but, does newnet need to be promoted? */
	          return TRUE;
	       }
	       else if (blab->string->type != FONT_NAME) {
	          sbus->netid = nnetid; 
	          sbus->subnetid = nsub;
	          rval = TRUE;
		  Fprintf(stderr, "Warning: Unexpected subnet value in mergenetlist!\n"); 
	       }
	    }
	 }
      }
      if (++obus >= orignet->subnets) break;
   }
   return rval;
}

/*----------------------------------------------------------------------*/
/* Combine two networks in an object's linked-list Netlist 		*/
/* Parameters: cschem - pointer to object containing netlist		*/
/*	       orignet - original netlist to be changed			*/
/*	       newnet - new netlist to be changed to			*/	
/*									*/
/* Attempts to merge different subnets in a bus are thwarted.		*/
/*----------------------------------------------------------------------*/

Boolean netmerge(objectptr cschem, Genericlist *orignet, Genericlist *newnet)
{
   PolylistPtr plist;
   LabellistPtr llist;
   CalllistPtr calls;
   PortlistPtr ports;
   Genericlist savenet;
   int i, net;
   buslist *obus, *nbus;
   Boolean rval;

   /* Trivial case; do nothing */
   if (match_buses(orignet, newnet, MATCH_EXACT)) return TRUE;

   /* Disallow an attempt to convert a global net to a local net: */
   /* The global net ID always dominates!			  */

   if ((orignet->subnets == 0) && (newnet->subnets == 0) &&
	(orignet->net.id < 0) && (newnet->net.id > 0)) {
      int globnet = orignet->net.id;
      orignet->net.id = newnet->net.id;
      newnet->net.id = globnet;
   }

   /* Check that the lists of changes are compatible.  It appears to be	*/
   /* okay to have non-matching buses (although they should not be	*/
   /* merged), as this is a valid style in which buses do not have taps	*/
   /* but the sub-buses are explicitly called out with labels.  In such	*/
   /* case, the polygons of different sub-buses touch, and this routine	*/
   /* rejects them as being incompatible.				*/

   if (!match_buses(orignet, newnet, MATCH_SUBNETS)) {
      if (match_buses(orignet, newnet, MATCH_SIZE)) {
	 labelptr nlab;
	 /* If only the subnet numbers don't match up, check if	*/
	 /* "orignet" has a temp label.				*/
	 nbus = orignet->net.list;
	 if ((nlab = NetToLabel(nbus->netid, cschem)) != NULL) {
	    if (nlab->string->type != FONT_NAME)
	       goto can_merge;
	 }
      }
      else
         Fprintf(stderr, "netmerge warning: non-matching bus subnets touching.\n");
      return FALSE;
   }

can_merge:

   /* If orignet is a bus size 1 and newnet is a wire, then promote	*/
   /* newnet to a bus size 1.						*/

   if (orignet->subnets == 1 && newnet->subnets == 0) {
      net = newnet->net.id;
      newnet->subnets = 1;
      newnet->net.list = (buslist *)malloc(sizeof(buslist));
      obus = orignet->net.list;
      nbus = newnet->net.list;
      nbus->subnetid = obus->subnetid;
      nbus->netid = net;
   }

   /* Make a copy of the net so we don't overwrite it */
   savenet.subnets = 0;
   copy_bus(&savenet, orignet);

   rval = FALSE;
   for (plist = cschem->polygons; plist != NULL; plist = plist->next)
      if (mergenetlist(cschem, (Genericlist *)plist, &savenet, newnet))
	 rval = TRUE;

   for (llist = cschem->labels; llist != NULL; llist = llist->next)
      if (mergenetlist(cschem, (Genericlist *)llist, &savenet, newnet)) {
	 int pinnet;
	 char *newtext;
	 rval = TRUE;

	 /* Because nets that have been merged away may be re-used later, */
	 /* change the name of any temporary labels to the new net number */

	 if (llist->label->string->type != FONT_NAME) {
	    newtext = llist->label->string->data.string;
	    if (sscanf(newtext + 3, "%d", &pinnet) == 1) {
	       if (pinnet == savenet.net.id) {
		  *(newtext + 3) = '\0';
		  llist->label->string->data.string = textprintnet(newtext,
				NULL, newnet);
		  free(newtext);
	       }
	    }
	 }
      }

   if (rval) {

      /* Reflect the net change in the object's call list, if it has one. */

      for (calls = cschem->calls; calls != NULL; calls = calls->next) {
         for (ports = calls->ports; ports != NULL; ports = ports->next) {
            if (newnet->subnets == 0) {
               if (ports->netid == savenet.net.id)
	          ports->netid = newnet->net.id;
	    }
	    else {
               for (i = 0; i < newnet->subnets; i++) {
	          obus = savenet.net.list + i;
	          nbus = newnet->net.list + i;
                  if (ports->netid == obus->netid)
	             ports->netid = nbus->netid;
	       }
	    }
	 }
      }
   }

   /* Free the copy of the bus that we made, if we made one */
   if (savenet.subnets > 0) free(savenet.net.list);
  
   return rval;
}

/*----------------------------------------------------------------------*/
/* Wrapper to netmerge() to make sure change is made both to the symbol	*/
/* and schematic, if both exist.					*/
/*----------------------------------------------------------------------*/

Boolean mergenets(objectptr cschem, Genericlist *orignet, Genericlist *newnet)
{
   Boolean merged;

   if (cschem->symschem != NULL)
      merged = netmerge(cschem->symschem, orignet, newnet);
   if (netmerge(cschem, orignet, newnet))
      merged = TRUE;
 
   return merged;
}

/*----------------------------------------------------------------------*/
/* Remove a call to an object instance from the call list of cschem	*/
/*----------------------------------------------------------------------*/

void removecall(objectptr cschem, CalllistPtr dontcallme)
{
   CalllistPtr lastcall, seeklist;
   PortlistPtr ports, savelist;

   /* find the instance call before this one and link it to the one following */

   lastcall = NULL;
   for (seeklist = cschem->calls; seeklist != NULL; seeklist = seeklist->next) {
      if (seeklist == dontcallme)
	 break;
      lastcall = seeklist;
   }

   if (seeklist == NULL) {
      Fprintf(stderr, "Error in removecall():  Call does not exist!\n");
      return;
   }

   if (lastcall == NULL)
      cschem->calls = dontcallme->next;
   else
      lastcall->next = dontcallme->next;

   ports = dontcallme->ports;
   while (ports != NULL) {
      savelist = ports;
      ports = ports->next;
      free (savelist);
   }
   free(dontcallme);
}

/*----------------------------------------------------------------------*/
/* Add a pin label to the netlist					*/
/* If cschem == NULL, add the pin to the list of global pins.		*/
/* The new label entry in the netlist gets a copy of the netlist	*/
/* "netlist".								*/
/*----------------------------------------------------------------------*/

Genericlist *addpin(objectptr cschem, objinstptr cinst, labelptr pin,
		Genericlist *netlist)
{
   LabellistPtr srchlab, newlabel, lastlabel = NULL;
   objectptr pschem;
   /* buslist *sbus; (jdk) */
   /* int lbus, sub_bus; (jdk) */

   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   for (srchlab = pschem->labels; srchlab != NULL; srchlab = srchlab->next) {
      if (srchlab->label == pin) {
	 if (!match_buses(netlist, (Genericlist *)srchlab, MATCH_EXACT)) {
	    if (srchlab->cinst == cinst) {
	       Fprintf(stderr, "addpin: Error in bus assignment\n");
	       return NULL;
	    }
	 }
	 else if (srchlab->cinst == NULL)
	    return (Genericlist *)srchlab;
	 break;		/* Stop at the first record for this label */
      }
      lastlabel = srchlab;
   }

   /* Create a new entry and link to label list of the object */
	 
   newlabel = (LabellistPtr) malloc(sizeof(Labellist));
   newlabel->cschem = cschem;
   newlabel->cinst = cinst;
   newlabel->label = pin;
   newlabel->subnets = 0;
   copy_bus((Genericlist *)newlabel, netlist);

   /* Always put the specific (instanced) cases	in front of	*/
   /* generic cases for the same label.				*/

   /* Generic case---make it the last record for this label */
   if ((cinst == NULL) && (lastlabel != NULL)) {
      while ((srchlab != NULL) && (srchlab->label == pin)) {
	 lastlabel = srchlab;
	 srchlab = srchlab->next;
      }
   }
   if (lastlabel != NULL) {
      newlabel->next = srchlab;
      lastlabel->next = newlabel;
   }
   else {
      newlabel->next = pschem->labels;
      pschem->labels = newlabel;
   }
   return (Genericlist *)newlabel;
}
		
/*----------------------------------------------------------------------*/
/* Add a pin label to the list of global net names.			*/
/* The new label entry in the netlist gets a copy of the netlist	*/
/* "netlist" and a copy of label "pin" containing the *instance* value	*/
/* of the text.								*/
/*----------------------------------------------------------------------*/

Genericlist *addglobalpin(objectptr cschem, objinstptr cinst, labelptr pin,
	Genericlist *netlist)
{
   LabellistPtr srchlab, newlabel, lastlabel = NULL;

   if (cinst == NULL) {
      Fprintf(stderr, "Error:  Global pin does not have an associated instance!\n");
      return NULL;
   }

   for (srchlab = global_labels; srchlab != NULL; srchlab = srchlab->next) {
      if (srchlab->label == pin) {
	 if (!match_buses(netlist, (Genericlist *)srchlab, MATCH_EXACT)) {
	    if (srchlab->cinst == cinst) {
	       Fprintf(stderr, "addglobalpin: Error in bus assignment\n");
	       return NULL;
	    }
	 }
	 else if (srchlab->cinst == NULL)
	    return (Genericlist *)srchlab;
	 break;		/* Stop at the first record for this label */
      }
      lastlabel = srchlab;
   }

   /* Create a new entry and link to label list of the object */
	 
   newlabel = (LabellistPtr) malloc(sizeof(Labellist));
   newlabel->cschem = cschem;
   newlabel->cinst = cinst;
   newlabel->label = new_global_pin(pin, cinst);
   newlabel->subnets = 0;
   copy_bus((Genericlist *)newlabel, netlist);

   if (lastlabel != NULL) {
      newlabel->next = srchlab;
      lastlabel->next = newlabel;
   }
   else {
      newlabel->next = global_labels;
      global_labels = newlabel;
   }
   return (Genericlist *)newlabel;
}
		
/*----------------------------------------------------------------------*/
/* Allocate memory for the new call list element             		*/
/* Define the values for the new call list element		   	*/
/* Insert new call into call list					*/
/* The new call is at the beginning of the list.			*/
/*----------------------------------------------------------------------*/

void addcall(objectptr cschem, objectptr callobj, objinstptr callinst)
{
   CalllistPtr newcall;
   objectptr pschem;

   /* Netlist is on the master schematic */
   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   newcall = (CalllistPtr) malloc(sizeof(Calllist));
   newcall->cschem = cschem;
   newcall->callobj = callobj;
   newcall->callinst = callinst;
   newcall->devindex = -1;
   newcall->devname = NULL;
   newcall->ports = NULL;
   newcall->next = pschem->calls;
   pschem->calls = newcall;
}
		
/*----------------------------------------------------------------------*/
/* Add a port to the object cschem which connects net "netto" to	*/
/* the calling object. 	One port is created for each net ID in "netto"	*/
/* (which may be a bus).  The port contains the net ID in the called	*/
/* object.  The port may already exist, in which case this routine does	*/
/* nothing.								*/
/*----------------------------------------------------------------------*/

void addport(objectptr cschem, Genericlist *netto)
{
   PortlistPtr newport, seekport;
   int portid = 0, netid, lbus;
   buslist *sbus;
   Boolean duplicate;

   for (lbus = 0;;) {

      if (netto->subnets == 0)
	 netid = netto->net.id;
      else {
	 sbus = netto->net.list + lbus;
	 netid = sbus->netid;
      }
   
      /* If a port already exists for this net, don't add another one! */

      duplicate = FALSE;
      for (seekport = cschem->ports; seekport != NULL; seekport = seekport->next) {
	 if (seekport->netid != netid) {
	    if (seekport->portid > portid)
	       portid = seekport->portid;
	 }
	 else
	    duplicate = TRUE;
      }

      if (!duplicate) {
	 portid++;

	 newport = (PortlistPtr)malloc(sizeof(Portlist));
	 newport->netid = netid;
	 newport->portid = portid;

	 if (cschem->ports != NULL)
	    newport->next = cschem->ports;
	 else
	    newport->next = NULL;

	 cschem->ports = newport;
      }
      if (++lbus >= netto->subnets) break;
   }
}

/*----------------------------------------------------------------------*/
/* Add a specific port connection from object cschem into the instance	*/
/* cinst.  This equates a net number in the calling object cschem 	*/
/* (netfrom) to a net number in the object instance being called	*/
/* (netto).								*/
/*									*/
/* If we attempt to connect a bus of one size to a port of a different	*/
/* size, return FALSE.  Otherwise, add the call and return TRUE.	*/
/*----------------------------------------------------------------------*/

Boolean addportcall(objectptr cschem, Genericlist *netfrom, Genericlist *netto)
{
   CalllistPtr ccall;
   PortlistPtr seekport, sp, newport;
   objectptr instobj;
   objinstptr cinst;
   int lbus, netid_from, netid_to;
   buslist *sbus, *tbus;
   Boolean duplicate;

   /* The call that we need to add a port to is the first on	*/
   /* the list for cschem, as created by addcall().		*/
   ccall = cschem->calls;
   instobj = ccall->callobj;
   cinst = ccall->callinst;

   if (netfrom->subnets != netto->subnets) {
      if (netfrom->subnets == 0) {
	 /* It is possible that "netfrom" is an unlabeled polygon that	*/
	 /* is implicitly declared a bus by its connection to this	*/
	 /* port.  If so, we promote "netfrom" to a bus but set the	*/
	 /* subnet entries to negative values, since we don't know what	*/
	 /* they are yet.						*/
	 promote_net(cschem, netfrom, netto->subnets);
      }

      /* Only other allowable condition is a bus size 1 connecting into	*/
      /* a single-wire port.  However, one should consider promotion of	*/
      /* the pin in the target object to a bus on a per-instance basis.	*/
      /* This has not yet been done. . .				*/

      else if ((netfrom->subnets != 1) || (netto->subnets != 0)) {
	 /* Let the caller report error information, because it knows more */
	 return FALSE;
      }
   }

   for (lbus = 0;;) {
      buslist bsingf, bsingo;
      Genericlist subnet_from, subnet_other;

      if (netfrom->subnets == 0) {
	 netid_from = netfrom->net.id;

	 subnet_from.subnets = 0;
	 subnet_from.net.id = netid_from;

	 subnet_other.subnets = 0;
      }
      else {
	 sbus = netfrom->net.list + lbus;
	 netid_from = sbus->netid;

	 subnet_from.subnets = 1;
	 subnet_from.net.list = &bsingf;
	 bsingf.netid = netid_from;
	 bsingf.subnetid = sbus->subnetid;

	 bsingo.subnetid = lbus;
	 subnet_other.subnets = 1;
	 subnet_other.net.list = &bsingo;
      }
      if (netto->subnets == 0) {
	 netid_to = netto->net.id;
      }
      else {
	 tbus = netto->net.list + lbus;
	 netid_to = tbus->netid;
      }

      /* Check the ports of the instance's object for the one matching	*/
      /* the "netto" net ID.						*/

      duplicate = FALSE;
      for (seekport = instobj->ports; seekport != NULL;
		seekport = seekport->next) {
         if (seekport->netid == netid_to) {

	    /* If there is already an entry for this port, then	*/
	    /* we may need to merge nets in cschem.		*/

	    for (sp = ccall->ports; sp != NULL; sp = sp->next)
	       if (sp->portid == seekport->portid) {
	          if (sp->netid != netid_from) {
		     if (netfrom->subnets == 0)
		        subnet_other.net.id = sp->netid;
		     else {
			bsingo.netid = sp->netid;
		        bsingo.subnetid = getsubnet(bsingo.netid, cschem);
		     }
		     if (!mergenets(cschem, &subnet_other, &subnet_from)) {
			/* Upon failure, see if we can merge the other	*/
			/* direction.					*/
			if (!mergenets(cschem, &subnet_from, &subnet_other))
			   continue;
			else {
			   if (subnet_from.subnets == 0)
			      subnet_from.net.id = subnet_other.net.id;
			   else {
			      bsingf.netid = bsingo.netid;
			      bsingf.subnetid = bsingo.subnetid;
			   }
			}
		     }
	          }
		  duplicate = TRUE;
	       }

	    if (!duplicate) {
	       newport = (PortlistPtr)malloc(sizeof(Portlist));
	       newport->netid = netid_from;
	       newport->portid = seekport->portid;
	       newport->next = ccall->ports;
	       ccall->ports = newport;
	    }
	    break;
	 }
      }
      if (++lbus >= netfrom->subnets) break;
   }
   return TRUE;
}

/*----------------------------------------------------------------------*/
/* Find the net ID corresponding to the indicated port ID in the 	*/
/* indicated object.							*/
/*----------------------------------------------------------------------*/

int porttonet(objectptr cschem, int portno)
{
   PortlistPtr plist;

   for (plist = cschem->ports; plist != NULL; plist = plist->next) {
      if (plist->portid == portno)
	 return plist->netid;
   }
   return 0;
}

/*----------------------------------------------------------------------*/
/* Traverse netlist and return netid of polygon or pin on which the	*/
/*   indicated point is located.					*/
/* If point is not on any polygon or does not match any pin position,	*/
/*   return NULL.							*/
/* Labels which have a non-NULL "cinst" record are instance-dependent	*/
/*   and must match parameter "cinst".					*/
/*									*/
/* This routine checks to see if more than one net crosses the point	*/
/*   of interest, and merges the nets if found.				*/ 
/* (the method has been removed and must be reinstated, presumably)	*/
/*----------------------------------------------------------------------*/

Genericlist *pointtonet(objectptr cschem, objinstptr cinst, XPoint *testpoint)
{
   XPoint *tpt, *tpt2;
   PolylistPtr ppoly;
   LabellistPtr plab;
   Genericlist *preturn;
   objectptr pschem;	/* primary schematic */

   /* cschem is the object containing the point.  However, if the object */
   /* is a secondary schematic, the netlist is located in the master.	 */

   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   for (plab = pschem->labels; plab != NULL; plab = plab->next) {
      if (plab->cschem != cschem) continue;
      else if ((plab->cinst != NULL) && (plab->cinst != cinst)) continue;
      tpt = &(plab->label->position);
      if (proximity(tpt, testpoint))
	 return (Genericlist *)plab;

      /* if we've encountered a unique instance, then continue past	*/
      /* all other instances using this label.				*/

      if (plab->cinst != NULL)
	 while (plab->next && (plab->next->label == plab->label))
	    plab = plab->next;
   }

   /* Check against polygons.  We use this part of the routine to see	*/
   /* if there are crossing wires on top of a port.  If so, they are	*/
   /* merged together.							*/

   preturn = (Genericlist *)NULL;
   for (ppoly = pschem->polygons; ppoly != NULL; ppoly = ppoly->next) {
      if (ppoly->cschem != cschem) continue;
      for (tpt = ppoly->poly->points; tpt < ppoly->poly->points
		+ EndPoint(ppoly->poly->number); tpt++) {
	 tpt2 = tpt + NextPoint(ppoly->poly->number);

	 if (onsegment(tpt, tpt2, testpoint)) {
	    if (preturn == (Genericlist *)NULL)
	       preturn = (Genericlist *)ppoly;	
	    else
	       mergenets(pschem, (Genericlist *)ppoly, preturn);
	 }
      }
   }

   return preturn;
}

/*----------------------------------------------------------------------*/
/* localpin keeps track of temporary pin labels when flattening the	*/
/* hierarchy without destroying the original pin names.			*/
/*----------------------------------------------------------------------*/

void makelocalpins(objectptr cschem, CalllistPtr clist, char *prefix)
{
   NetnamePtr netnames;
   PortlistPtr ports, plist;
   stringpart *locpin;
   int locnet, callnet;
   objectptr callobj = clist->callobj;

   /* Copy all net names which are passed from above through ports */

   for (ports = clist->ports; ports != NULL; ports = ports->next) {
      callnet = ports->netid;
      for (plist = callobj->ports; plist != NULL; plist = plist->next) {
	 if (plist->portid == ports->portid) {
	    locnet = plist->netid;
            locpin = nettopin(callnet, cschem, prefix);
	    break;
	 }
      }
      
      for (netnames = callobj->netnames; netnames != NULL; netnames = netnames->next)
	 if (netnames->netid == locnet)
	    break;

      if (netnames == NULL) {
         netnames = (NetnamePtr)malloc(sizeof(Netname));
         netnames->netid = locnet;
         netnames->localpin = stringcopy(locpin);
         netnames->next = callobj->netnames;
         callobj->netnames = netnames;
      }
   }
}

/*----------------------------------------------------------------------*/
/* Find the first point associated with the net "netid" in the object	*/
/* cschem.								*/ 
/*----------------------------------------------------------------------*/

XPoint *NetToPosition(int netid, objectptr cschem)
{
   PolylistPtr  plist;
   LabellistPtr llist;
   buslist *sbus;
   int lbus, locnetid; /* sub_bus, (jdk) */

   plist = cschem->polygons;
   for (; plist != NULL; plist = plist->next) {
      for (lbus = 0;;) {
         if (plist->subnets == 0) {
	    locnetid = plist->net.id;
         }
         else {
	    sbus = plist->net.list + lbus;
	    locnetid = sbus->netid;
	 }
	 if (locnetid == netid) {
	    return plist->poly->points;
	 }
	 if (++lbus >= plist->subnets) break;
      }
   }

   llist = (netid < 0) ? global_labels : cschem->labels;
   for (; llist != NULL; llist = llist->next) {
      for (lbus = 0;;) {
         if (llist->subnets == 0) {
	    locnetid = llist->net.id;
         }
         else {
	    sbus = llist->net.list + lbus;
	    locnetid = sbus->netid;
	 }
	 if (locnetid == netid) {
	    return (&(llist->label->position));
	 }
	 if (++lbus >= llist->subnets) break;
      }
   }
   return NULL;
}

/*----------------------------------------------------------------------*/
/* Find a label element for the given net number.  In a symbol, this	*/
/* will be the label representing the pin (or the first one found, if	*/
/* the pin has multiple labels).  If no label is found, return NULL.	*/
/* Preferably choose a non-temporary label, if one exists		*/
/*----------------------------------------------------------------------*/

labelptr NetToLabel(int netid, objectptr cschem)
{
   LabellistPtr llist;
   labelptr standby = NULL;
   buslist *sbus;
   int lbus, locnetid;

   llist = (netid < 0) ? global_labels : cschem->labels;

   for (; llist != NULL; llist = llist->next) {
      for (lbus = 0;;) {
         if (llist->subnets == 0) {
	    locnetid = llist->net.id;
         }
         else {
	    sbus = llist->net.list + lbus;
	    locnetid = sbus->netid;
	 }
	 if (locnetid == netid) {
	    if (llist->label->string->type == FONT_NAME)
	       return llist->label;
	    else if (standby == NULL)
	       standby = llist->label;
	 }
	 if (++lbus >= llist->subnets) break;
      }
   }
   return standby;
}


/*----------------------------------------------------------------------*/
/* Find the subnet number of the given net.  This routine should be	*/
/* used only as a last resort in case the subnet number is not		*/
/* available;  it has to look through the polygon and label lists in	*/
/* detail to find the specified net ID.					*/
/*----------------------------------------------------------------------*/

int getsubnet(int netid, objectptr cschem)
{
   PolylistPtr  plist;
   LabellistPtr llist;
   buslist *sbus;
   int lbus, sub_bus, locnetid;

   plist = cschem->polygons;
   for (; plist != NULL; plist = plist->next) {
      for (lbus = 0;;) {
         if (plist->subnets == 0) {
	    locnetid = plist->net.id;
	    sub_bus = -1;
         }
         else {
	    sbus = plist->net.list + lbus;
	    locnetid = sbus->netid;
	    sub_bus = sbus->subnetid;
	 }
	 if (locnetid == netid) {
	    return sub_bus;
	 }
	 if (++lbus >= plist->subnets) break;
      }
   }

   llist = (netid < 0) ? global_labels : cschem->labels;
   for (; llist != NULL; llist = llist->next) {
      for (lbus = 0;;) {
         if (llist->subnets == 0) {
	    locnetid = llist->net.id;
	    sub_bus = -1;
         }
         else {
	    sbus = llist->net.list + lbus;
	    locnetid = sbus->netid;
	    sub_bus = sbus->subnetid;
	 }
	 if (locnetid == netid) {
	    return sub_bus;
	 }
	 if (++lbus >= llist->subnets) break;
      }
   }
   return -1;
}

/*----------------------------------------------------------------------*/
/* Find a pin name for the given net number				*/
/* Either take the last pin name with the net, or generate a new pin,	*/
/* assigning it a net number for a string.				*/
/* prefix = NULL indicates spice netlist, but is also used for PCB-type	*/
/* netlists because the calling hierarchy is generated elsewhere.	*/
/*----------------------------------------------------------------------*/

stringpart *nettopin(int netid, objectptr cschem, char *prefix)
{
   NetnamePtr netname;
   labelptr pinlab;
   LabellistPtr netlabel;
   char *newtext, *snew = NULL;
   XPoint *pinpos;
   int locnetid; /* lbus, (jdk) */
   /* buslist *sbus; (jdk) */
   Genericlist newnet;
   static stringpart *newstring = NULL;

   /* prefix is NULL for hierarchical (spice) netlists and for	*/
   /* internal netlist manipulation.				*/ 

   if (prefix == NULL) {  
      pinlab = NetToLabel(netid, cschem);
      if (pinlab != NULL) {

	 /* If this is a "temp label", regenerate the name according to	*/
	 /* the actual net number, if it does not match.  This prevents	*/
	 /* unrelated temp labels from getting the same name.		*/

	 if (pinlab->string->type != FONT_NAME) {
	    if (sscanf(pinlab->string->data.string + 3, "%d", &locnetid) == 1) {
	       if (locnetid != netid) {
		  newtext = pinlab->string->data.string;
		  newtext[3] = '\0';
		  newnet.subnets = 0;
		  newnet.net.id = netid;
		  pinlab->string->data.string = textprintnet(newtext, NULL, &newnet);
		  free(newtext);
	       }
	    }
	 }
	 return pinlab->string;
      }
      else {
	 /* If there's no label associated with this network, make one  */
	 /* called "intn" where n is the net number (netid).  This is a	*/
	 /* temp label (denoted by lack of a font specifier).		*/

	 newnet.subnets = 0;
	 newnet.net.id = netid;
	 pinpos = NetToPosition(netid, cschem);
	 netlabel = (LabellistPtr)new_tmp_pin(cschem, pinpos, NULL, "int", &newnet);
	 return (netlabel) ? netlabel->label->string : NULL;
      }
   }

   /* Flattened (e.g., sim) netlists */

   for (netname = cschem->netnames; netname != NULL; netname = netname->next) {
      if (netname->netid == netid) {
	 if (netname->localpin != NULL)
	    return netname->localpin;
	 break;
      }
   }

   /* Generate the string for the local instantiation of this pin	*/
   /* If this node does not have a pin, create one using the current	*/
   /* hierarchy prefix as the string.					*/

   pinlab = NetToLabel(netid, cschem);
   if (pinlab != NULL) {
      stringpart *tmpstring = pinlab->string;
      snew = textprint(tmpstring, NULL);
   }
   else {
      snew = (char *)malloc(12);
      sprintf(snew, "int%d", netid);
   }

   if (netid < 0) {
      newtext = snew;
   }
   else {
      newtext = (char *)malloc(1 + strlen(snew) + strlen(prefix));
      sprintf(newtext, "%s%s", prefix, snew);
      free(snew);
   }

   /* "newstring" is allocated only once and should not be free'd	*/
   /* by the calling routine.						*/

   if (newstring == NULL) {
      newstring = (stringpart *)malloc(sizeof(stringpart));
      newstring->nextpart = NULL;
      newstring->type = TEXT_STRING;
   }
   else {
      free(newstring->data.string);
   }
   newstring->data.string = newtext;
   return newstring;
}

/*----------------------------------------------------------------------*/
/* Find the net which connects to the given pin label			*/
/* Return NULL (no net) if no match was found. 				*/
/*----------------------------------------------------------------------*/

Genericlist *pintonet(objectptr cschem, objinstptr cinst, labelptr testpin)
{
   LabellistPtr seeklabel;
   int lbus, matched;
   buslist *sbus, *tbus;
   Genericlist netlist, *tmplist;

   /* check against local pins, if this pin is declared local */

   seeklabel = (testpin->pin == GLOBAL) ? global_labels : cschem->labels;

   netlist.subnets = 0;
   for (; seeklabel != NULL; seeklabel = seeklabel->next)
      if (!stringcomprelaxed(seeklabel->label->string, testpin->string, cinst))
      {
	 /* Quick simple case:  no bus */
	 if (seeklabel->subnets == 0)
	    return (Genericlist *)seeklabel;

	 tmplist = break_up_bus(testpin, cinst, (Genericlist *)seeklabel);

	 /* It is necessary to be able to put together a netlist from	*/
	 /* partial sources none of which may be complete.		*/

	 if (tmplist != NULL) {
	    if (netlist.subnets == 0) copy_bus(&netlist, tmplist);
	    matched = 0;
	    for (lbus = 0; lbus < tmplist->subnets; lbus++) {
	       sbus = netlist.net.list + lbus;
	       tbus = tmplist->net.list + lbus;
	       if (sbus->netid == 0)
		  sbus->netid = tbus->netid;	/* copy forward */
	       else if (tbus->netid == 0)
		  tbus->netid = sbus->netid;	/* copy back */
	       if (sbus->netid != 0)
		  matched++;
	    }
	    if (matched == netlist.subnets) {
	       free(netlist.net.list);
	       return tmplist;
	    }
	 }
      }

   if (netlist.subnets != 0) {
      free(netlist.net.list);
      return tmplist;		/* Returns list with partial entries */
   }
   return NULL;			/* No matches found */
}

#ifdef TCL_WRAPPER

/*----------------------------------------------------------------------*/
/* Automatic Schematic Generation stuff---				*/
/* Blow away all polygons in the schematic and replace them with a	*/
/* simple rat's-nest (point-to-point).					*/
/*									*/
/* This routine is more of a proof-of-concept than a useful routine,	*/
/* intended to be called only from the Tcl console.  It is intended to	*/
/* reveal most of the issues related to automatic schematic generation,	*/
/* in which the netlist, or a portion of it, can be redrawn as objects	*/
/* are moved around to maintain the relationships present in the 	*/
/* calls structure.							*/
/*----------------------------------------------------------------------*/

void ratsnest(objinstptr thisinst)
{
   CalllistPtr calls;
   PortlistPtr ports;
   PolylistPtr plist;
   LabellistPtr llist;
   objectptr pschem, cschem;
   objinstptr cinst;
   polyptr ppoly, *newpoly;
   buslist *sbus;
   int i, netid, points, sub_bus, lbus;
   XPoint portpos;
   Boolean result;

   cschem = thisinst->thisobject;
   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   /* Go through the netlist and remove all polygons.  Remove	*/
   /* the polygons from the object as well as from the netlist.	*/

   for (plist = pschem->polygons; plist != NULL; plist = plist->next) {
      ppoly = plist->poly;
      ppoly->type += REMOVE_TAG;	/* tag for removal */
   }
   freepolylist(&pschem->polygons);

   /* for each schematic (PRIMARY or SECONDARY), remove the tagged	*/
   /* elements.								*/

   for (i = 0; i < xobjs.pages; i++) {
      cinst = xobjs.pagelist[i]->pageinst;
      if (cinst && (cinst->thisobject->schemtype == SECONDARY) &&
		(cinst->thisobject->symschem == pschem)) {
	 delete_tagged(cinst);
      }
      else if (cinst == thisinst)
	 delete_tagged(cinst);
   }

   /* Now, for each net ID in the label list, run through the calls and	*/
   /* find all points in the calls connecting to that net.  Link	*/
   /* them with as many polygons as needed.  These polygons are point-	*/
   /* to-point lines, turning the schematic representation into a 	*/
   /* "rat's-nest".							*/

   for (llist = pschem->labels; llist != NULL; llist = llist->next) {
      for (lbus = 0;;) {
	 if (llist->subnets == 0) {
	    netid = llist->net.id;
	    sub_bus = -1;
	 }
	 else {
	    sbus = llist->net.list + lbus;
	    netid = sbus->netid;
	    sub_bus = sbus->subnetid;
	 }

         points = 0;
         for (calls = pschem->calls; calls != NULL; calls = calls->next) {

            /* Determine schematic object from the object called.  Start a */
	    /* new polygon any time we switch schematic pages.		   */

	    if (calls->cschem != cschem)
	       points = 0;
	    cschem = calls->cschem;

	    for (ports = calls->ports; ports != NULL; ports = ports->next) {
	       if (ports->netid == netid) {

	          /* Find the location of this port in the coordinates of cschem */

	          result = PortToPosition(calls->callinst, ports->portid, &portpos);
	          if (result == True) {
		     points++;
		     if (points == 1) {
		        NEW_POLY(newpoly, cschem);
		        polydefaults(*newpoly, 1, portpos.x, portpos.y);
		        (*newpoly)->style |= UNCLOSED;
		        (*newpoly)->color = RATSNESTCOLOR;
		        addpoly(cschem, *newpoly, (Genericlist *)llist);
		     }
		     else
		        poly_add_point(*newpoly, &portpos);
	          }
	          else {
		    Fprintf(stderr, "Error:  Cannot find pin connection in symbol!\n");
		    /* Deal with error condition? */
	          }
	       }
	    }
         }
	 if (++lbus >= llist->subnets) break;
      }
   }

   /* Now, move all the labels to reconnect to their networks */

   /* (to be done) */

   /* Refresh the screen */
   drawarea(NULL, NULL, NULL);
}

#endif

/*----------------------------------------------------------------------*/
/* Find a net or netlist in object cschem with the indicated name.  	*/
/*									*/
/* This is the same routine as above, but finds the net with the given	*/
/* name, or all nets which are a subset of the given name, if the name	*/
/* is the name of a bus.  Return NULL if no match was found. 		*/
/*----------------------------------------------------------------------*/

Genericlist *nametonet(objectptr cschem, objinstptr cinst, char *netname)
{
   LabellistPtr seeklabel;
   stringpart newstring;

   /* Build a simple xcircuit string from "netname" (nothing malloc'd) */
   newstring.type = TEXT_STRING;
   newstring.nextpart = NULL;
   newstring.data.string = netname;

   /* Check against local networks */

   for (seeklabel = cschem->labels; seeklabel != NULL; seeklabel = seeklabel->next)
      if (!stringcomprelaxed(seeklabel->label->string, &newstring, cinst))
         return (Genericlist *)seeklabel;

   /* Check against global networks */

   for (seeklabel = global_labels; seeklabel != NULL; seeklabel = seeklabel->next)
      if (!stringcomprelaxed(seeklabel->label->string, &newstring, cinst))
	 return (Genericlist *)seeklabel;

   return NULL;
}

/*----------------------------------------------------------------------*/
/* Check if two parts may have the same component index.  This is true	*/
/* for parts with parameterized pins, such as those in the "quadparts"	*/
/* library.  Compare the names of the ports in each instance.  If none	*/
/* match, then these are unique subparts of a single component, and we	*/
/* should return FALSE.  Otherwise, return TRUE.			*/
/*----------------------------------------------------------------------*/

Boolean samepart(CalllistPtr clist1, CalllistPtr clist2)
{
   PortlistPtr port;
   labelptr plab;
   char *pinname1, *pinname2;
   Boolean rvalue;

   if (clist1->callobj != clist2->callobj) return FALSE;

   rvalue = FALSE;
   for (port = clist1->ports; port != NULL; port = port->next) {
      plab = PortToLabel(clist1->callinst, port->portid);
      pinname1 = textprint(plab->string, clist1->callinst);
      pinname2 = textprint(plab->string, clist2->callinst);
      if (!strcmp(pinname1, pinname2)) rvalue = TRUE;
      free(pinname1);
      free(pinname2);
   }
   return rvalue;
}

/*----------------------------------------------------------------------*/
/* Generate an index number for a device in a flat netlist format.	*/
/* We ignore any values given to "index" (this is a to-do item, since	*/
/* many schematics are only a single level of hierarchy, so it makes	*/
/* sense to keep designated indices when possible), but do generate	*/
/* independent index numbering for different device classes.		*/
/*----------------------------------------------------------------------*/

u_int devflatindex(char *devname)
{
   flatindex *fp = flatrecord;
   while (fp != NULL) {
      if (!strcmp(devname, fp->devname)) {
	 return ++fp->index;
      }
      fp = fp->next;
   }
   fp = (flatindex *)malloc(sizeof(flatindex));
   fp->next = flatrecord;
   flatrecord = fp;
   fp->index = 1;
   fp->devname = devname; 
   return 1;
}

/*----------------------------------------------------------------------*/
/* Free the list of per-class flat device indices			*/
/*----------------------------------------------------------------------*/

void freeflatindex()
{
   flatindex *fpnext, *fp = flatrecord;
   while (fp != NULL) {
      fpnext = fp->next;
      free(fp);
      fp = fpnext;
   }
   flatrecord = NULL;
}

/*---------------------------------------------------------*/
/* Convert a number (up to 99999) to its base-36 equivalent */
/*---------------------------------------------------------*/

int convert_to_b36(int number)
{
   int b36idx, tmpidx;

   tmpidx = number;
   b36idx = (tmpidx / 10000) * 1679616;
   tmpidx %= 10000;
   b36idx += (tmpidx / 1000) * 46656;
   tmpidx %= 1000;
   b36idx += (tmpidx / 100) * 1296;
   tmpidx %= 100;
   b36idx += ((tmpidx / 10) * 36) + (tmpidx % 10);

   return b36idx;
}

/*----------------------------------------------------------------------*/
/* Generate an index number for this device.  Count all devices having	*/
/* the same device name (as printed in the netlist) in the calls of	*/
/* the calling object (cfrom)						*/
/*									*/
/* Update (9/29/04):  Remove any leading whitespace from the device	*/
/* name to prevent treating, for example, "J?" and " J?" as separate	*/
/* device types.							*/
/*									*/
/* Update (7/28/05):  To allow SPICE device indices to include all	*/
/* alphanumerics, we parse in base-36, and count in the subset of	*/
/* base-36 that can be interpreted as decimal (1-9, 36-45, etc.)	*/
/*----------------------------------------------------------------------*/

u_int devindex(objectptr cfrom, CalllistPtr clist)
{
   CalllistPtr cptr, listfrom = cfrom->calls;
   objectptr devobj = clist->callobj;
   u_int *occupied, total, objindex, i, b36idx;
   char *devname, *cname;
   /* oparamptr ops; (jdk) */

   if (listfrom == NULL) return (u_int)0;
   if (clist->devindex >= 0) return clist->devindex;

   devname = (clist->devname == NULL) ? devobj->name : clist->devname;
   while (isspace(*devname)) devname++;

   /* Count total number of devices */
   for (cptr = listfrom, total = 0; cptr != NULL; cptr = cptr->next, total++);
   occupied = (u_int *)malloc(total * sizeof(u_int));
   objindex = 1;

   for (cptr = listfrom, total = 0; cptr != NULL; cptr = cptr->next, total++) {
      occupied[total] = 0;
      if (cptr == clist) continue;
      cname = (cptr->devname == NULL) ? cptr->callobj->name : cptr->devname;
      while (isspace(*cname)) cname++;
      if (!strcmp(cname, devname)) {
	 occupied[total] = cptr->devindex;
	 if (cptr->devindex == objindex) objindex++;
      }
   }

   b36idx = convert_to_b36(objindex);
   for (; objindex <= total; objindex++) {
      b36idx = convert_to_b36(objindex);
      for (i = 0; i < total; i++)
	 if (occupied[i] == b36idx)
	    break;
      if (i == total)
	 break;
   }
   free(occupied);

   clist->devindex = b36idx;
   return objindex;
}

/*----------------------------------------------------------------------*/
/* Generate and return a list of all info label prefixes in a design	*/
/* This does not depend on the netlist being generated (i.e., does not	*/
/* use CalllistPtr)							*/
/*----------------------------------------------------------------------*/

void genprefixlist(objectptr cschem, slistptr *modelist)
{
   int locpos;
   genericptr *pgen;
   labelptr plabel;
   objinstptr cinst;
   stringpart *strptr;
   slistptr modeptr;

   for (pgen = cschem->plist; pgen < cschem->plist + cschem->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 plabel = TOLABEL(pgen);
         if (plabel->pin == INFO) {

	    strptr = findtextinstring(":", &locpos, plabel->string, cinst);
	    if (locpos <= 0 || strptr == NULL)
	       continue;  /* null after netlist type designator, don't count */

	    for (modeptr = *modelist; modeptr; modeptr = modeptr->next)
	       if (!strncmp(modeptr->alias, strptr->data.string, locpos))
		  break;

	    if (modeptr == NULL) {	/* Mode has not been enumerated */
	       modeptr = (slistptr)malloc(sizeof(stringlist));
	       modeptr->alias = (char *)malloc(locpos + 1);
	       strncpy(modeptr->alias, strptr->data.string, locpos);
	       modeptr->next = *modelist;
	       *modelist = modeptr;
 	    }
	 }
      }
      else if (IS_OBJINST(*pgen)) {
	 cinst = TOOBJINST(pgen);
	 genprefixlist(cinst->thisobject, modelist);
	 if (cinst->thisobject->symschem != NULL)
	    genprefixlist(cinst->thisobject->symschem, modelist);
      }
   }
}

/*----------------------------------------------------------------------*/
/* Create and return an ordered list of info labels for the schematic	*/
/* page cschem.  A linked label list is returned, combining info labels	*/
/* from all schematic pages related to cschem (master and slave pages).	*/
/* It is the responsibility of the calling routine to free the linked	*/
/* list.								*/
/*----------------------------------------------------------------------*/

LabellistPtr geninfolist(objectptr cschem, objinstptr cinst, char *mode)
{
   int locpos, j;
   int vmax;
   genericptr *pgen;
   labelptr plabel;
   LabellistPtr newllist, listtop, srchlist;
   u_char *strt;
   stringpart *strptr;

   listtop = NULL;
   vmax = 0;

   for (pgen = cschem->plist; pgen < cschem->plist + cschem->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 plabel = TOLABEL(pgen);
         if ((plabel->pin == INFO) && !textncomp(plabel->string, mode, cinst)) {

	    if (mode[0] == '\0') {
	       strptr = findtextinstring(":", &locpos, plabel->string, cinst);
	       locpos--;
	    }
	    else
	       strptr = findstringpart(strlen(mode), &locpos, plabel->string, cinst);

	    if (locpos < 0)
	       continue;  /* null after netlist type designator */

	    strt = strptr->data.string + locpos + 1;
	    
	    if (*strt != ':') {
	       if (sscanf(strt, "%d", &j) != 1) continue;

	       /* Consider only positive-valued numbers.  Negative	*/
	       /* indices are handled by "writenet" by merging the	*/
	       /* minus character into the mode name.			*/

	       if (j < 0) continue;
	       if (j >= vmax) vmax = j + 1;
	    }
	    else
	       j = ++vmax;

	    newllist = (LabellistPtr)malloc(sizeof(Labellist));
	    newllist->label = plabel;
	    newllist->cschem = cschem;
	    newllist->cinst = cinst;
	    newllist->net.id = j;	/* use this to find the ordering */
	    newllist->subnets = 0;	/* so free() doesn't screw up */

	    /* Order the linked list */

	    if ((listtop == NULL) || (listtop->net.id >= j)) {
	       newllist->next = listtop;
	       listtop = newllist;
	    }
	    else {
	       for (srchlist = listtop; srchlist != NULL; srchlist = srchlist->next) {
		  if ((srchlist->next != NULL) && (srchlist->next->net.id >= j)) {
		     newllist->next = srchlist->next;
		     srchlist->next = newllist;
		     break;
		  }
		  else if (srchlist->next == NULL) {
		     srchlist->next = newllist;
		     newllist->next = NULL;
		  }
	       }
	    }
         }
      }
   }
   return listtop;
}

/*----------------------------------------------------------------------*/
/* Trivial default pin name lookup.  Parse an object's pin labels for	*/
/* pin names.  Return the text of the nth pin name, as counted by the	*/
/* position in the object's parts list.  If there are fewer than (n+1)	*/
/* pin labels in the object, return NULL.  Otherwise, return the pin	*/
/* name in a malloc'd character string which the caller must free.	*/
/*----------------------------------------------------------------------*/

char *defaultpininfo(objinstptr cinst, int pidx)
{
   genericptr *pgen;
   labelptr plabel;
   objectptr cschem = cinst->thisobject;
   int count = 0;
   char *stxt;

   for (pgen = cschem->plist; pgen < cschem->plist + cschem->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 plabel = TOLABEL(pgen);
         if (plabel->pin == LOCAL) {
	    if (count == pidx) {
	       stxt = textprint(plabel->string, cinst);
	       return stxt;
	    }
	    count++;
	 }
      }
   }
   return NULL;
}

/*----------------------------------------------------------------------*/
/* Parse an object's info labels for pin names.  This is *not* a	*/
/* netlist function.  The nth pin name is returned, or NULL if not	*/
/* found.  Return value is a malloc'd string which the caller must	*/
/* free.								*/
/*----------------------------------------------------------------------*/

char *parsepininfo(objinstptr cinst, char *mode, int pidx)
{
   genericptr *pgen;
   labelptr plabel;
   u_char *strt, *fnsh;
   int slen, i, locpos;
   objectptr cschem = cinst->thisobject;
   stringpart *strptr;
   char *sout;
   int count = 0;

   for (pgen = cschem->plist; pgen < cschem->plist + cschem->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 plabel = TOLABEL(pgen);
         if (plabel->pin == INFO) {
	    slen = stringlength(plabel->string, True, cinst);
	    for (i = 1; i < slen; i++) {
	       strptr = findstringpart(i, &locpos, plabel->string, cinst);
	       if (locpos >= 0 && *(strptr->data.string + locpos) == ':') break;
	    }
	    /* Currently we are not checking against "mode". . .*/
	    /* interpret all characters after the colon 	*/

	    for (++i; i < slen; i++) {
	       strptr = findstringpart(i, &locpos, plabel->string, cinst);
	       if (locpos >= 0) {

		  /* Do for all text characters */
		  strt = strptr->data.string + locpos;
		  if (*strt == '%') {
		     strt++;
		     i++;
		     if (*strt == 'p') {	/* Pin found! */
			if (count == pidx) {	/* Get pin text */
			   strt++;
			   fnsh = strt + 1;
			   while (*fnsh != ' ' && *fnsh != '\0') fnsh++;
			   sout = malloc(fnsh - strt + 1);
			   strncpy(sout, strt, fnsh - strt);
			   return sout;
			}
			count++;
		     }
		  }
	       }
	    }
	 }
      }
   }
   return NULL;
}

/*----------------------------------------------------------------------*/
/* Look for information labels in the object parts list.  Parse the	*/
/* information labels and print results to specified output device.	*/
/* (This is not very robust to errors---needs work!)			*/
/* 									*/
/* If "autonumber" is true, then the parsed info label is used to	*/
/* auto-number devices.							*/
/*----------------------------------------------------------------------*/

char *parseinfo(objectptr cfrom, objectptr cthis, CalllistPtr clist,
	char *prefix, char *mode, Boolean autonumber, Boolean no_output)
{
  /* genericptr *pgen; (jdk) */
   stringpart *strptr, *ppin, *optr;
   char *snew, *sout = NULL, *pstring;
   u_char *strt, *fnsh;
   PortlistPtr ports;
   oparamptr ops, instops;
   int portid, locpos, i, k, subnet, slen; /*  j, (jdk) */
   char *key = NULL;
   u_int newindex;
   Boolean is_flat = False, is_symbol = False, is_iso = False, is_quoted;
   Boolean do_update = True, include_once = False;
   char *locmode, *b36str, *sptr;
   LabellistPtr infolist, infoptr;
   FILE *finclude;

   /* For flat netlists, prefix the mode with the word "flat".	*/
   /* As of 3/8/07, this includes the ".sim" format, which	*/
   /* should be called as mode "flatsim".			*/

   locmode = mode;
   if (locmode && (!strncmp(mode, "flat", 4) || !strncmp(mode, "pseu", 4))) {
      locmode += 4;
      is_flat = True;
   }

   /* mode == "" is passed when running resolve_devindex() and indicates */
   /* that the routine should be used to assign devindex to calls whose	 */
   /* devices have been manually assigned numbers.  The preferred method */
   /* is to assign these values through the "index" parameter.  Use of	 */
   /* parseinfo() ensures that objects that have no "index" parameter	 */
   /* but have valid info-labels for SPICE or PCB output will be	 */
   /* assigned the correct device index.				 */
   /* Sept. 3, 2006---The use of the prepended "@" character on		 */
   /* parameter names means that I can replace the cryptic "idx" with	 */
   /* the more obvious "index", which previously conflicted with the	 */
   /* PostScript command of the same name.  Parameters "index" and "idx" */
   /* are treated interchangeably by the netlister.			 */

   if (locmode[0] == '\0')
      do_update = False;

   /* 1st pass: look for valid labels;  see if more than one applies.	*/
   /* If so, order them correctly.  Labels may not be numbered in	*/
   /* sequence, and may begin with zero.  We allow a lot of flexibility	*/
   /* but the general expectation is that sequences start at 0 or 1 and	*/
   /* increase by consecutive integers.					*/

   infolist = geninfolist(cthis, clist->callinst, locmode);

   /* Now parse each label in sequence and output the result to */
   /* the return string.					*/ 

   sout = (char *)malloc(1);
   *sout = '\0';

   for (infoptr = infolist; infoptr != NULL; infoptr = infoptr->next) {
      objectptr pschem, cschem = infoptr->cschem;
      labelptr plabel = infoptr->label;
      objinstptr cinst = infoptr->cinst;

      /* move to colon character */
      slen = stringlength(plabel->string, True, cinst);
      for (i = 1; i < slen; i++) {
	 strptr = findstringpart(i, &locpos, plabel->string, cinst);
	 if (locpos >= 0 && *(strptr->data.string + locpos) == ':') break;
      }

      /* interpret all characters after the colon */
      for (++i; i < slen; i++) {
	 strptr = findstringpart(i, &locpos, plabel->string, cinst);
	 if (locpos >= 0) {

	    /* Do for all text characters */
	    strt = strptr->data.string + locpos;
	    if (*strt == '%') {
	       is_quoted = False;
	       strt++;
	       i++;
	       switch(*strt) {
		  case '%':
		     sout = (char *)realloc(sout, strlen(sout) + 2);
		     strcat(sout, "%");
		     break;
		  case 'r':
		     sout = (char *)realloc(sout, strlen(sout) + 2);
		     strcat(sout, "\n");
		     break;
		  case 't':
		     sout = (char *)realloc(sout, strlen(sout) + 2);
		     strcat(sout, "\t");
		     break;
		  case 'i':
		     if (clist->devname == NULL)
			clist->devname = strdup(sout);
		     if (do_update) {
		        if (is_flat)
		           newindex = devflatindex(clist->devname);
		        else
		           newindex = devindex(cfrom, clist);
		        b36str = d36a(newindex);
		        sout = (char *)realloc(sout, strlen(sout) + strlen(b36str) + 1);
		        sprintf(sout + strlen(sout), "%s", b36str);
		     }
		     break;
		  case 'N':
		     sout = (char *)realloc(sout, strlen(sout)
				+ strlen(cschem->name) + 1);
		     strcat(sout, cschem->name);
		     break;
		  case 'n':
		     sptr = strstr(cschem->name, "::");
		     if (sptr == NULL)
			sptr = cschem->name;
		     else       
			sptr += 2;
		     sout = (char *)realloc(sout, strlen(sout)
				+ strlen(sptr) + 1);
		     strcat(sout, sptr);
		     break;
		  case 'x':
		     sout = (char *)realloc(sout, strlen(sout) + 7);
		     sprintf(sout + strlen(sout), "%d",
				cinst->position.x);
		     break;
		  case 'y':
		     sout = (char *)realloc(sout, strlen(sout) + 7);
		     sprintf(sout + strlen(sout), "%d",
				cinst->position.y);
		     break;
		  case 'F':
		     if (no_output) break;
		     include_once = True;
		     /* drop through */
		  case 'f':
		     if (no_output) break;

		     /* Followed by a filename to include verbatim into */
		     /* the output.  Filename either has no spaces or	*/
		     /* is in quotes.					*/
		
		     /* Use textprint to catch any embedded parameters	*/

		     snew = textprint(strptr, cinst);
		     strt = snew;
		     while (*strt != '\0') {
			if (*strt == '%') {
			   if (include_once && *(strt + 1) == 'F')
			      break;
			   else if (!include_once && *(strt + 1) == 'f')
			      break;
			}
		        strt++;
		     }

		     if (*strt == '\0') {
			/* No filename; print verbatim */
			free(snew);
		        include_once = False;
			break;
		     }

		     strt += 2;
		     if (*strt == '"') strt++;
		     if (*strt == '"' || *strt == '\0') break;
		     fnsh = strt + 1;
		     while (*fnsh != '\0' && !isspace(*fnsh) && *fnsh != '"')
			fnsh++; 
		     strncpy(_STR, strt, (int)(fnsh - strt));
		     _STR[(int)(fnsh - strt)] = '\0';
		     free(snew);
		     i = slen;

		     /* Do variable and tilde expansion on the filename */
		     xc_tilde_expand(_STR, 149);
		     xc_variable_expand(_STR, 149);

		     /* Check if this file has been included already */
		     if (include_once) {
			if (check_included(_STR)) {
			   include_once = False;
			   break;
			}
			append_included(_STR);
		     }

		     /* Open the indicated file and dump to the output */
		     finclude = fopen(_STR, "r");
		     if (finclude != NULL) {
			char *sptr = sout;
			int nlen;
			int stlen = strlen(sout);
			while (fgets(_STR, 149, finclude)) {
			   nlen = strlen(_STR);
			   sout = (char *)realloc(sout, stlen + nlen + 1);
			   sptr = sout + stlen;
	            	   strcpy(sptr, _STR);
			   stlen += nlen;
			}
			fclose(finclude);
		     }
		     else {
			Wprintf("No file named %s found", _STR);
		     }
		     include_once = False;
		     break;

		  case 'p':
		     /* Pin name either has no spaces or is in quotes */
		     strt++;
		     if (*strt == '"') {
			is_quoted = True;
			strt++;
			i++;
		     }
		     if (*strt == '"' || *strt == '\0') break;
		     fnsh = strt + 1;
		     while (*fnsh != '\0' && !isspace(*fnsh) && *fnsh != '"')
			fnsh++; 
		     strncpy(_STR2, strt, (int)(fnsh - strt));
		     _STR2[(int)(fnsh - strt)] = '\0';
		     i += (fnsh - strt);
		     /* if (is_quoted || *fnsh == '\0') i++; */
		     if (is_quoted) i++;

		     /* Find the port which corresponds to this pin name */
		     /* in the called object (cschem).  If this is a	 */
		     /* linked symbol/schematic, then the port list will */
		     /* be in the schematic view, not in the symbol view */

		     pschem = cschem;
		     if (cschem->ports == NULL && cschem->symschem != NULL &&
				cschem->symschem->ports != NULL)
			pschem = cschem->symschem;

		     for (ports = pschem->ports; ports != NULL;
				ports = ports->next) {

			subnet = getsubnet(ports->netid, pschem);
			ppin = nettopin(ports->netid, pschem, NULL);
			// NOTE:  _STR is used inside textprintsubnet()
			pstring = textprintsubnet(ppin, NULL, subnet);
			if (!strcmp(pstring, _STR2)) {
			   portid = ports->portid;
			   free(pstring);
			   break;
			}
			else
			   free(pstring);
		     }
		     if (ports != NULL) {

		        /* Find the matching port in the calling object instance */

			for (ports = clist->ports; ports != NULL;
				ports = ports->next)
			   if (ports->portid == portid) break;

		        if (ports != NULL) {

			   ppin = nettopin(ports->netid, cfrom, prefix);
			   subnet = getsubnet(ports->netid, cfrom);
			   snew = textprintsubnet(ppin, cinst, subnet);
			   sout = (char *)realloc(sout, strlen(sout) +
					strlen(snew) + 1);
	            	   strcat(sout, snew);
			   free(snew);
			   break;
			}
			else {
			   Fprintf(stderr, "Error: called non-existant port\n");
			}
		     }
		     else {
			Wprintf("No pin named %s in device %s", _STR, cschem->name);
		     }
		     break;
		  case 'v':
		     /* Parameter name either has no spaces or is in quotes */
		     strt++;
		     if (*strt == '"')  {
			is_quoted = True;
			strt++;
			i++;
		     }
		     if (*strt == '"' || *strt == '\0') break;
		     fnsh = strt + 1;
		     while (*fnsh != '\0' && !isspace(*fnsh) && *fnsh != '"')
			fnsh++; 
		     strncpy(_STR, strt, (int)(fnsh - strt));
		     _STR[(int)(fnsh - strt)] = '\0';
		     i += (fnsh - strt);
		     /* if (is_quoted || *fnsh == '\0') i++; */
		     if (is_quoted) i++;

		     /* Compare this name against the parameter keys */
		     ops = match_param(cschem, _STR);

		     /* For backwards compatibility, also try matching against	*/
		     /* the parameter default value (method used before 	*/
		     /* implementing parameters as key:value pairs).		*/
		     
		     if (ops == NULL) {
		        for (ops = cschem->params; ops != NULL; ops = ops->next) {
		           if (ops->type == XC_STRING) {
			      if (!textcomp(ops->parameter.string, _STR, NULL)) {
			         /* substitute the parameter or default */
			         instops = find_param(cinst, ops->key);
			         optr = instops->parameter.string;
			         if (stringlength(optr, True, cinst) > 0) {
				    snew = textprint(optr, cinst);
			            sout = (char *)realloc(sout, strlen(sout)
						+ strlen(snew) + 1);
			            strcat(sout, snew);
				    free(snew);
				 }
			         break;
			      }
			   }
			}
		     }

		     if (ops == NULL) {
		        Wprintf("No parameter named %s in device %s",
				  _STR, cschem->name);
		     }
		     break;

		  default:
		     /* Presence of ".ends" statement forces xcircuit	*/
		     /* not to write ".end" at the end of the netlist.	*/

	             if (*strt == '.')
		        if (!strncmp(strt + 1, "ends", 4))
			   spice_end = FALSE;

		     sout = (char *)realloc(sout, strlen(sout) + 2);
		     *(sout + strlen(sout) - 1) = *strt;
		     *(sout + strlen(sout)) = '\0';
	       }
	    }
	    else {
	       if (key != NULL)
	          if (clist->devname == NULL)
		     clist->devname = strdup(sout);

	       /* Parameters with unresolved question marks are treated */
	       /* like a "%i" string.					*/

	       if ((key != NULL) && !textncomp(strptr, "?", cinst)) {
		  if (do_update || autonumber) {
		     if (is_flat || (cfrom == NULL))
		        newindex = devflatindex(clist->devname);
		     else
		        newindex = devindex(cfrom, clist);
		     k = strlen(sout);
		     b36str = d36a(newindex);
		     sout = (char *)realloc(sout, strlen(sout) + strlen(b36str) + 1);
		     sprintf(sout + k, "%s", b36str);
		  }

		  /* When called with autonumber = TRUE, generate a parameter	*/
		  /* instance, replacing the question mark with the new index	*/
		  /* number.							*/

		  if (autonumber) {
		     copyparams(cinst, cinst);
		     ops = match_instance_param(cinst, key);
		     optr = ops->parameter.string;
		     if (!textncomp(optr, "?", cinst)) {
		        optr->data.string = (char *)realloc(optr->data.string,
				strlen(sout + k) + 1);
		        strcpy(optr->data.string, sout + k);
		     }
		     else Wprintf("Error while auto-numbering parameters");
		     resolveparams(cinst);
		  }
	       }
	       else {
		  /* A "?" default parameter that has a (different)	*/
		  /* instance value becomes the device number.		*/
		  int stlen;
		  if ((key != NULL) && (clist->devindex < 0)) {
		     ops = match_param(cschem, key);
		     if (ops->type == XC_STRING) {
			CalllistPtr plist;
			if (!textcomp(ops->parameter.string, "?", NULL)) {
			   if (is_flat) {
			      /* Ignore the value and generate one instead */
			      newindex = devflatindex(clist->devname);
			      b36str = d36a(newindex);
			      k = strlen(sout);
			      sout = (char *)realloc(sout, strlen(sout)
					+ strlen(b36str) + 1);
			      sprintf(sout + k, "%s", b36str);
			      continue;		/* go on to next stringpart */
			   }
			   else if (cfrom != NULL) {

			      /* Interpret parameter string as a component	*/
			      /* number so we can check for duplicates.  Assume	*/
			      /* a base-36 number, which includes all alphabet	*/
			      /* characters.  This will disambiguate, e.g., "1"	*/
			      /* and "1R", but flag case-sensitivity, e.g.,	*/
			      /* "1R" and "1r".	 Thanks to Carsten Thomas for	*/
			      /* pointing out the problem with assuming		*/
			      /* numerical component values.			*/

			      char *endptr;
			      newindex = (u_int) strtol(strt, &endptr, 36);
			      if (*endptr != '\0') {
			         Fprintf(stderr, "Warning:  Use of non-alphanumeric"
				   " characters in component number \"%s\"\n", strt);
			      }
			      clist->devindex = newindex;
			      for (plist = cfrom->calls; plist; plist = plist->next) {
			         if ((plist == clist) ||
					(plist->callobj != clist->callobj)) continue;

			         /* Two parts have been named the same.  Flag	*/
				 /* it, but don't try to do anything about it.  */
				 /* 11/22/06---additionally check part numbers,	*/
				 /* in case the symbol is a component sub-part.	*/

			         if (plist->devindex == newindex) {
				    if (samepart(plist, clist)) {
				       Fprintf(stderr, "Warning:  duplicate part number"
					   " %s%s and %s%s\n", sout, strt, sout, strt);
				       break;
				    }
				 }
			      }
			   }
			}
		     }
	          }
	          stlen = strlen(sout);
	          sout = (char *)realloc(sout, stlen + 2);

		  /* By convention, greek "mu" becomes ASCII "u", NOT "m",	*/
		  /* otherwise we get, e.g., millifarads instead of microfarads */

		  if ((is_symbol && (*strt == 'm')) || (is_iso && (*strt == 0265)))
		     *(sout + stlen) = 'u';
		  else
	             *(sout + stlen) = *strt;
	          *(sout + stlen + 1) = '\0';
	       }
	    }
	 }

	 /* Some label controls are interpreted.  Most are ignored */
	 else {
	    switch(strptr->type) {
	       case PARAM_START:
		  key = strptr->data.string;
		  break;
	       case PARAM_END:
		  key = NULL;
		  break;
	       case RETURN:
		  sout = (char *)realloc(sout, strlen(sout) + 2);
		  strcat(sout, "\n");
		  break;
	       case TABFORWARD:
		  sout = (char *)realloc(sout, strlen(sout) + 2);
		  strcat(sout, "\t");
		  break;
	       case FONT_NAME:
		  is_symbol = issymbolfont(strptr->data.font);
		  is_iso = isisolatin1(strptr->data.font);
		  break;
	    }
	 }
      }

      /* Insert a newline between multiple valid info labels */
      if (infoptr->next != NULL) {
	 sout = (char *)realloc(sout, strlen(sout) + 2);
	 strcat(sout, "\n");
      }
   }
   freelabellist(&infoptr);

   if (*sout == '\0') {
      free(sout);
      return NULL;
   }
   return sout;
}

/*----------------------------------------------------------------------*/
/* Write a low-level device						*/
/*----------------------------------------------------------------------*/

int writedevice(FILE *fp, char *mode, objectptr cfrom, CalllistPtr clist,
	char *prefix)
{
   char *sout;
   objectptr cthis;

   if (clist == NULL) {
      if (fp != NULL) fprintf(fp, "error: null device\n");
      return -1;
   }

   /* Devices are always symbols.  If the call indicates a schematic	*/
   /* and a separate associated symbol, then we want the symbol.	*/
   /* If we're writing a flat netlist, then return -1 to indicate that	*/
   /* the symbol instance needs to be flattened.			*/

   cthis = clist->callobj;
   if (clist->callobj->schemtype == PRIMARY || clist->callobj->schemtype ==
		SECONDARY)
      if (clist->callobj->symschem != NULL) {
	 if (!strncmp(mode, "flat", 4))
	    return -1;
	 else
	    cthis = clist->callobj->symschem;
      }

   /* Look for information labels in the object parts list. */

   if ((sout = parseinfo(cfrom, cthis, clist, prefix, mode, FALSE, FALSE)) != NULL) {
      if (fp != NULL) {
	 fputs(sout, fp);
	 fprintf(fp, "\n");
      }
      free(sout);
      return 0;
   }

   /* Information labels do not necessarily exist. */
   return -1;
}

#ifdef TCL_WRAPPER

/*----------------------------------------------------------------------*/
/* Translate a hierarchical net name into an object and instance.	*/
/* Return TRUE if found, FALSE if not.  If found, object and instance	*/
/* are returned in the argument pointers.				*/
/*----------------------------------------------------------------------*/

Boolean HierNameToObject(objinstptr cinst, char *hiername, pushlistptr *stack)
{
   CalllistPtr calls;
   char *nexttoken, *hptr, *pptr;
   objectptr cschem, curobj, newobj;
   objinstptr newinst;
   int devindex, devlen;
   /* pushlistptr stackentry; (jdk) */

   cschem = cinst->thisobject;
   curobj = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;
 
   if (curobj->calls == NULL) {
      if ((updatenets(cinst, FALSE) <= 0) || (curobj->calls == NULL)) {
	 Wprintf("Error in generating netlists!");
	 return False;
      }
   }

   hptr = hiername;
   while (hptr != NULL) {
      nexttoken = strchr(hptr, '/');
      if (nexttoken != NULL) *nexttoken = '\0';
      pptr = strrchr(hptr, '(');
      if (pptr != NULL) {
	 if (sscanf(pptr + 1, "%d", &devindex) == 0) {
	    pptr = NULL;
	    devindex = 0;
	 }
	 else
	    *pptr = '\0';
      }
      else devindex = -1;

      /* check to make sure that the netlist and device indices have been generated */
      for (calls = curobj->calls; calls != NULL; calls = calls->next) {
	 if (calls->devindex == -1) {
	    cleartraversed(curobj);
	    resolve_indices(curobj, FALSE);
	 }
      }

      /* hptr is now the object name, and devindex is the instance's call index */
      /* find the object corresponding to the name.				*/

      newobj = NameToObject(hptr, &newinst, TRUE);

      if (newobj == NULL) {

	 /* Try device names (for netlist output) instead of object names */

	 for (calls = curobj->calls; calls != NULL; calls = calls->next) {
	    if (calls->devname != NULL) {
	       devlen = strlen(calls->devname);
	       if (!strncmp(hptr, calls->devname, devlen)) {
		  if (devindex == -1) {
		     if (sscanf(hptr + devlen, "%d", &devindex) == 0)
			devindex = 0;
		  }
		  if (calls->devindex == devindex) {
		     newobj = calls->callinst->thisobject;
		     break;
		  }
	       }
	    }
	 }
      }
      else {
         for (calls = curobj->calls; calls != NULL; calls = calls->next)
	    if ((calls->callobj == newobj) && (calls->devindex == devindex))
	       break;
      }
      if (newobj == NULL || calls == NULL) {
	 Fprintf(stderr, "object %s in hierarchy not found in schematic.\n", hptr);
	 free_stack(stack);
	 return FALSE; 
      }

      /* Diagnostic information */
      /* fprintf(stderr, "object %s(%d) = %s\n", newobj->name, devindex, hptr); */

      curobj = calls->callobj;
      push_stack(stack, calls->callinst, NULL);
      
      if (pptr != NULL) *pptr = '(';
      if (nexttoken == NULL) break;
      *nexttoken = '/';
      hptr = nexttoken + 1;
   }
   return TRUE;   
}

#endif

/*----------------------------------------------------------------------*/
/* Save netlist into a flattened sim or spice file			*/
/*----------------------------------------------------------------------*/

void writeflat(objectptr cschem, CalllistPtr cfrom, char *prefix, FILE *fp, char *mode)
{
   CalllistPtr calls = cschem->calls;
   char *newprefix = (char *)malloc(sizeof(char));

   /* reset device indexes */

   for (calls = cschem->calls; calls != NULL; calls = calls->next)
      calls->devindex = -1;

   /* The naming convention for nodes below the top level uses a	*/
   /* slash-separated list of hierarchical names.  Thus the call to	*/
   /* the hierarchical resolve_indices().  Indices used by bottom-most	*/
   /* symbols (e.g., flattened spice) will be generated by a separate	*/
   /* routine devflatindex(), unrelated to what is generated here, 	*/
   /* which only affects the hierarchical naming of nodes.		*/

   resolve_indices(cschem, FALSE);

   /* write all the subcircuits */

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {

      makelocalpins(cschem, calls, prefix);
      if (writedevice(fp, mode, cschem, calls, prefix) < 0) {
	 sprintf(_STR, "%s_%u", calls->callobj->name,
		devindex(cschem, calls));
	 newprefix = (char *)realloc(newprefix, sizeof(char) * (strlen(prefix)
		   + strlen(_STR) + 2));
	 sprintf(newprefix, "%s%s/", prefix, _STR);
	 /* Allow cross-referenced parameters between symbols and schematics */
	 /* by substituting into a schematic from its corresponding symbol   */
	 opsubstitute(calls->callobj, calls->callinst);
         /* psubstitute(calls->callinst); */
         writeflat(calls->callobj, calls, newprefix, fp, mode);
      }
      clearlocalpins(calls->callobj);
   }
   free(newprefix);
}

/*----------------------------------------------------------------------*/
/* Topmost call to write a flattened netlist				*/
/*----------------------------------------------------------------------*/

void topflat(objectptr cschem, objinstptr thisinst, CalllistPtr cfrom,
	char *prefix, FILE *fp, char *mode)
{
   char *locmode, *stsave = NULL;
   int modlen;
   Calllist loccalls;

   /* Set up local call list for parsing info labels in the top-level schematic */
   loccalls.cschem = NULL;
   loccalls.callobj = cschem;
   loccalls.callinst = thisinst;
   loccalls.devindex = -1;
   loccalls.ports = NULL;
   loccalls.next = NULL;

   modlen = strlen(mode);
   locmode = malloc(2 + modlen);
   strcpy(locmode, mode);
   locmode[modlen + 1] = '\0';

   /* "<mode>@" lines go in front */

   locmode[modlen] = '@';
   if (fp != NULL) stsave = parseinfo(NULL, cschem, &loccalls, NULL, locmode, FALSE,
		FALSE);
   if (stsave != NULL) {
      fputs(stsave, fp);
      fprintf(fp, "\n");
      free(stsave);
      stsave = NULL;
   }

   writeflat(cschem, cfrom, prefix, fp, mode);
   freeflatindex();

   /* Check for negative-numbered info labels, indicated output that	*/
   /* should be processed after everything else.			*/

   locmode[modlen] = '-';
   stsave = parseinfo(NULL, cschem, &loccalls, NULL, locmode, FALSE, FALSE);
   if (stsave != NULL) {
      fputs(stsave, fp);
      fprintf(fp, "\n");
      free(stsave);
      stsave = NULL;
   }
   free(locmode);
}

/*----------------------------------------------------------------------*/
/* Write out the list of global nets and their pin names		*/
/*----------------------------------------------------------------------*/

void writeglobals(objectptr cschem, FILE *fp)
{
   LabellistPtr llist;
   labelptr gpin;
   char *snew;

   if (fp == NULL) return;

   for (llist = global_labels; llist != NULL; llist = llist->next) {
      gpin = llist->label;
      snew = textprint(gpin->string, NULL);
      fprintf(fp, ".GLOBAL %s\n", snew);		/* hspice format */
      /* fprintf(fp, "%s = %d\n", snew, -gptr->netid); */ /* generic format */
      free(snew);
   }
   fprintf(fp, "\n");
}

/*----------------------------------------------------------------------*/
/* Write a SPICE subcircuit entry.					*/
/*----------------------------------------------------------------------*/

void writesubcircuit(FILE *fp, objectptr cschem)
{
   PortlistPtr ports;
   char *pstring;
   stringpart *ppin;
   int netid, length, plen, subnet; /*  portid, (jdk) */

   /* Objects which have no ports are not written */
   if ((cschem->ports != NULL) && (fp != NULL)) {

      fprintf(fp, ".subckt %s", cschem->name);
      length = 9 + strlen(cschem->name);

      /* List of parameters in subcircuit.			  */
      /* Each parameter connects to a net which may have multiple */
      /* names, so find the net associated with the parameter	  */
      /* and convert it back into a pin name in the usual manner  */
      /* using nettopin().					  */

      for (ports = cschem->ports; ports != NULL;
		ports = ports->next) {
         netid = ports->netid;
	 subnet = getsubnet(netid, cschem);
	 ppin = nettopin(netid, cschem, NULL);
	 pstring = textprintsubnet(ppin, NULL, subnet);
	 plen = strlen(pstring) + 1;
	 if (length + plen > 78) {
            fprintf(fp, "\n+ ");	/* SPICE line break & continuation */
	    length = 0;
	 }
	 else length += plen;
         fprintf(fp, " %s", pstring);
	 free(pstring);
      }
      fprintf(fp, "\n");
   }
}

/*----------------------------------------------------------------------*/
/* Resolve device names (fill calllist structure member "devname")	*/
/*----------------------------------------------------------------------*/

void resolve_devnames(objectptr cschem)
{
   CalllistPtr calls;
   char *stmp;
   oparamptr ops;

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {
      if (calls->callobj->traversed == False) {
         calls->callobj->traversed = True;
	 resolve_devnames(calls->callobj);
      }

      if (calls->devname == NULL) {

         /* Check for "class" parameter.  Note that although this	*/
         /* parameter may take a different instance value, the		*/
         /* device class is determined by the default value.		*/

         ops = find_param(calls->callinst, "class");
         if (ops && (ops->type == XC_STRING)) 
	    calls->devname = textprint(ops->parameter.string, NULL);
	 else {
	    /* The slow way---parse info labels for any device information */
            if ((stmp = parseinfo(cschem, calls->callinst->thisobject, calls,
			NULL, "", FALSE, TRUE)) != NULL)
	       free(stmp);
	 }
      }
   }
}

/*----------------------------------------------------------------------*/
/* Resolve (hierarchical) component numbering.  If "autonumber" is TRUE	*/
/* then this routine does auto-numbering.  Otherwise, it auto-generates	*/
/* device indices internally (in the "calllist" structure) but does not	*/
/* copy them into parameter space.					*/
/*									*/
/* Note: resolve_devindex() only operates on a single object.  Use	*/
/* resolve_indices() to operate on the entire hierarchy.		*/
/*----------------------------------------------------------------------*/

void resolve_devindex(objectptr cschem, Boolean autonumber)
{
   CalllistPtr calls;
   char *stmp, *endptr;
   char *idxtype[] = {"index", "idx", NULL};
   oparamptr ops, ips;
   objinstptr cinst;
   stringpart *optr;
   int newindex, j;

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {

      /* Check if there is an "index" parameter set to "?" in	*/
      /* the object with no existing instance value.		*/

      for (j = 0; idxtype[j] != NULL; j++)
         if ((ops = match_param(calls->callinst->thisobject, idxtype[j])) != NULL)
	    break;

      if (ops && (ops->type == XC_STRING)) {
 	 if (!textcomp(ops->parameter.string, "?", NULL)) {
	    cinst = calls->callinst;
	    ips = match_instance_param(cinst, idxtype[j]);
	    if ((autonumber == TRUE) && (ips == NULL)) {
	       copyparams(cinst, cinst);
	       ops = match_instance_param(cinst, idxtype[j]);
	       optr = ops->parameter.string;
	       newindex = devindex(cschem, calls);
	       stmp = d36a(newindex);
	       optr->data.string = (char *)realloc(optr->data.string, strlen(stmp) + 1);
	       sprintf(optr->data.string, "%s", stmp);
	    }
	    else if (calls->devindex < 0) {
	       if (ips != NULL) {
	          optr = ips->parameter.string;
		  if (optr->type != TEXT_STRING) {
		     /* Not a simple string.  Dump the text. */
		     char *snew = NULL;
		     snew = textprint(optr, NULL),
	             newindex = (u_int) strtol(snew, &endptr, 36);
		     free(snew);
		  }
		  else {
	             newindex = (u_int) strtol(optr->data.string, &endptr, 36);
		  }
	          if (*endptr != '\0') {
		     /* It is possible to get an instance value that is	*/
		     /* the same as the default, although this normally	*/
		     /* doesn't happen.  If it does, quietly remove the	*/
		     /* unneeded instance value.			*/

		     if (!stringcomp(ops->parameter.string, ips->parameter.string))
			resolveparams(cinst);
		     else
		        Fprintf(stderr, "Warning:  Use of non-alphanumeric"
				" characters in component \"%s%s\" (instance"
				" of %s)\n", (calls->devname) ? calls->devname
				: calls->callobj->name, optr->data.string,
				calls->callobj->name);
	          }
	          else
		     calls->devindex = newindex; 
	       }
	       else /* if (autonumber) */
	          devindex(cschem, calls);
	    }
	 }
      }
      else {
	 /* The slow way---parse info labels for any index information */
         if ((stmp = parseinfo(cschem, calls->callinst->thisobject, calls,
		NULL, "", autonumber, TRUE)) != NULL)
	    free(stmp);
      }
   }
}

/*----------------------------------------------------------------------*/
/* Recursive call to resolve_devindex() through the circuit hierarchy	*/
/*----------------------------------------------------------------------*/

void resolve_indices(objectptr cschem, Boolean autonumber) {
   CalllistPtr calls;

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {
      if (calls->callobj->traversed == False) {
         calls->callobj->traversed = True;
         resolve_indices(calls->callobj, autonumber);
      }
   }
   resolve_devindex(cschem, autonumber);
}

/*----------------------------------------------------------------------*/
/* Recursive call to clear all generated device numbers.		*/
/*----------------------------------------------------------------------*/

void clear_indices(objectptr cschem) {
   CalllistPtr calls;

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {
      if (calls->callobj->traversed == False) {
         calls->callobj->traversed = True;
         clear_indices(calls->callobj);
      }
      calls->devindex = -1;
   }
}

/*----------------------------------------------------------------------*/
/* Recursive call to clear all device indexes (parameter "index")	*/
/*----------------------------------------------------------------------*/

void unnumber(objectptr cschem) {
   CalllistPtr calls;
   oparamptr ops, ips;
   char *idxtype[] = {"index", "idx", NULL};
   int j;

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {
      if (calls->callobj->traversed == False) {
         calls->callobj->traversed = True;
         unnumber(calls->callobj);
      }

      for (j = 0; idxtype[j] != NULL; j++)
         if ((ops = match_param(calls->callobj, idxtype[j])) != NULL) break;

      if (ops && (ops->type == XC_STRING)) {
 	 if (!textcomp(ops->parameter.string, "?", NULL)) {
	    ips = match_instance_param(calls->callinst, idxtype[j]);
	    if (ips != NULL)
	       free_instance_param(calls->callinst, ips);
	 }
      }
   }
}

/*----------------------------------------------------------------------*/
/* Save netlist into a hierarchical file				*/
/*----------------------------------------------------------------------*/

void writehierarchy(objectptr cschem, objinstptr thisinst, CalllistPtr cfrom,
	FILE *fp, char *mode)
{
   CalllistPtr calls = cschem->calls;
   PortlistPtr ports, plist;
   int pnet, portid, length, plen, subnet, modlen; /* netid, (jdk) */
   char *locmode = mode, *stsave = NULL, *pstring;
   stringpart *ppin;
   Calllist loccalls;

   if (cschem->traversed == True) return;

   /* Set up local call list for parsing info labels in this schematic */
   loccalls.cschem = NULL;
   loccalls.callobj = cschem;
   loccalls.callinst = thisinst;
   loccalls.devindex = -1;
   loccalls.ports = NULL;
   loccalls.next = NULL;

   modlen = strlen(mode);
   locmode = malloc(2 + modlen);
   strcpy(locmode, mode);
   locmode[modlen + 1] = '\0';

   /* "<mode>@" lines go before any subcircuit calls	*/

   locmode[modlen] = '@';
   if (fp != NULL) stsave = parseinfo(NULL, cschem, &loccalls, NULL, locmode, FALSE,
		FALSE);
   if (stsave != NULL) {
      fputs(stsave, fp);
      fprintf(fp, "\n");
      free(stsave);
      stsave = NULL;
   }

   /* Subcircuits which make no calls or have no devices do not get written */

   if (calls != NULL) {

      /* Make sure that all the subcircuits have been written first */

      for (; calls != NULL; calls = calls->next) {
         if (calls->callobj->traversed == False) {
	    psubstitute(calls->callinst);
            writehierarchy(calls->callobj, calls->callinst, calls, fp, mode);
            calls->callobj->traversed = True;
         }
      }
      if (cschem->schemtype == FUNDAMENTAL) {
	 free(locmode);
	 return;
      }
   }

   /* Info-labels on a schematic (if any) get printed out first	*/

   if ((fp != NULL) && (cschem->calls != NULL)) {
      stsave = parseinfo(NULL, cschem, &loccalls, NULL, mode, FALSE, FALSE);
      if (stsave != NULL) {

	 /* Check stsave for embedded SPICE subcircuit syntax */

	 if (!strcmp(mode, "spice"))
	    if (strstr(stsave, ".subckt ") == NULL)
	       writesubcircuit(fp, cschem);

         fputs(stsave, fp);
         fprintf(fp, "\n");
         free(stsave);
         stsave = NULL;
      }
      else if (cschem->calls != NULL)
	 /* top-level schematics will be ignored because they have no ports */
	 writesubcircuit(fp, cschem);
   }

   /* Resolve all fixed part assignments (devindex) */
   resolve_devindex(cschem, FALSE);

   /* If the output file is NULL, then we're done */

   if (fp == NULL) {
      free(locmode);
      return;
   }

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {

      /* writedevice() is just another call to parseinfo() */
      if (writedevice(fp, mode, cschem, calls, NULL) < 0) {

	 /* If the device did not produce any output in writedevice(), but	*/
	 /* does not make any calls itself, then it is implicitly a TRIVIAL	*/
	 /* symbol.								*/

	 if ((calls->callobj->schemtype == TRIVIAL) || (calls->callobj->calls == NULL))
	    continue;

         /* No syntax indicating how to write this call.  Use SPICE "X"		*/
	 /* format and arrange the parameters according to the structure.	*/

	 calls->devname = strdup(spice_devname);
         fprintf(fp, "X%s", d36a(devindex(cschem, calls)));
	 stsave = calls->callobj->name;
         length = 6;

	 /* The object's definition lists calls in the order of the object's	*/
	 /* port list.  Therefore, use this order to find the port list.  	*/
	 /* This will also deal with the fact that not every port has to be	*/
	 /* called by the instance (ports may be left disconnected).		*/

	 for (ports = calls->callobj->ports; ports != NULL;
			ports = ports->next) {
	    portid = ports->portid;
	    for (plist = calls->ports; plist != NULL; plist = plist->next)
	       if (plist->portid == ports->portid)
		  break;

	    pnet = (plist == NULL) ?  netmax(cschem) + 1 : plist->netid;
	    subnet = getsubnet(pnet, cschem);
	    ppin = nettopin(pnet, cschem, NULL);
	    pstring = textprintsubnet(ppin, NULL, subnet);
	    plen = strlen(pstring) + 1;
	    if (length + plen > 78) {
               fprintf(fp, "\n+ ");	/* SPICE line continuation */
	       length = 0;
	    }
	    else length += plen;
	    fprintf(fp, " %s", pstring);
	    free(pstring);
         }
	 plen = 1 + strlen(stsave);
	 if (length + plen > 78) fprintf(fp, "\n+ ");	/* SPICE line cont. */
         fprintf(fp, " %s\n", stsave);
      }
   }

   /* Check for negative-numbered info labels, indicated output that	*/
   /* should be processed after everything else.  If we're processing	*/
   /* SPICE output but the schematic does not provide a ".ends"		*/
   /* statement, then add it to the end of the deck.			*/

   if (cschem->calls != NULL) {
      locmode[modlen] = '-';
      stsave = parseinfo(NULL, cschem, &loccalls, NULL, locmode, FALSE, FALSE);
      if (stsave != NULL) {
         fputs(stsave, fp);
         fprintf(fp, "\n");
         if (cfrom != NULL)
	    if (!strcmp(mode, "spice"))
	       if (strstr(stsave, ".ends") == NULL)
		   fprintf(fp, ".ends\n");
         free(stsave);
      }
      else if (cfrom != NULL)
         fprintf(fp, ".ends\n");

      fprintf(fp, "\n");
   }

   free(locmode);
}

/*----------------------------------------------------------------------*/
/* Create generic netlist in the Tcl interpreter variable space		*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER

static Tcl_Obj *tclparseinfo(objectptr cschem)
{
   genericptr *pgen;
   labelptr plabel;

   Tcl_Obj *rlist = Tcl_NewListObj(0, NULL);

   for (pgen = cschem->plist; pgen < cschem->plist + cschem->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 plabel = TOLABEL(pgen);
         if (plabel->pin == INFO) {
	    Tcl_ListObjAppendElement(xcinterp, rlist, 
			TclGetStringParts(plabel->string));
	 }
      }
   }
   return rlist;
}

/*----------------------------------------------------------------------*/
/* Write global variables to Tcl list (in key-value pairs which can be	*/
/* turned into an array variable using the "array set" command)		*/
/*									*/
/* Note: argument "cinst" is unused, but it really should be.  We	*/
/* should not assume that different schematics have the same list of	*/
/* globals!								*/
/*----------------------------------------------------------------------*/

Tcl_Obj *tclglobals(objinstptr cinst)
{
   LabellistPtr llist;
   labelptr gpin;
   Tcl_Obj *gdict; /*, *netnum; (jdk) */
   buslist *sbus;
   int netid, lbus;

   gdict = Tcl_NewListObj(0, NULL);
   for (llist = global_labels; llist != NULL; llist = llist->next) {
      gpin = llist->label;
      Tcl_ListObjAppendElement(xcinterp, gdict, TclGetStringParts(gpin->string));
      for (lbus = 0;;) {
         if (llist->subnets == 0) {
	    netid = llist->net.id;
	 }
	 else {
	    sbus = llist->net.list + lbus;
	    netid = sbus->netid;
	 }
         Tcl_ListObjAppendElement(xcinterp, gdict, Tcl_NewIntObj(netid));
	 if (++lbus >= llist->subnets) break;
      }
   }
   return gdict;
}

/*----------------------------------------------------------------------*/
/* Write a generic hierarchical netlist into Tcl list "subcircuits"	*/
/*----------------------------------------------------------------------*/

void tclhierarchy(objectptr cschem, objinstptr cinst, CalllistPtr cfrom, Tcl_Obj *cktlist)
{
   CalllistPtr calls = cschem->calls;
   PortlistPtr ports, plist;
   int netid, pnet, portid; /* , length, plen, i; (jdk) */
   Tcl_Obj *tclports, *tclcalls, *tclnewcall, *tclnets, *netnum;
   Tcl_Obj *tcldevs, *tclparams, *subckt, *newdev, *tcllabel, *portnum;
   oparamptr paramlist; /* , locparam; (jdk) */
   char *netsdone;

   /* Trivial objects are by definition those that are supposed	*/
   /* to be resolved by the netlist generation prior to output	*/
   /* and ignored by the output generator.			*/

   if (cschem->schemtype == TRIVIAL) return;

   /* Make sure that all the subcircuits have been written first */

   for (; calls != NULL; calls = calls->next) {
      if (calls->callobj->traversed == False) {
	 tclhierarchy(calls->callobj, calls->callinst, calls, cktlist);
         calls->callobj->traversed = True;
      }
   }

   /* Write own subcircuit netlist */
   subckt = Tcl_NewListObj(0, NULL);

   /* Prepare the list of network cross-references */
   tclnets = Tcl_NewListObj(0, NULL);

   /* Make list of the nets which have been written so we don't redundantly	*/
   /* list any entries (use of calloc() initializes all entries to FALSE).	*/
   /* (calloc() replaced by malloc() and bzero() because Tcl doesn't	*/
   /* have a calloc() function (2/6/04))				*/

   netsdone = (char *)malloc(2 + netmax(cschem));
#ifdef _MSC_VER
   memset((void *)netsdone, 0, 2 + netmax(cschem));
#else
   bzero((void *)netsdone, 2 + netmax(cschem));
#endif

   /* Write the name (key value pair) */
   Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj("name", 4));
   Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj(cschem->name,
		strlen(cschem->name)));

   /* Write the handle of the object instance (key value pair) */
   Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj("handle", 6));
   Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewHandleObj(cinst));

   /* Write the list of ports */
   if ((ports = cschem->ports) != NULL) {

      /* List of ports in subcircuit. 			  	  */
      /* Each parameter connects to a net which may have multiple */
      /* names, so find the net associated with the parameter     */
      /* and convert it back into a pin name in the usual manner  */
      /* using nettopin().					  */

      tclports = Tcl_NewListObj(0, NULL);
      for (; ports != NULL; ports = ports->next) {
         netid = ports->netid;
	 portid = ports->portid;
	 netnum = Tcl_NewIntObj(netid);
	 portnum = Tcl_NewIntObj(portid);
	 Tcl_ListObjAppendElement(xcinterp, tclports, portnum);
 	 Tcl_ListObjAppendElement(xcinterp, tclports, netnum);
	 if ((netid >= 0) && (netsdone[netid] == (char)0))
	 {
 	    Tcl_ListObjAppendElement(xcinterp, tclnets, netnum);
 	    Tcl_ListObjAppendElement(xcinterp, tclnets,
		TclGetStringParts(nettopin(netid, cschem, NULL)));
	    /* record this net as having been added to the list */
	    netsdone[netid] = (char)1;
	 }
      }
      Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj("ports", 5));
      Tcl_ListObjAppendElement(xcinterp, subckt, tclports);
   }

   /* Write the list of parameter defaults (key:value pairs) */

   if (cschem->params != NULL) {
      tclparams = Tcl_NewListObj(0, NULL);

      for (paramlist = cschem->params; paramlist != NULL; paramlist = paramlist->next) {
	 Tcl_ListObjAppendElement(xcinterp, tclparams,
		Tcl_NewStringObj(paramlist->key, strlen(paramlist->key)));
	 switch(paramlist->type) {
	    case XC_INT:
	       Tcl_ListObjAppendElement(xcinterp, tclparams,
			Tcl_NewIntObj((int)paramlist->parameter.ivalue));
	       break;
	    case XC_FLOAT:
	       Tcl_ListObjAppendElement(xcinterp, tclparams,
			Tcl_NewDoubleObj((double)paramlist->parameter.fvalue));
	       break;
	    case XC_STRING:
	       tcllabel = TclGetStringParts(paramlist->parameter.string);
	       /* Should get rid of last entry, "End Parameter"; not needed */
	       Tcl_ListObjAppendElement(xcinterp, tclparams, tcllabel);
	       break;
	    case XC_EXPR:
	       Tcl_ListObjAppendElement(xcinterp, tclparams,
			evaluate_raw(cschem, paramlist, cinst, NULL));
	       break;	
	 }
      }
      Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj("parameters", 10));
      Tcl_ListObjAppendElement(xcinterp, subckt, tclparams);
   }

   /* Write the list of calls to subcircuits */

   if ((calls = cschem->calls) != NULL) {
      tclcalls = Tcl_NewListObj(0, NULL);
      for (; calls != NULL; calls = calls->next) {

         /* Don't write calls to non-functional subcircuits. */
	 if (calls->callobj->schemtype == TRIVIAL) continue;

         tclnewcall = Tcl_NewListObj(0, NULL);
	 Tcl_ListObjAppendElement(xcinterp, tclnewcall, Tcl_NewStringObj("name", 4));
	 Tcl_ListObjAppendElement(xcinterp, tclnewcall,
		Tcl_NewStringObj(calls->callobj->name,
		strlen(calls->callobj->name)));
	 

         /* Log any local parameter instances */
         if (calls->callinst->params != NULL) {
	    tclparams = Tcl_NewListObj(0, NULL);

	    for (paramlist = calls->callinst->params; paramlist != NULL;
			paramlist = paramlist->next) {
	       Tcl_ListObjAppendElement(xcinterp, tclparams,
			Tcl_NewStringObj(paramlist->key, strlen(paramlist->key)));
	       switch(paramlist->type) {
		  case XC_INT:
	             Tcl_ListObjAppendElement(xcinterp, tclparams,
				Tcl_NewIntObj((int)paramlist->parameter.ivalue));
		     break;
		  case XC_FLOAT:
	             Tcl_ListObjAppendElement(xcinterp, tclparams,
				Tcl_NewDoubleObj((double)paramlist->parameter.fvalue));
		     break;
		  case XC_STRING:
	             Tcl_ListObjAppendElement(xcinterp, tclparams,
				TclGetStringParts(paramlist->parameter.string));
		     break;
		  case XC_EXPR:
		     Tcl_ListObjAppendElement(xcinterp, tclparams,
				evaluate_raw(cschem, paramlist, cinst, NULL));
		     break;
	       }
	    }
	    Tcl_ListObjAppendElement(xcinterp, tclnewcall,
			Tcl_NewStringObj("parameters", 10));
	    Tcl_ListObjAppendElement(xcinterp, tclnewcall, tclparams);
         }

	 /* Called ports are listed by key:value pair (port number: net id) */

         if (calls->callobj->ports != NULL) {
            tclports = Tcl_NewListObj(0, NULL);
	    for (ports = calls->callobj->ports; ports != NULL;
			ports = ports->next) {
	       portid = ports->portid;
	       for (plist = calls->ports; plist != NULL; plist = plist->next)
	          if (plist->portid == ports->portid)
		     break;

	       pnet = (plist == NULL) ?  netmax(cschem) + 1 : plist->netid;

	       netnum = Tcl_NewIntObj((int)pnet); 
	       portnum = Tcl_NewIntObj(portid);
	       Tcl_ListObjAppendElement(xcinterp, tclports, portnum);
	       Tcl_ListObjAppendElement(xcinterp, tclports, netnum);
	       if ((pnet >= 0) && (netsdone[pnet] == (char)0))
	       {
		  Tcl_ListObjAppendElement(xcinterp, tclnets, netnum);
		  Tcl_ListObjAppendElement(xcinterp, tclnets,
			TclGetStringParts(nettopin(pnet, cschem, NULL)));
		  /* record this net as having been added to the list */
		  netsdone[pnet] = (char)1;
	       }
            }
	    Tcl_ListObjAppendElement(xcinterp, tclnewcall,
			Tcl_NewStringObj("ports", 5));
	    Tcl_ListObjAppendElement(xcinterp, tclnewcall, tclports);
         }
	 Tcl_ListObjAppendElement(xcinterp, tclcalls, tclnewcall);
      }
      Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj("calls", 5));
      Tcl_ListObjAppendElement(xcinterp, subckt, tclcalls);
   }
   free(netsdone);

   /* If the object has info labels, write a device list.	*/
   /* Check both the schematic and its symbol for info labels.	*/

   tcldevs = Tcl_NewListObj(0, NULL);
   if (cschem->symschem != NULL) {
      newdev = tclparseinfo(cschem->symschem);
      Tcl_ListObjAppendElement(xcinterp, tcldevs, newdev);
   }
   newdev = tclparseinfo(cschem);
   Tcl_ListObjAppendElement(xcinterp, tcldevs, newdev);
   Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj("devices", 7));
   Tcl_ListObjAppendElement(xcinterp, subckt, tcldevs);

   /* Write the network cross-reference dictionary */
   Tcl_ListObjAppendElement(xcinterp, subckt, Tcl_NewStringObj("nets", 4));
   Tcl_ListObjAppendElement(xcinterp, subckt, tclnets);

   Tcl_ListObjAppendElement(xcinterp, cktlist, subckt);
}

/*----------------------------------------------------------------------*/

Tcl_Obj *tcltoplevel(objinstptr cinst)
{
   Tcl_Obj *cktlist;
   objectptr cschem = cinst->thisobject;

   cktlist = Tcl_NewListObj(0, NULL);
   cleartraversed(cschem);
   tclhierarchy(cschem, cinst, NULL, cktlist);
   return cktlist;
}

#endif	/* TCL_WRAPPER */

/*----------------------------------------------------------------------*/
/* Create generic netlist in the Python interpreter variable space	*/
/*----------------------------------------------------------------------*/

#ifdef HAVE_PYTHON

static PyObject *pyparseinfo(objectptr cschem)
{
   genericptr *pgen;
   labelptr plabel;

   PyObject *rlist = PyList_New(0);

   for (pgen = cschem->plist; pgen < cschem->plist + cschem->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 plabel = TOLABEL(pgen);
         if (plabel->pin == INFO)
	    PyList_Append(rlist, PyGetStringParts(plabel->string));
      }
   }
   return rlist;
}

/*----------------------------------------------------------------------*/
/* Write global variables to Python dictionary				*/
/*----------------------------------------------------------------------*/

PyObject *pyglobals(objectptr cschem)
{
   LabellistPtr llist;
   labelptr gpin;
   PyObject *gdict, *netnum;
   int netid, lbus;
   buslist *sbus;

   gdict = PyDict_New();
   for (llist = global_labels; llist != NULL; llist = llist->next) {
      gpin = llist->label;
      for (lbus = 0;;) {
	 if (llist->subnets == 0)
	    netid = llist->net.id;
	 else {
	    sbus = llist->net.list + lbus;
	    netid = sbus->netid;
	 }
         netnum = PyInt_FromLong((long)(netid));
         PyDict_SetItem(gdict, netnum, PyGetStringParts(gpin->string));
	 if (++lbus >= llist->subnets) break;
      }
   }
   return gdict;
}

/*----------------------------------------------------------------------*/
/* Write a generic hierarchical netlist into Python list "subcircuits"	*/
/*----------------------------------------------------------------------*/

void pyhierarchy(objectptr cschem, CalllistPtr cfrom, PyObject *cktlist)
{
   CalllistPtr calls = cschem->calls;
   PortlistPtr ports, plist;
   int netid, pnet, portid, length, plen, i;
   PyObject *pyports, *pycalls, *pynewcall, *pynets, *netnum;
   PyObject *pydevs, *pyparams, *subckt, *newdev, *pylabel;
   oparamptr paramlist;

   /* Trivial objects are by definition those that are supposed	*/
   /* to be resolved by the netlist generation prior to output	*/
   /* and ignored by the output generator.			*/

   if (cschem->schemtype == TRIVIAL) return;

   /* Make sure that all the subcircuits have been written first */

   for (; calls != NULL; calls = calls->next) {
      if (calls->callobj->traversed == False) {
         calls->callobj->traversed = True;
	 pyhierarchy(calls->callobj, calls, cktlist);
      }
   }

   /* Write own subcircuit netlist */
   subckt = PyDict_New();

   /* Prepare the dictionary of network cross-references */
   pynets = PyDict_New();

   /* Write the name */
   PyDict_SetItem(subckt, PyString_FromString("name"),
		PyString_FromString(cschem->name));

   /* Write the list of ports */
   if ((ports = cschem->ports) != NULL) {

      /* List of ports in subcircuit. 			  	  */
      /* Each parameter connects to a net which may have multiple */
      /* names, so find the net associated with the parameter     */
      /* and convert it back into a pin name in the usual manner  */
      /* using nettopin().					  */

      pyports = PyList_New(0);
      for (; ports != NULL; ports = ports->next) {
         netid = ports->netid;
	 netnum = PyInt_FromLong((long)netid);
         PyList_Append(pyports, netnum);
	 if (netid >= 0)
	    PyDict_SetItem(pynets, netnum,
		PyGetStringParts(nettopin(netid, cschem, NULL)));
      }
      PyDict_SetItem(subckt, PyString_FromString("ports"), pyports);
   }

   /* Write the list of parameters */

   if (cschem->params != NULL) {
      pyparams = PyList_New(0);

      for (paramlist = cschem->params; paramlist != NULL; paramlist = paramlist->next) {
	 if (paramlist->type == XC_INT)
	    PyList_Append(pyparams,
			PyInt_FromLong((long)paramlist->parameter.ivalue));
	 if (paramlist->type == XC_FLOAT)
	    PyList_Append(pyparams,
			PyFloat_FromDouble((double)paramlist->parameter.fvalue));
	 else if (paramlist->type == XC_STRING) {
	    pylabel = PyGetStringParts(paramlist->parameter.string);
	    /* Should get rid of last entry, "End Parameter"; not needed */
	    PyList_Append(pyparams, pylabel);
	 }
      }
      PyDict_SetItem(subckt, PyString_FromString("parameters"), pyparams);
   }

   /* Write the list of calls to subcircuits */

   if ((calls = cschem->calls) != NULL) {
      pycalls = PyList_New(0);
      for (; calls != NULL; calls = calls->next) {

         /* Don't write calls to non-functional subcircuits. */
	 if (calls->callobj->schemtype == TRIVIAL) continue;

         pynewcall = PyDict_New();
         PyDict_SetItem(pynewcall, PyString_FromString("name"),
		PyString_FromString(calls->callobj->name));

         /* Log any local parameter instances */
         if (calls->callinst->params != NULL) {
	    pyparams = PyList_New(0);

	    for (paramlist = cschem->params; paramlist != NULL;
			paramlist = paramlist->next) {
	       if (paramlist->type == XC_INT)
	          PyList_Append(pyparams,
			PyInt_FromLong((long)paramlist->parameter.ivalue));
	       else if (paramlist->type == XC_FLOAT)
	          PyList_Append(pyparams,
			PyFloat_FromDouble((double)paramlist->parameter.fvalue));
	       else if (paramlist->type == XC_STRING)
	          PyList_Append(pyparams,
			PyGetStringParts(paramlist->parameter.string));
	    }
	    PyDict_SetItem(pynewcall, PyString_FromString("parameters"),
			pyparams);
         }

         /* The object's definition lists calls in the order of the object's	*/
         /* port list.  Therefore, use this order to find the port list.  	*/
	 /* Unconnected ports will be NULL entries in the list (?).		*/

         if (calls->callobj->ports != NULL) {
            pyports = PyList_New(0);
	    for (ports = calls->callobj->ports; ports != NULL;
			ports = ports->next) {
	       portid = ports->portid;
	       for (plist = calls->ports; plist != NULL; plist = plist->next)
	          if (plist->portid == ports->portid)
		     break;

	       pnet = (plist == NULL) ?  netmax(cschem) + 1 : plist->netid;

	       netnum = PyInt_FromLong((long)pnet); 
	       PyList_Append(pyports, netnum);
	       if (pnet >= 0)
	          PyDict_SetItem(pynets, netnum,
			PyGetStringParts(nettopin(pnet, cschem, NULL)));
            }
	    PyDict_SetItem(pynewcall, PyString_FromString("ports"), pyports);
         }
         PyList_Append(pycalls, pynewcall);
      }
      PyDict_SetItem(subckt, PyString_FromString("calls"), pycalls);
   }

   /* If the object has info labels, write a device list.	*/
   /* Check both the schematic and its symbol for info labels.	*/

   pydevs = PyList_New(0);
   if (cschem->symschem != NULL) {
      newdev = pyparseinfo(cschem->symschem);
      PyList_Append(pydevs, newdev);
   }
   newdev = pyparseinfo(cschem);
   PyList_Append(pydevs, newdev);
   PyDict_SetItem(subckt, PyString_FromString("devices"), pydevs);

   /* Write the network cross-reference dictionary */
   PyDict_SetItem(subckt, PyString_FromString("nets"), pynets);

   PyList_Append(cktlist, subckt);
}

/*----------------------------------------------------------------------*/

PyObject *pytoplevel(objectptr cschem)
{
   PyObject *cktlist;

   cktlist = PyList_New(0);
   cleartraversed(cschem);
   pyhierarchy(cschem, NULL, cktlist);
   return cktlist;
}

#endif	/* HAVE_PYTHON */

/*----------------------------------------------------------------------*/
/* Update networks in an object by checking if the network is valid,	*/
/* and destroying and recreating the network if necessary.		*/
/*									*/
/* Run cleartraversed() on all types.  The traversed flag allows a	*/
/* check for infinite recursion when creating calls.			*/
/*									*/
/* Returns -1 on discovery of infinite recursion, 0 on failure to	*/
/* generate a net, and 1 on success.					*/
/*----------------------------------------------------------------------*/

int updatenets(objinstptr uinst, Boolean quiet) {
   objectptr thisobject;
   objinstptr thisinst;
   int spage;

   /* Never try to generate a netlist while a file is being read, as	*/
   /* can happen if an expression parameter attempts to query a netlist	*/
   /* node.								*/

   if (load_in_progress) return 0;	

   if (uinst->thisobject->symschem != NULL
		&& uinst->thisobject->schemtype != PRIMARY) {
      thisobject = uinst->thisobject->symschem;
      if ((spage = is_page(thisobject)) >= 0)
         thisinst = xobjs.pagelist[spage]->pageinst;
   }
   else {
      thisobject = uinst->thisobject;
      thisinst = uinst;
   }

   if (checkvalid(thisobject) == -1) {
      uselection *ssave;

      if (cleartraversed(thisobject) == -1) {
         Wprintf("Netlist error:  Check for recursion in circuit!");
         return -1;
      }

      /* Note: destroy/create nets messes up the part list.  Why?? */

      if (areawin->selects > 0)
         ssave = remember_selection(areawin->topinstance, areawin->selectlist,
		areawin->selects);
      destroynets(thisobject);
      createnets(thisinst, quiet);
      if (areawin->selects > 0) {
         areawin->selectlist = regen_selection(areawin->topinstance, ssave);
         free_selection(ssave);
      }
   }

   if (thisobject->labels == NULL && thisobject->polygons == NULL) {
      if (quiet == FALSE)
         Wprintf("Netlist error:  No netlist elements in object %s",
		thisobject->name);
      return 0;
   }
   return 1;
}

/*----------------------------------------------------------------------*/
/* Generate netlist, write information according to mode, and then	*/
/* destroy the netlist (this will be replaced eventually with a dynamic	*/
/* netlist model in which the netlist changes according to editing of	*/
/* individual elements, not created and destroyed wholesale)		*/
/*----------------------------------------------------------------------*/

void writenet(objectptr thisobject, char *mode, char *suffix)
{
   objectptr cschem;
   objinstptr thisinst;
   char filename[100];
   char *prefix, *cpos, *locmode = mode, *stsave = NULL;
   FILE *fp;
   /* Calllist topcell; (jdk) */
   Boolean is_spice = FALSE, sp_end_save = spice_end;

   /* Always use the master schematic, if there is one. */

   if (thisobject->schemtype == SECONDARY)
      cschem = thisobject->symschem;
   else
      cschem = thisobject;

   /* Update the netlist if this has not been done */

   if (NameToPageObject(cschem->name, &thisinst, NULL) == NULL) {
      Wprintf("Not a schematic. . . cannot generate output!\n");
      return;
   }
   if (updatenets(thisinst, FALSE) <= 0) {
      Wprintf("No file written!");
      return;
   }

   prefix = (char *)malloc(sizeof(char));
   *prefix = '\0';
 
   if ((cpos = strchr(cschem->name, ':')) != NULL) *cpos = '\0';
   sprintf(filename, "%s.%s", cschem->name, suffix);
   if (cpos != NULL) *cpos = ':';

   if (!strncmp(mode, "index", 5)) {	/* This mode generates no output */
      locmode += 5;
      fp = (FILE *)NULL;
   }
   else if ((fp = fopen(filename, "w")) == NULL) {
      Wprintf("Could not open file %s for writing.", filename);
      free(prefix);
      return;
   }

   /* Clear device indices from any previous netlist output */
   cleartraversed(cschem);
   clear_indices(cschem);

   /* Make sure list of include-once files is empty */

   free_included();

   /* Handle different netlist modes */

   if (!strcmp(mode, "spice")) {

      if (thisobject->schemtype == SYMBOL)
	 cschem = thisobject->symschem;

      is_spice = TRUE;
      fprintf(fp, "*SPICE %scircuit <%s> from XCircuit v%s rev %s\n\n",
		(thisobject->schemtype == SYMBOL) ? "sub" : "",
		cschem->name, PROG_VERSION, PROG_REVISION);

      /* writeglobals(cschem, fp); */
      cleartraversed(cschem);
      writehierarchy(cschem, thisinst, NULL, fp, mode);
   }
   else if (!strcmp(mode, "flatspice")) {
      is_spice = TRUE;
      fprintf(fp, "*SPICE (flattened) circuit \"%s\" from XCircuit v%s rev %s\n\n",
		cschem->name, PROG_VERSION, PROG_REVISION);
      if (stsave != NULL) {
	 fputs(stsave, fp);
	 fprintf(fp, "\n");
      }
      topflat(cschem, thisinst, NULL, prefix, fp, mode);
   }
   else if (!strcmp(mode, "pseuspice")) {
      is_spice = TRUE;
      fprintf(fp, "*SPICE subcircuit \"%s\" from XCircuit v%s rev %s\n\n",
		cschem->name, PROG_VERSION, PROG_REVISION);
      if (stsave != NULL) {
	 fputs(stsave, fp);
	 fprintf(fp, "\n");
      }
      writeflat(cschem, NULL, prefix, fp, mode);
      freeflatindex();
   }
   else if (!strcmp(mode, "flatsim") || !strcmp(mode, "pseusim")) {
      fprintf(fp, "| sim circuit \"%s\" from XCircuit v%s rev %s\n",
		cschem->name, PROG_VERSION, PROG_REVISION);
      if (stsave != NULL) {
	 fputs(stsave, fp);
	 fprintf(fp, "\n");
      }
      topflat(cschem, thisinst, NULL, prefix, fp, mode);
   }
   else if (!strcmp(locmode, "pcb")) {
      struct Ptab *ptable = (struct Ptab *)NULL;
      writepcb(&ptable, cschem, NULL, "", mode);
      if (stsave != NULL) {
	 fputs(stsave, fp);
	 fprintf(fp, "\n");
      }
      outputpcb(ptable, fp);
      freepcb(ptable);
   }
   else if (!strncmp(mode, "flat", 4)) {
      /* User-defined modes beginning with the word "flat":		*/
      /* Assume a flattened netlist, and assume nothing else.		*/

      if (thisobject->schemtype == SYMBOL)
	 cschem = thisobject->symschem;
      cleartraversed(cschem);
      writeflat(cschem, NULL, prefix, fp, mode);
      freeflatindex();
   }
   else if (!strncmp(mode, "pseu", 4)) {
      /* User-defined modes beginning with the word "pseu":		*/
      /* Assume a flattened netlist for everything under the top level.	*/

      if (thisobject->schemtype == SYMBOL)
	 cschem = thisobject->symschem;
      cleartraversed(cschem);
      topflat(cschem, thisinst, NULL, prefix, fp, mode);
   }
   else {
      /* All user-defined modes:  Assume a hierarchical netlist, and	*/
      /* assume nothing else.						*/

      if (thisobject->schemtype == SYMBOL)
	 cschem = thisobject->symschem;
      cleartraversed(cschem);
      writehierarchy(cschem, thisinst, NULL, fp, mode);
   }

   /* Finish up SPICE files with a ".end" statement (if requested) */
   if (is_spice && (spice_end == True)) fprintf(fp, ".end\n");
   spice_end = sp_end_save;

   /* Finish up */

   if (fp != NULL) {
      fclose(fp);
      Wprintf("%s netlist saved as %s", mode, filename);
   }
   if (stsave != NULL) free(stsave);
   free(prefix);
}

/*----------------------------------------------------------------------*/
/* Flatten netlist and save into a table of pcb-style nets		*/
/* The routine is recursive.  Each call returns TRUE if some nested	*/
/* call generated output;  this is a reasonable way to tell if we have	*/
/* reached the bottom of the hierarchy.					*/
/*----------------------------------------------------------------------*/

Boolean writepcb(struct Ptab **ptableptr, objectptr cschem, CalllistPtr cfrom,
		char *prefix, char *mode)
{
   PolylistPtr plist;
   LabellistPtr llist;
   CalllistPtr calls;
   PortlistPtr ports;
   int i, testnet, tmplen, subnet;
   char *newprefix = (char *)malloc(sizeof(char));
   char *sout, *snew;
   struct Ptab *hidx, *htmp;
   struct Pstr *tmpstr;
   struct Pnet *tmpnet;
   objinstptr cinst;
   buslist *sbus;
   int locnet, lbus;
   Boolean outputdone = FALSE;
   Boolean outputcall;
 
   /* Step 1A:  Go through the polygons of this object and add	*/
   /* 		any unvisited nets to the table.		*/

   for (plist = cschem->polygons; plist != NULL; plist = plist->next) {
      for (lbus = 0;;) {
	 if (plist->subnets == 0)
	    testnet = plist->net.id;
	 else {
	    sbus = plist->net.list + lbus;
	    testnet = sbus->netid;
	 }

         hidx = *ptableptr;
         while (hidx != NULL) {
            if (hidx->nets != NULL) {
	       for (i = 0; i < hidx->nets->numnets; i++)
	          if (*(hidx->nets->netidx + i) == testnet) break;
	       if (i < hidx->nets->numnets) break;
	    }
	    hidx = hidx->next;
         }
         if (hidx == NULL) {	/* make new entry for net in table */
	    htmp = (struct Ptab *)malloc(sizeof(struct Ptab));
	    tmpnet = (struct Pnet *)malloc(sizeof(struct Pnet));
	    tmpnet->numnets = 1;
	    tmpnet->netidx = (int *)malloc(sizeof(int));
	    *(tmpnet->netidx) = testnet;
	    tmpnet->next = NULL;
	    htmp->cschem = cschem;
	    htmp->nets = tmpnet;
	    htmp->pins = NULL;
	    htmp->next = *ptableptr;
	    *ptableptr = htmp;
	    /* Fprintf(stdout, "Added new index entry for net %d\n", testnet); */
         }
	 if (++lbus >= plist->subnets) break;
      }
   }

   /* Step 1B:  Do the same thing for labels.			*/

   for (llist = cschem->labels; llist != NULL; llist = llist->next) {
      for (lbus = 0;;) {
	 if (llist->subnets == 0)
	    testnet = llist->net.id;
	 else {
	    sbus = llist->net.list + lbus;
	    testnet = sbus->netid;
	 }

         hidx = *ptableptr;
         while (hidx != NULL) {
            if (hidx->nets != NULL) {
	       for (i = 0; i < hidx->nets->numnets; i++)
	          if (*(hidx->nets->netidx + i) == testnet) break;
	       if (i < hidx->nets->numnets) break;
	    }
	    hidx = hidx->next;
         }
         if (hidx == NULL) {	/* make new entry for net in table */
	    htmp = (struct Ptab *)malloc(sizeof(struct Ptab));
	    tmpnet = (struct Pnet *)malloc(sizeof(struct Pnet));
	    tmpnet->numnets = 1;
	    tmpnet->netidx = (int *)malloc(sizeof(int));
	    *(tmpnet->netidx) = testnet;
	    tmpnet->next = NULL;
	    htmp->cschem = cschem;
	    htmp->nets = tmpnet;
	    htmp->pins = NULL;
	    htmp->next = *ptableptr;
	    *ptableptr = htmp;
	    /* Fprintf(stdout, "Added new index entry for net %d\n", testnet); */
         }
	 if (++lbus >= llist->subnets) break;
      }
   }

   /* Step 2:  Resolve fixed device indices */
   resolve_devindex(cschem, FALSE);

   /* Step 3:  Go through the list of calls to search for endpoints */

   for (calls = cschem->calls; calls != NULL; calls = calls->next) {
      objectptr cthis;

      cinst = calls->callinst;

      /* Trivial objects should have been dealt with already.		*/
      /* If we don't continue the loop here, you get netlist output for	*/
      /* objects like dots and circles.					*/
      if (calls->callobj->schemtype == TRIVIAL) continue;

      /* Step 4:  If call is to a bottom-most schematic, output the device connections */
      /* Info-label can provide an alternate name or specify the instance number */

      cthis = calls->callobj;
      if (calls->callobj->schemtype == PRIMARY || calls->callobj->schemtype ==
		SECONDARY)
	 if (calls->callobj->symschem != NULL)
	    cthis = calls->callobj->symschem;

      if ((sout = parseinfo(cschem, cthis, calls, prefix, mode, FALSE, TRUE)) == NULL) {
	 if (calls->devname == NULL)
	    calls->devname = strdup(calls->callinst->thisobject->name);
         sprintf(_STR, "%s_%s", calls->devname, d36a(devindex(cschem, calls)));
      }
      else {
	 sscanf(sout, "%s", _STR);	/* Copy the first word out of sout */
      }
      newprefix = (char *)realloc(newprefix, sizeof(char) * (strlen(prefix)
		+ strlen(_STR) + 2));
      sprintf(newprefix, "%s%s/", prefix, _STR);

      /*----------------------------------------------------------------*/
      /* Parsing of the "pcb:" info label---				*/
      /* Find all <net>=<name> strings and create the appropriate nets	*/
      /*----------------------------------------------------------------*/

      if (sout) {
	 char rsave, *lhs, *rhs, *rend, *sptr = sout, *tmppinname;

	 while ((rhs = strchr(sptr, '=')) != NULL) {
	    Genericlist *implicit, newlist;
	    struct Pstr *psrch = NULL;

	    lhs = rhs - 1;
	    while ((lhs >= sptr) && isspace(*lhs)) lhs--;
	    while ((lhs >= sptr) && !isspace(*lhs)) lhs--;
	    *rhs = '\0';
	    rhs++;
	    if (isspace(*lhs)) lhs++;
	    while ((*rhs) && isspace(*rhs)) rhs++;
	    for (rend = rhs; (*rend) && (!isspace(*rend)); rend++);
	    rsave = *rend;
	    *rend = '\0';

	    /* Get the net from the equation RHS.  If none exists, assume  */
	    /* the name is a global net and add it to the list of globals. */

	    implicit = nametonet(cschem, cinst, rhs);
	    if (implicit == NULL) {
	       stringpart *strptr;
	       label templabel;

	       labeldefaults(&templabel, GLOBAL, 0, 0);
	       strptr = templabel.string;
	       strptr->type = TEXT_STRING;
	       strptr->data.string = strdup(rhs);
	       newlist.subnets = 0;
	       newlist.net.id = globalmax() - 1;
	       addglobalpin(cschem, cinst, &templabel, &newlist);
	       implicit = &newlist;
	       free(strptr->data.string);
	    }

	    /* Get the name of the pin */
	    tmppinname = (char *)malloc(strlen(newprefix) + strlen(lhs) + 2);
	    strcpy(tmppinname, newprefix);
	    if ((tmplen = strlen(newprefix)) > 0) tmppinname[tmplen - 1] = '-';
	    strcat(tmppinname, lhs);

	    /* Find the net in the ptable, or create a new entry for it */

	    hidx = *ptableptr;
	    while (hidx != NULL) {
	       if (hidx->nets != NULL) {
		  for (i = 0; i < hidx->nets->numnets; i++)
		     if (*(hidx->nets->netidx + i) == implicit->net.id)
			 break;
		  if (i < hidx->nets->numnets) break;
	       }
	       hidx = hidx->next;
	    }
	    if (hidx == NULL) {
	       htmp = (struct Ptab *)malloc(sizeof(struct Ptab));
	       tmpnet = (struct Pnet *)malloc(sizeof(struct Pnet));
	       tmpnet->numnets = 1;
	       tmpnet->netidx = (int *)malloc(sizeof(int));
	       *(tmpnet->netidx) = implicit->net.id;
	       tmpnet->next = NULL;
	       htmp->cschem = cschem;
	       htmp->nets = tmpnet;
	       htmp->pins = NULL;
	       htmp->next = *ptableptr;
	       *ptableptr = htmp;
	       hidx = htmp;
	    }
	    else {
	       /* Check if any entries are the same as tmppinname */
	       for (psrch = hidx->pins; psrch != NULL; psrch = psrch->next) {
		  if (!strcmp(psrch->string->data.string, tmppinname))
		     break;
	       }
	    }

	    /* Get the pin name from the equation LHS */ 
	    if (psrch == NULL) {
	       tmpstr = (struct Pstr *)malloc(sizeof(struct Pstr));
	       tmpstr->string = (stringpart *)malloc(sizeof(stringpart));
	       tmpstr->string->type = TEXT_STRING;
	       tmpstr->string->nextpart = NULL;
	       tmpstr->string->data.string = tmppinname;
	       tmpstr->next = hidx->pins;
	       hidx->pins = tmpstr;
	    }
	    else {
	       free(tmppinname);
	    }

	    /* Proceed to the next LHS=RHS pair */
	    *rend = rsave;
	    sptr = rend;
	 }
	 free(sout);
      }

      outputcall = FALSE;
      if (calls->callobj->calls != NULL) {

         /* Step 4A: Push current net translations */
	 /* (Don't push or pop global net numbers:  no translation needed!) */

         hidx = *ptableptr;
         while (hidx != NULL) {
            if ((hidx->nets != NULL) && (((hidx->nets->numnets > 0) &&
			(*(hidx->nets->netidx) >= 0)) || (hidx->nets->numnets == 0))) {
               tmpnet = (struct Pnet *)malloc(sizeof(struct Pnet));
	       tmpnet->numnets = 0;
               tmpnet->netidx = NULL;
               tmpnet->next = hidx->nets;
               hidx->nets = tmpnet;
            }
            hidx = hidx->next;
         }

	 /* Step 4B: Generate net translation table for each subcircuit */

         for (ports = calls->ports; ports != NULL; ports = ports->next) {
	    for (hidx = *ptableptr; hidx != NULL; hidx = hidx->next) {
               if (hidx->nets != NULL) {
	          if (hidx->nets->next != NULL) {
		     for (i = 0; i < hidx->nets->next->numnets; i++)
		        if (*(hidx->nets->next->netidx + i) == ports->netid)
			   break;
		     if (i < hidx->nets->next->numnets) break;
		  }
		  else if (ports->netid < 0) {
		     if (hidx->nets->netidx != NULL)
			if (*(hidx->nets->netidx) == ports->netid)
			   break;
		  }
	       }
            }
	    if (hidx != NULL) {
	       hidx->nets->numnets++;
	       if (hidx->nets->numnets == 1)
		  hidx->nets->netidx = (int *)malloc(sizeof(int));
	       else
		  hidx->nets->netidx = (int *)realloc(hidx->nets->netidx,
			hidx->nets->numnets * sizeof(int));

	       /* Translate net value */
	       locnet = translatedown(ports->netid, ports->portid,
				calls->callobj);
	       *(hidx->nets->netidx + hidx->nets->numnets - 1) = locnet;

	       /* Fprintf(stdout, "Translation: net %d in object %s is net "
			"%d in object %s\n", ports->netid, cschem->name,
			locnet, calls->callobj->name); */
	    }
         }

         /* Step 4C: Run routine recursively on the subcircuit */

	 /* Fprintf(stdout, "Recursive call of writepcb() to %s\n",
		calls->callobj->name); */
         outputcall = writepcb(ptableptr, calls->callobj, calls, newprefix, mode);

         /* Step 4D: Pop the translation table */
	 /* (Don't pop global nets (designated by negative net number)) */

         hidx = *ptableptr;
         while (hidx != NULL) {
            if ((hidx->nets != NULL) && (((hidx->nets->numnets > 0) &&
			(*(hidx->nets->netidx) >= 0)) || (hidx->nets->numnets == 0))) {
	       tmpnet = hidx->nets->next;
	       if (hidx->nets->numnets > 0) free(hidx->nets->netidx);
	       free(hidx->nets);
	       hidx->nets = tmpnet;
            }
            hidx = hidx->next;
         }
      }

      if (!outputcall) {
	 stringpart *ppin;

         /* Fprintf(stdout, "Reached lowest-level schematic:  Finding connections\n"); */
	 for (ports = calls->ports; ports != NULL; ports = ports->next) {
	    locnet = translatedown(ports->netid, ports->portid, calls->callobj);
	    /* Global pin names should probably be ignored, not just	*/
	    /* when an object is declared to be "trivial".  Not sure if	*/
	    /* this breaks certain netlists. . .			*/
	    if (locnet < 0) continue;

	    /* Get the name of the pin in the called object with no prefix. */
	    subnet = getsubnet(locnet, calls->callobj);
	    ppin = nettopin(locnet, calls->callobj, NULL);
	    hidx = *ptableptr;
	    while (hidx != NULL) {
	       if (hidx->nets != NULL) {
		  for (i = 0; i < hidx->nets->numnets; i++)
	             if (*(hidx->nets->netidx + i) == ports->netid)
		        break;
		  if (i < hidx->nets->numnets) break;
	       }
	       hidx = hidx->next;
	    }
	    if (hidx == NULL) {
	       snew = textprintsubnet(ppin, cinst, subnet);
	       Fprintf(stdout, "Warning:  Unconnected pin %s%s\n", newprefix, snew);
	       free(snew);
	    }
	    else {
	       outputcall = TRUE;
	       outputdone = TRUE;
	       tmpstr = (struct Pstr *)malloc(sizeof(struct Pstr));
	       tmpstr->string = (stringpart *)malloc(sizeof(stringpart));
	       tmpstr->string->type = TEXT_STRING;
	       tmpstr->string->nextpart = NULL;
	       snew = textprintsubnet(ppin, cinst, subnet);
	       tmpstr->string->data.string = (char *)malloc(strlen(newprefix)
			+ strlen(snew) + 2);
	       strcpy(tmpstr->string->data.string, newprefix);
	       /* Replace slash '/' with dash '-' at bottommost level */
	       if ((tmplen = strlen(newprefix)) > 0)
	          tmpstr->string->data.string[tmplen - 1] = '-';
	       strcat(tmpstr->string->data.string, snew);
	       free(snew);
	       tmpstr->next = hidx->pins;
	       hidx->pins = tmpstr;

	       /* diagnositic information */
	       {
		  struct Pnet *locnet = hidx->nets;
		  int ctr = 0;
		  while (locnet->next != NULL) {
		     locnet = locnet->next;
		     ctr++;
		  }
	          /* Fprintf(stdout, "Logged level-%d net %d (local net %d) pin %s\n",
			ctr, *(locnet->netidx), *(hidx->nets->netidx + i),
			tmpstr->string);			*/
	       }
            }
         }
      }
   }

   /* Step 5: Cleanup */

   free(newprefix);
   return outputdone;
}

/*----------------------------------------------------------------------*/
/* Save PCB table into pcb-style file				   	*/
/*----------------------------------------------------------------------*/

void outputpcb(struct Ptab *ptable, FILE *fp)
{
   int netidx = 1, ccol, subnet;
   struct Ptab *pseek;
   struct Pstr *sseek;
   char *snew;
   stringpart *ppin;

   if (fp == NULL) return;

   for (pseek = ptable; pseek != NULL; pseek = pseek->next) {
      if (pseek->pins != NULL) {
	 if ((pseek->nets != NULL) && (pseek->nets->numnets > 0)) {
	     subnet = getsubnet(*(pseek->nets->netidx), pseek->cschem);
	     ppin = nettopin(*(pseek->nets->netidx), pseek->cschem, "");
	     snew = textprintsubnet(ppin, NULL, subnet);
	     strcpy(_STR, snew);
	     free(snew);
	 }
	 else
             sprintf(_STR, "NET%d ", netidx++);
         fprintf(fp, "%-11s ", _STR);
         ccol = 12;
         for (sseek = pseek->pins; sseek != NULL; sseek = sseek->next) {
	    ccol += stringlength(sseek->string, False, NULL) + 3;
	    if (ccol > 78) {
	       fprintf(fp, "\\\n              ");
      	       ccol = 18 + stringlength(sseek->string, False, NULL);
	    }
	    snew = textprint(sseek->string, NULL);
	    fprintf(fp, "%s   ", snew);
	    free(snew);
         }
         fprintf(fp, "\n");
      }
      /* else fprintf(fp, "NET%d	*UNCONNECTED*\n", netidx++);	*/
   }
}

/*----------------------------------------------*/
/* free memory allocated to PCB net tables	*/
/*----------------------------------------------*/

void freepcb(struct Ptab *ptable)
{
   struct Ptab *pseek, *pseek2;
   struct Pstr *sseek, *sseek2;
   struct Pnet *nseek, *nseek2;

   pseek = ptable;
   pseek2 = pseek;

   while (pseek2 != NULL) {
      pseek = pseek->next;

      sseek = pseek2->pins;
      sseek2 = sseek;
      while (sseek2 != NULL) {
	 sseek = sseek->next;
	 freelabel(sseek2->string);
	 free(sseek2);
	 sseek2 = sseek;
      }

      nseek = pseek2->nets;
      nseek2 = nseek;
      while (nseek2 != NULL) {
	 nseek = nseek->next;
	 if (nseek2->numnets > 0) free(nseek2->netidx);
	 free(nseek2);
	 nseek2 = nseek;
      }

      free(pseek2);
      pseek2 = pseek;
   }
}

/*----------------------------------------------------------------------*/
/* Remove an element from the netlist.  This is necessary when an	*/
/* element is deleted from the object, so that the netlist does not	*/
/* refer to a nonexistant element, or try to perform netlist function	*/
/* on it.								*/
/*									*/
/* Return True or False depending on whether a pin was "orphaned" on	*/
/* the corresponding schematic or symbol (if any), as this may cause	*/
/* the class of the object (FUNDAMENTAL, TRIVIAL, etc.) to change.	*/
/*----------------------------------------------------------------------*/

Boolean RemoveFromNetlist(objectptr thisobject, genericptr thiselem)
{
   Boolean pinchanged = False;
   labelptr nlab;
   polyptr npoly;
   objectptr pschem;
   objinstptr ninst;

   PolylistPtr plist, plast;
   LabellistPtr llist, llast;
   CalllistPtr clist, clast;

   pschem = (thisobject->schemtype == SECONDARY) ? thisobject->symschem
	: thisobject;

   switch (thiselem->type) {
      case OBJINST:
	 ninst = (objinstptr)thiselem;
	 /* If this is an object instance, remove it from the calls */
	 clast = NULL;
	 for (clist = pschem->calls; clist != NULL; clist = clist->next) {
	    if (clist->callinst == ninst) {
	       if (clast == NULL)
		  pschem->calls = clist->next;
	       else
		  clast->next = clist->next;
	       freecalls(clist);
	       clist = (clast) ? clast : pschem->calls;
	       break;
	    }
	    else
	       clast = clist;
	 }
	 break;

      case POLYGON:
	 npoly = (polyptr)thiselem;
	 if (nonnetwork(npoly)) break;

	 /* If this is a polygon, remove it from the netlist */
	 plast = NULL;
	 for (plist = pschem->polygons; plist != NULL; plist = plist->next) {
	    if (plist->poly == npoly) {
	       if (plast == NULL)
		  pschem->polygons = plist->next;
	       else
		  plast->next = plist->next;
	       if (plist->subnets > 0)
		  free(plist->net.list);
	       break;
	    }
	    else
	       plast = plist;
	 }
	 break;

      case LABEL:
	 nlab = (labelptr)thiselem;
	 if ((nlab->pin != LOCAL) && (nlab->pin != GLOBAL)) break;

	 /* If this is a label, remove it from the netlist */
	 llast = NULL;
	 for (llist = pschem->labels; llist != NULL; llist = llist->next) {
	    if (llist->label == nlab) {
	       if (llast == NULL)
		  pschem->labels = llist->next;
	       else
		  llast->next = llist->next;
	       if (llist->subnets > 0)
		  free(llist->net.list);
	       break;
	    }
	    else
	       llast = llist;
	 }

	 /* Mark pin label in corresponding schematic/symbol as "orphaned" */
	 /* by changing designation from type "pin" to type "label".       */

	 if (findlabelcopy(nlab, nlab->string) == NULL) {
	    changeotherpins(NULL, nlab->string);
	    if (nlab->pin == INFO) pinchanged = True;
	 }
   }
   return pinchanged;
}

/*----------------------------------------------------------------------*/
/* Remove one element from the netlist					*/
/*----------------------------------------------------------------------*/

void remove_netlist_element(objectptr cschem, genericptr genelem) {

   objectptr pschem;
   CalllistPtr clist, clast, cnext;
   LabellistPtr llist, llast, lnext;
   PolylistPtr plist, plast, pnext;
   Boolean found = FALSE;

   /* Always call on the primary schematic */
   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   switch (ELEMENTTYPE(genelem)) {
      case POLYGON:
	 /* remove polygon from polygon list */
	 plast = NULL;
	 for (plist = pschem->polygons; plist != NULL; ) {
	    pnext = plist->next;
	    if ((genericptr)plist->poly == genelem) {
	       found = TRUE;
	       if (plist->subnets > 0)
	          free(plist->net.list);
	       free(plist);
	       if (plast != NULL)
	          plast->next = pnext;
	       else
		  pschem->polygons = pnext;
	       break;
	    }
	    else
	       plast = plist;
	    plist = pnext;
	 }
	 break;

      case LABEL:

	 /* remove label from label list */
	 llast = NULL;
	 for (llist = pschem->labels; llist != NULL; ) {
	    lnext = llist->next;
	    if ((genericptr)llist->label == genelem) {
	       found = TRUE;
	       if (llist->subnets > 0)
	          free(llist->net.list);
	       free(llist);
	       if (llast != NULL)
	          llast->next = lnext;
	       else
		  pschem->labels = lnext;
	       break;
	    }
	    else
	       llast = llist;
	    llist = lnext;
	 }

	 /* also check the globals */
	 llast = NULL;
	 for (llist = global_labels; llist != NULL; ) {
	    lnext = llist->next;
	    if ((genericptr)llist->label == genelem) {
	       found = TRUE;
	       if (llist->subnets > 0)
	          free(llist->net.list);
	       free(llist);
	       if (llast != NULL)
	          llast->next = lnext;
	       else
		  global_labels = lnext;
	       break;
	    }
	    else
	       llast = llist;
	    llist = lnext;
         }
	 break;

      case OBJINST:

	 /* remove instance from call list */
	 clast = NULL;
	 for (clist = pschem->calls; clist != NULL; ) {
	    cnext = clist->next;
	    if ((genericptr)clist->callinst == genelem) {
	       found = TRUE;
	       freecalls(clist);
	       if (clast != NULL)
	          clast->next = cnext;
	       else
		  pschem->calls = cnext;
	    }
	    else
	       clast = clist;
	    clist = cnext;
	 }
	 break;
   }
   if (found)
      pschem->valid = FALSE;
}

/*----------------------------------------------------------------------*/
/* Free memory allocated for the ports in a calls.			*/
/*----------------------------------------------------------------------*/

void freecalls(CalllistPtr calls)
{
   PortlistPtr ports, pptr;

   for (ports = calls->ports; ports != NULL;) {
      pptr = ports->next;
      free(ports);
      ports = pptr;
   }
   if (calls->devname != NULL) free(calls->devname);
   free(calls);
}

/*----------------------------------------------------------------------*/
/* Free memory for a Genericlist structure (may also be a Labellist or	*/
/* Polylist structure).							*/
/*----------------------------------------------------------------------*/

void freegenlist(Genericlist *nets)
{
   if (nets == NULL) return;
   if (nets->subnets > 0)
      free(nets->net.list);
   free(nets);
}

/*----------------------------------------------------------------------*/
/* Free memory allocated for the label list in a netlist.		*/
/*----------------------------------------------------------------------*/

void freelabellist(LabellistPtr *listtop)
{
   LabellistPtr labellist, llist;

   for (labellist = *listtop; labellist != NULL;) {
      llist = labellist->next;
      freegenlist((Genericlist *)labellist);
      labellist = llist;
   }
   *listtop = NULL;
}

/*----------------------------------------------------------------------*/
/* Free memory allocated for the polygon list in a netlist.		*/
/*----------------------------------------------------------------------*/

void freepolylist(PolylistPtr *listtop)
{
   PolylistPtr polylist, plist;

   for (polylist = *listtop; polylist != NULL;) {
      plist = polylist->next;
      freegenlist((Genericlist *)polylist);
      polylist = plist;
   }
   *listtop = NULL;
}

/*----------------------------------------------------------------------*/
/* Free memory allocated for netlist 					*/
/*----------------------------------------------------------------------*/

void freenetlist(objectptr cschem)
{
   PolylistPtr *plist;
   LabellistPtr *llist;

   plist = &cschem->polygons;
   freepolylist(plist);
   llist = &cschem->labels;
   freelabellist(llist);
}

/*----------------------------------------------------------------------*/
/* Clear the "traversed" flag in all objects of the hierarchy.		*/
/*----------------------------------------------------------------------*/

int cleartraversed_level(objectptr cschem, int level)
{
   genericptr *cgen;
   objinstptr cinst;
   objectptr callobj, pschem;

   /* Always call on the primary schematic */
   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   /* Recursively call cleartraversed() on all subobjects, a la gennetlist()  */
   /* Use the parts list of the object, not the calls, because calls */
   /* may have been removed.						*/

   if (level == HIERARCHY_LIMIT) return -1;

   for (cgen = pschem->plist; cgen < pschem->plist + pschem->parts; cgen++) {
      if (IS_OBJINST(*cgen)) {
 	 cinst = TOOBJINST(cgen);

	 if (cinst->thisobject->symschem != NULL)
	    callobj = cinst->thisobject->symschem;
	 else
	    callobj = cinst->thisobject;
	
	 /* Don't infinitely recurse if object is on its own schematic	  */
	 /* However, we need to take a stab at more subtle recursion, too */

	 if (callobj != pschem)
	    if (cleartraversed_level(callobj, level + 1) == -1)
	       return -1;
      }
   }
   pschem->traversed = False;

   return 0;
}

/*----------------------------------------------------------------------*/
/* This is the routine normally called, as it hides the "level"		*/
/* argument tagging the level of recursion.				*/
/*----------------------------------------------------------------------*/

int cleartraversed(objectptr cschem) {
   return cleartraversed_level(cschem, 0);
}

/*----------------------------------------------------------------------*/
/* If any part of the netlist is invalid, destroy the entire netlist 	*/
/*----------------------------------------------------------------------*/

int checkvalid(objectptr cschem)
{
   genericptr *cgen;
   objinstptr cinst;
   objectptr callobj, pschem;

   /* If the object has been declared a non-network object, ignore it */
   if (cschem->schemtype == NONETWORK) return 0;

   /* Always operate on the master schematic */
   pschem = (cschem->schemtype == SECONDARY) ? cschem->symschem : cschem;

   /* Stop immediately if the netlist is invalid */
   if (pschem->valid == False) return -1;

   /* Otherwise, recursively call checkvalid() on all subobjects.	*/
   /* Use the parts list of the object, not the calls, because calls */
   /* may have been removed.						*/

   for (cgen = pschem->plist; cgen < pschem->plist + pschem->parts; cgen++) {
      if (IS_OBJINST(*cgen)) {
 	 cinst = TOOBJINST(cgen);

	 if (cinst->thisobject->symschem != NULL)
	    callobj = cinst->thisobject->symschem;
	 else
	    callobj = cinst->thisobject;
	
	 /* Don't infinitely recurse if object is on its own schematic	  */

	 if (callobj == pschem) continue;

	 /* If there is a symbol, don't check its parts, but check if 	*/
	 /* its netlist has been checkvalid.				*/

	 if ((cinst->thisobject->symschem != NULL) &&
		(cinst->thisobject->labels == NULL) &&
		(cinst->thisobject->polygons == NULL) &&
		(cinst->thisobject->valid == False))
	    return -1;
	       
	 /* Recursive call on subschematic */
	 if (checkvalid(callobj) == -1)
	    return -1;
      }
   }
   return 0;	/* All subnetlists and own netlist are valid */
}

/*----------------------------------------------------------------------*/
/* Free memory allocated to temporary labels generated for the netlist	*/
/*----------------------------------------------------------------------*/

void freetemplabels(objectptr cschem)
{
   genericptr *cgen;
   objinstptr cinst;
   objectptr callobj;
	
   /* Recursively call freetemplabels() on all subobjects, a la gennetlist()  */
   /* Use the parts list of the object, not the calls, because calls */
   /* may have been removed.						*/

   for (cgen = cschem->plist; cgen < cschem->plist + cschem->parts; cgen++) {
      if (IS_OBJINST(*cgen)) {

 	 cinst = TOOBJINST(cgen);
	 if (cinst->thisobject->symschem != NULL)
	    callobj = cinst->thisobject->symschem;
	 else
	    callobj = cinst->thisobject;
	
	 /* Don't infinitely recurse if object is on its own schematic */
	 if (callobj != cschem) freetemplabels(callobj);

	 /* Also free the temp labels of any associated symbol */
	 if (cinst->thisobject->symschem != NULL) freetemplabels(cinst->thisobject);
      }

      /* Free any temporary labels which have been created */

      else if (IS_LABEL(*cgen)) {
	 labelptr clab = TOLABEL(cgen);
	 /* int tmpval = (int)(cgen - cschem->plist); (jdk) */
	 if (clab->string->type != FONT_NAME) {
	    genericptr *tgen;

	    clab = TOLABEL(cgen);
	    freelabel(clab->string);
	    free(clab);
	    for (tgen = cgen + 1; tgen < cschem->plist + cschem->parts; tgen++)
	       *(tgen - 1) = *tgen;
	    cschem->parts--;
	    cgen--;
	 }
      }
   }
}

/*----------------------------------------------------------------------*/
/* Free memory allocated for netlists, ports, and calls			*/
/*----------------------------------------------------------------------*/

void freenets(objectptr cschem)
{
   CalllistPtr calls, cptr;
   PortlistPtr ports, pptr;
   genericptr *cgen;
   objinstptr cinst;
   objectptr callobj;
	
   /* Recursively call freenets() on all subobjects, a la gennetlist()  */
   /* Use the parts list of the object, not the calls, because calls */
   /* may have been removed.						*/

   if (cschem->schemtype == PRIMARY || cschem->schemtype == SECONDARY ||
		(cschem->schemtype == SYMBOL && cschem->symschem == NULL)) {
      for (cgen = cschem->plist; cgen < cschem->plist + cschem->parts; cgen++) {
         if (IS_OBJINST(*cgen)) {

 	    cinst = TOOBJINST(cgen);
	    if (cinst->thisobject->symschem != NULL)
	       callobj = cinst->thisobject->symschem;
	    else
	       callobj = cinst->thisobject;
	
	    /* Don't infinitely recurse if object is on its own schematic */
	    if (callobj != cschem) freenets(callobj);

	    /* Also free the netlist of any associated symbol */
	    if (cinst->thisobject->symschem != NULL) freenets(cinst->thisobject);
	 }
      }
   }

   /* Free the allocated structures for this object */

   for (calls = cschem->calls; calls != NULL;) {
      cptr = calls->next;
      freecalls(calls);
      calls = cptr;
   }
   cschem->calls = NULL;

   for (ports = cschem->ports; ports != NULL;) {
      pptr = ports->next;
      free(ports);
      ports = pptr;
   }
   cschem->ports = NULL;

   freenetlist(cschem);

   cschem->traversed = False;
   cschem->valid = False;
   freegenlist(cschem->highlight.netlist);
   cschem->highlight.netlist = NULL;
   cschem->highlight.thisinst = NULL;
}

/*----------------------------------------------------------------------*/
/* Free the global pin list 					   	*/
/*----------------------------------------------------------------------*/

void freeglobals()
{
   LabellistPtr labellist, llist;

   for (labellist = global_labels; labellist != NULL;) {
      llist = labellist->next;

      /* Labels in the global list are temporary and must be deallocated */
      freelabel(labellist->label->string);
      free(labellist->label);

      freegenlist((Genericlist *)labellist);
      labellist = llist;
   }
   global_labels = NULL;
}

/*----------------------------------------------------------------------*/
/* Get rid of locally-defined pin names	 				*/ 
/*----------------------------------------------------------------------*/

void clearlocalpins(objectptr cschem)
{
   NetnamePtr netnames, nextname;
  
   for (netnames = cschem->netnames; netnames != NULL; ) {
      nextname = netnames->next;
      if (netnames->localpin != NULL) {
         freelabel(netnames->localpin);
      }
      free(netnames);
      netnames = nextname;
   }
   cschem->netnames = NULL;
}

/*----------------------------------------------------------------------*/
/* Handle lists of included files					*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* check_included() --- check if a file has already been included.	*/
/*	Check by inode instead of filename so that we cannot trick the	*/
/*	program by giving, e.g., one relative path and one full path.	*/
/*----------------------------------------------------------------------*/

Boolean check_included(char *filename)
{
   struct stat filestatus;
   int numi;

   if (stat(filename, &filestatus) == 0) {
      if (included_files == NULL) return FALSE;
      for (numi = 0; *(included_files + numi) != (ino_t)NULL; numi++) {
	 if (*(included_files + numi) == filestatus.st_ino) return TRUE;
      }
   }
   return FALSE;
}

/*----------------------------------------------------------------------*/
/* append_included() --- update the list of included files by adding	*/
/*	the inode of filename to the list.				*/
/*----------------------------------------------------------------------*/

void append_included(char *filename)
{
   struct stat filestatus;
   int numi;

   if (stat(filename, &filestatus) == 0) {

      if (included_files == NULL) {
         included_files = (ino_t *)malloc(2 * sizeof(ino_t));
         *included_files = filestatus.st_ino;
         *(included_files + 1) = (ino_t)NULL;
      }
      else {
	 for (numi = 0; *(included_files + numi) != (ino_t)NULL; numi++);
	 
	 included_files = (ino_t *)realloc(included_files,
		(++numi + 1) * sizeof(ino_t));
	 
	 *(included_files + numi - 1) = filestatus.st_ino;
	 *(included_files + numi) = (ino_t)NULL;
      }
   }
   else {
      Wprintf("Error: Cannot stat include file \"%s\"\n", filename);
   }
}

/*----------------------------------------------------------------------*/

void free_included()
{
   if (included_files != NULL) {
      free(included_files);
      included_files = NULL;
   }
}

/*-------------------------------------------------------------------------*/
