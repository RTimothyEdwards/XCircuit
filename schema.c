/*-------------------------------------------------------------------------*/
/* schema.c --- xcircuit routines specific to the schematic capture system */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*      written by Tim Edwards, 10/13/97    				   */
/*-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#ifdef TCL_WRAPPER 
#include <tk.h>
#else
#ifndef XC_WIN32
#include "Xw/Xw.h"
#include "Xw/MenuBtn.h"
#endif
#endif

/*-------------------------------------------------------------------------*/
/* Local includes							   */
/*-------------------------------------------------------------------------*/

#include "xcircuit.h"
#include "colordefs.h"
#include "menudep.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*------------------------------------------------------------------------*/
/* External Variable definitions                                          */
/*------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#endif

extern Globaldata xobjs;
extern XCWindowData *areawin;
extern xcWidget     menuwidgets[];
extern xcWidget	  netbutton;
extern char	  _STR[150];
extern char	  _STR2[250];
extern colorindex *colorlist;

/*-------------------------------------------------------------------------*/

extern xcWidget wsymb, wschema;

/*--------------------------------------------------------*/
/* Menu calls (procedure wrappers)			  */
/*--------------------------------------------------------*/

void callwritenet(xcWidget w, pointertype mode, caddr_t calldata)
{
   switch(mode) {
     case 0:
        writenet(topobject, "spice", "spc");
	break;
     case 1:
        writenet(topobject, "flatsim", "sim");
	break;
     case 2:
        writenet(topobject, "pcb", "pcbnet");
	break;
     case 3:
        writenet(topobject, "flatspice", "fspc");
	break;
     case 4:
	/* Auto-numbering:  no output generated */
	writenet(topobject, "indexpcb", "");
	break;
   }
}

/*----------------------------------------------------------------------*/
/* Find the page object and instance with the indicated name.		*/
/*----------------------------------------------------------------------*/

objectptr NameToPageObject(char *objname, objinstptr *ret_inst, int *ret_page)
{
   int i;

   for (i = 0; i < xobjs.pages; i++) {
      if (xobjs.pagelist[i]->pageinst == NULL) continue;
      if (!strcmp(objname, xobjs.pagelist[i]->pageinst->thisobject->name)) {
	 if (ret_inst) *ret_inst = xobjs.pagelist[i]->pageinst;
	 if (ret_page) *ret_page = i;
	 return xobjs.pagelist[i]->pageinst->thisobject;
      }
   }
   return NULL;
}

/*--------------------------------------------------------------*/
/* Get the canonical name of an object (the part without the	*/
/* technology prefix, if any).					*/
/*--------------------------------------------------------------*/

char *GetCanonicalName(char *fullname)
{
   char *canonname = strstr(fullname, "::");
   if (canonname == NULL) return fullname;
   return canonname + 2;
}

/*----------------------------------------------------------------------*/
/* Find the object with the indicated name.				*/
/*									*/
/* WARNING:  If no library technology is given, XCircuit will return	*/
/* the first object with the name "objname".  If there are multiple	*/
/* objects of the same name in different technologies, then the one	*/
/* returned may not be the one expected!				*/
/*----------------------------------------------------------------------*/

objectptr NameToObject(char *objname, objinstptr *ret_inst, Boolean dopages)
{
   int i;
   liblistptr spec;
   Boolean notech = FALSE;
   char *techptr;

   if (strstr(objname, "::") == NULL) notech = TRUE;

   for (i = 0; i < xobjs.numlibs; i++) {
      for (spec = xobjs.userlibs[i].instlist; spec != NULL; spec = spec->next) {
	 techptr = spec->thisinst->thisobject->name;
	 if (notech)
	    techptr = GetCanonicalName(spec->thisinst->thisobject->name);
         if (!strcmp(objname, techptr)) {
	    if (ret_inst) *ret_inst = spec->thisinst;
	    return spec->thisinst->thisobject;
	 }
      }
   }

   if (dopages)
      return NameToPageObject(objname, ret_inst, NULL);
   else
      return NULL;
}

/*------------------------------------------------------------------------*/
/* Ensure that a page name is unique (required for schematic association) */
/*------------------------------------------------------------------------*/

int checkpagename(objectptr thispageobj)
{
   int p, thispage;
   /* char *objname = thispageobj->name; (jdk) */
   Boolean changed;
   Boolean update = False;
   char *clnptr = NULL;
   int n;

   /* Check for ":n" suffix and prepare for possible update */
   clnptr = strrchr(thispageobj->name, ':');
   if (clnptr != NULL)
      if (sscanf(clnptr + 1, "%d", &n) != 1)
	 clnptr = NULL;

   /* Find the page number of this page object */
   for (p = 0; p < xobjs.pages; p++) {
      if (xobjs.pagelist[p]->pageinst != NULL) {
	 if (xobjs.pagelist[p]->pageinst->thisobject == thispageobj) {
	    thispage = p;
	    break;
	 }
      }
   }
   if (p == xobjs.pages) {
      Fprintf(stderr, "Error:  Object is not a page object!\n");
      return 0;
   }

   /* Look for any other pages with the same name */
   do {
      changed = False;
      for (p = 0; p < xobjs.pages; p++) {
	 if (p == thispage) continue;
         if (xobjs.pagelist[p]->pageinst != NULL) {
	    if (!filecmp(xobjs.pagelist[p]->pageinst->thisobject->name,
			thispageobj->name)) {
	       /* append ":2" to name or update suffix to ensure uniqueness */
	       if (clnptr == NULL)
		  sprintf(thispageobj->name, "%s:2", thispageobj->name);
	       else
		  sprintf(clnptr + 1, "%d", n + 1);
	       changed = True;
	       update = True;
	       break;
	    }
	 }
      }
   } while (changed);
   if (update) {
      renamepage(thispage);
      return -1;
   }
   return 0;
}

/*--------------------------------------*/
/* Find connectivity of a selection	*/
/*--------------------------------------*/

void startconnect(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   if (areawin->selects > 0)
      connectivity(button, clientdata, calldata);
}

/*----------------------------------------------------------------------*/
/* connectivity():  Find electrical connections into a node.		*/
/* (does recursive search on object heirarchy)				*/
/*----------------------------------------------------------------------*/

void connectivity(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   short *gsel = NULL;
   selection *rselect = NULL, *nextselect;
   genericptr ggen = NULL;
   Genericlist *netlist = NULL;
   int depth, lbus, netid, subnetid;
   buslist *sbus;
   pushlistptr seltop, nextptr;
   objectptr nettop, pschem; /*  thisobj, (jdk) */
   char *snew;
   stringpart *ppin;

   /* erase any existing highlighted network */
   highlightnetlist(topobject, areawin->topinstance, 0);

   seltop = (pushlistptr)malloc(sizeof(pushlist));
   seltop->thisinst = areawin->topinstance;
   seltop->next = NULL;

   /* pick the first selection that looks like a valid network part */

   if (areawin->selects > 0) {
      for (gsel = areawin->selectlist; gsel < areawin->selectlist +
		areawin->selects; gsel++) {
	 ggen = *(topobject->plist + *gsel);
         if (SELECTTYPE(gsel) == LABEL) {
	    labelptr glab = SELTOLABEL(gsel);
	    if (glab->pin == LOCAL || glab->pin == GLOBAL) break;
         }
	 else if (SELECTTYPE(gsel) == POLYGON) {
	    polyptr gpoly = SELTOPOLY(gsel);
	    if (!nonnetwork(gpoly)) break;
	 }
      }
   }
   if ((areawin->selects == 0) || (gsel == areawin->selectlist
		+ areawin->selects)) {
      rselect = recurselect(POLYGON | LABEL | OBJINST, MODE_CONNECT, &seltop);
      /* Should look at the top scorer, when scoring has been implemented */
      if (rselect && (rselect->selects > 0)) {
	 for (nextselect = rselect; (nextselect->next != NULL) &&
		(nextselect->selects > 0); nextselect = nextselect->next);
         ggen = *(nextselect->thisinst->thisobject->plist + *(nextselect->selectlist));
	 while (rselect != NULL) {
	    nextselect = rselect->next;
            free(rselect->selectlist);
	    free(rselect);
	    rselect = nextselect;
	 }
      }
   }

   /* Determine the net and the topmost object in which that	*/
   /* net appears.  Then build the transformation matrix down	*/
   /* to that object.  highlightnet() should end by popping the */
   /* entire matrix stack.					*/

   if (ggen != NULL) {
      if (checkvalid(topobject) == -1) {
         destroynets(topobject);
         createnets(areawin->topinstance, FALSE);
      }
      if ((netlist = is_resolved(&ggen, seltop, &nettop)) != NULL) {
         depth = pushnetwork(seltop, nettop);
         /* Fprintf(stdout, ">> Pushed network %d levels deep\n", depth); */
         nextptr = seltop;
         while (nextptr->thisinst->thisobject != nettop)
	    nextptr = nextptr->next;

	 nextptr->thisinst->thisobject->highlight.netlist = netlist;
	 nextptr->thisinst->thisobject->highlight.thisinst = nextptr->thisinst;
	 highlightnetlist(nettop, nextptr->thisinst, 1);
	    
         /* pop the matrix stack */
         while (depth-- > 0) 
	    UPopCTM();

	 /* get the primary schematic */
	 pschem = (nettop->schemtype == SECONDARY) ? nettop->symschem : nettop;

	 /* print the net name to the message window */

	 if (netlist->subnets == 0) {
	    ppin = nettopin(netlist->net.id, pschem, NULL);
            snew = textprint(ppin, areawin->topinstance);
	    sprintf(_STR2, "Network is \"%s\" in %s", snew, nettop->name);
            free(snew);
	 }
	 else {
	    char *sptr;
	    sprintf(_STR2, "Network(s): ");
	    sptr = _STR2 + strlen(_STR2);
	    for (lbus = 0; lbus < netlist->subnets; lbus++) {
	       sbus = netlist->net.list + lbus;
	       netid = sbus->netid;
	       subnetid = sbus->subnetid;
	       ppin = nettopin(netid, pschem, NULL);
               snew = textprintsubnet(ppin, areawin->topinstance, subnetid);
	       sprintf(sptr, "%s ", snew);
	       sptr += strlen(snew) + 1;
	       free(snew);
	    }
	    sprintf(sptr, "in %s", nettop->name);
	 }
	 Wprintf("%s", _STR2);

#ifdef TCL_WRAPPER
	 Tcl_SetObjResult(xcinterp, Tcl_NewStringObj(snew, strlen(snew)));
#endif
      }
      else
	 Wprintf("Selected element is not part of a valid network.");
   }
   else { 
      Wprintf("No networks found near the cursor position");
      netid = 0;
   }

   /* free up linked list */

   while (seltop != NULL) {
      nextptr = seltop->next;
      free(seltop);
      seltop = nextptr;
   }
}

/*--------------------------------------------------------------*/
/* Set object type to FUNDAMENTAL if it contains one or more	*/
/* info labels and is not associated with a schematic/symbol.	*/
/*								*/
/* Return TRUE if the object has electrically relevant		*/
/* networks, FALSE if not.					*/
/*--------------------------------------------------------------*/

Boolean setobjecttype(objectptr cschem)
{
   genericptr *cgen;
   labelptr clab;

   /* If networks are specifically prohibited. . . */
   if (cschem->schemtype == NONETWORK) return False;

   /* Apply only to schematic objects */

   if ((cschem->schemtype != PRIMARY) && (cschem->schemtype != SECONDARY)) {
      if (cschem->schemtype == FUNDAMENTAL)
	 cschem->schemtype = SYMBOL;
      if (cschem->symschem == NULL) {
         for (cgen = cschem->plist; cgen < cschem->plist + cschem->parts; cgen++) {
            if (IS_LABEL(*cgen)) {
               clab = TOLABEL(cgen);
	       if (clab->pin == INFO) {
	          cschem->schemtype = FUNDAMENTAL;
	          break;
	       }
	    }
         }
      }
   }

   if ((cschem->symschem != NULL) && (cschem->schemtype == SYMBOL))
      return False;
   else if ((cschem->schemtype == TRIVIAL) || (cschem->schemtype == FUNDAMENTAL))
      return False;

   return True;
}

/*------------------------------------------------------*/
/* Pin conversion subroutine for dopintype()		*/
/*------------------------------------------------------*/

void pinconvert(labelptr thislab, pointertype mode)
{
   thislab->pin = mode;
   switch (mode) {
      case NORMAL:
	 thislab->color = DEFAULTCOLOR;		/* nominally black */
	 break;
      case GLOBAL:
	 thislab->color = GLOBALPINCOLOR;	/* orange */
         break;
      case LOCAL:
	 thislab->color = LOCALPINCOLOR;	/* red */
         break;
      case INFO:
	 thislab->color = INFOLABELCOLOR;	/* green */
         break;
   }
}

/*---------------------------------------------------------*/
/* Change a label's type to NORMAL, GLOBAL, INFO, or LOCAL */
/*---------------------------------------------------------*/

void dopintype(xcWidget w, pointertype mode, caddr_t calldata)
{
   short *gsel;
   char typestr[40];
   short savetype = -1;

   if (areawin->selects == 0) {
      Wprintf("Must first select a label to change type");
      return;
   }

   strcpy(typestr, "Changed label to ");
   switch(mode) {
      case NORMAL:
	 strcat(typestr, "normal label");
         break;
      case GLOBAL:
	 strcat(typestr, "global pin");
         break;
      case LOCAL:
	 strcat(typestr, "local pin");
         break;
      case INFO:
	 strcat(typestr, "info-label");
         break;
   }

   for (gsel = areawin->selectlist; gsel < areawin->selectlist +
		areawin->selects; gsel++)
      if (SELECTTYPE(gsel) == LABEL) {
	 labelptr glab = SELTOLABEL(gsel);
	 savetype = glab->pin;
	 pinconvert(glab, mode);
	 setobjecttype(topobject);
      }

   if (savetype >= 0) {
      unselect_all();
      drawarea(NULL, NULL, NULL);
      Wprintf("%s", typestr);
   }
   else {
      Wprintf("No labels selected.");
   }
}

/*----------------------------------------------------------*/
/* Set colors on the symbol/schematic buttons appropriately */
/*----------------------------------------------------------*/

#ifdef TCL_WRAPPER

void setsymschem()
{
   XcInternalTagCall(xcinterp, 1, "schematic");
}

#else

void setsymschem()
{
   Arg aargs[2], bargs[2];

   /* Set menu items appropriately for this object */

   if (topobject->symschem != NULL) {
      if (topobject->schemtype == PRIMARY || topobject->schemtype == SECONDARY) {
         XtSetArg(aargs[0], XtNlabel, "Go To Symbol");
         XtSetArg(bargs[0], XtNlabel, "Disassociate Symbol");
      }
      else {
         XtSetArg(aargs[0], XtNlabel, "Go To Schematic");
         XtSetArg(bargs[0], XtNlabel, "Disassociate Schematic");
      }
   }
   else {
      if (topobject->schemtype == PRIMARY || topobject->schemtype == SECONDARY) {
         XtSetArg(aargs[0], XtNlabel, "Make Matching Symbol");
         XtSetArg(bargs[0], XtNlabel, "Associate with Symbol");
      }
      else {
         XtSetArg(aargs[0], XtNlabel, "Make Matching Schematic");
         XtSetArg(bargs[0], XtNlabel, "Associate with Schematic");
      }
   }
   XtSetValues(NetlistMakeMatchingSymbolButton, aargs, 1);
   XtSetValues(NetlistAssociatewithSymbolButton, bargs, 1);

   /* Set colors on the symbol and schematic buttons */

   if (topobject->schemtype == PRIMARY || topobject->schemtype == SECONDARY) {
      if (topobject->symschem == NULL) {
         XtSetArg(aargs[0], XtNbackground, colorlist[OFFBUTTONCOLOR].color.pixel);
	 XtSetArg(aargs[1], XtNforeground, colorlist[OFFBUTTONCOLOR].color.pixel);
      }
      else {
         XtSetArg(aargs[0], XtNbackground, colorlist[BACKGROUND].color.pixel);
	 XtSetArg(aargs[1], XtNforeground, colorlist[FOREGROUND].color.pixel);
      }

      XtSetArg(bargs[1], XtNforeground, colorlist[FOREGROUND].color.pixel);
      XtSetArg(bargs[0], XtNbackground, colorlist[SNAPCOLOR].color.pixel);
   }
   else {
      if (topobject->symschem != NULL) {
         XtSetArg(bargs[0], XtNbackground, colorlist[BACKGROUND].color.pixel);
	 XtSetArg(bargs[1], XtNforeground, colorlist[FOREGROUND].color.pixel);
      }
      else {
         XtSetArg(bargs[0], XtNbackground, colorlist[OFFBUTTONCOLOR].color.pixel);
	 XtSetArg(bargs[1], XtNforeground, colorlist[OFFBUTTONCOLOR].color.pixel);
      }

      XtSetArg(aargs[1], XtNforeground, colorlist[FOREGROUND].color.pixel);
      if (topobject->schemtype == FUNDAMENTAL)
         XtSetArg(aargs[0], XtNbackground, colorlist[AUXCOLOR].color.pixel);
      else if (topobject->schemtype == TRIVIAL || topobject->symschem != NULL)
         XtSetArg(aargs[0], XtNbackground, colorlist[SNAPCOLOR].color.pixel);
      else {
         XtSetArg(aargs[0], XtNbackground, colorlist[OFFBUTTONCOLOR].color.pixel);
	 XtSetArg(aargs[1], XtNforeground, colorlist[OFFBUTTONCOLOR].color.pixel);
         XtSetArg(bargs[0], XtNbackground, colorlist[BBOXCOLOR].color.pixel);
         XtSetArg(bargs[1], XtNforeground, colorlist[FOREGROUND].color.pixel);
      }
   }

   XtSetValues(wsymb, aargs, 2);
   XtSetValues(wschema, bargs, 2);
}

#endif

/*--------------------------------------------------------*/
/* Find the page number for an object			  */
/*--------------------------------------------------------*/

int findpageobj(objectptr pobj)
{
   int tpage;

   for (tpage = 0; tpage < xobjs.pages; tpage++)
      if (xobjs.pagelist[tpage]->pageinst != NULL)
         if (xobjs.pagelist[tpage]->pageinst->thisobject == pobj)
	    return tpage;

   return -1;
}

/*------------------------------------------------------*/
/* Enumerate all of the pages which are subschematics   */
/* of "toppage" and return the result in the list	*/
/* passed as a pointer.  The list is assumed to be	*/
/* already allocated, and equal to the total number of	*/
/* pages (xobjs.pages).					*/
/*							*/
/* Avoid possible recursion problems by limiting the	*/
/* number of recursion levels.  Presumably no circuit	*/
/* would have more than several hundred hierarchical 	*/
/* levels.						*/
/*							*/
/* If "dolinks" is TRUE, ennumerate and follow pages	*/
/* that are descendents of the top level when the	*/
/* calling symbol instance has a "link" parameter and	*/
/* that parameter points to the same filename as the	*/
/* page.  This indicates a multiple-file project; the	*/
/* pages keep separate records of changes, and are	*/
/* saved independently, and loaded through the link	*/
/* dependency method.					*/
/*------------------------------------------------------*/

int findsubschems(int toppage, objectptr cschem, int level, short *pagelist,
	Boolean dolinks)
{
   genericptr *cgen;

   if (level == HIERARCHY_LIMIT) return -1;	/* sanity check */

   for (cgen = cschem->plist; cgen < cschem->plist + cschem->parts; cgen++) {
      if (IS_OBJINST(*cgen)) {
	 objinstptr cinst = TOOBJINST(cgen);
	 objectptr cobj = cinst->thisobject;

	 if (cobj->symschem != NULL) {
	    int pageno = findpageobj(cobj->symschem);

	    if ((pageno >= 0) && (pageno < xobjs.pages)) {

	       /* Look for a "link" parameter */
	       if (dolinks == FALSE) {
		  oparamptr ops;
	          ops = find_param(cinst, "link");
	          if ((ops != NULL) && (ops->type == XC_STRING)) {
	             char *filename = textprint(ops->parameter.string, cinst);
		     if (!strcmp(filename, "%n") || !strcmp(filename, "%N") ||
			  !strcmp(filename, xobjs.pagelist[pageno]->filename)) {
		        free(filename);
		        continue;
		     }
	             free(filename);
		  }
	       }

	       /* Add this page to the list */
	       pagelist[pageno]++;
	    }

	    /* A symbol on its own schematic page is allowed for clarity */
	    /* of the schematic, but this cannot be a functional part of */
	    /* the schematic circuit!					 */
	    
	    if (cobj->symschem != cschem) {
	       if (findsubschems(toppage, cobj->symschem,
				level + 1, pagelist, dolinks) == -1)
		  return -1;
	    }
	 }
	 else if (cobj->schemtype != FUNDAMENTAL && cobj->schemtype != TRIVIAL) {
	    /* Check symbols acting as their own schematics */
	    if (findsubschems(toppage, cobj, level + 1, pagelist, dolinks) == -1)
	       return -1;
	 }
      }
   }
   return 0;
}

/*------------------------------------------------------*/
/* Recursively find all sub-circuits associated with	*/
/* the top-level circuit.  For each schematic that does	*/
/* not have a valid filename, set the filename equal to	*/
/* the filename of the "toppage" schematic.		*/
/*------------------------------------------------------*/

void collectsubschems(int toppage)
{
   int loctop;
   objectptr cschem;
   short *pagelist, pageno;
   Pagedata *curpage;

   loctop = toppage;
   curpage = xobjs.pagelist[loctop];
   if (curpage->pageinst == NULL) return;
   cschem = curpage->pageinst->thisobject;
   if (cschem->schemtype == SECONDARY) {
      cschem = cschem->symschem;
      loctop = is_page(cschem);
      if (loctop < 0) return;
      curpage = xobjs.pagelist[loctop];
   }

   pagelist = (short *)malloc(xobjs.pages * sizeof(short));

   for (pageno = 0; pageno < xobjs.pages; pageno++)
      pagelist[pageno] = 0;

   findsubschems(loctop, cschem, 0, pagelist, FALSE);

   for (pageno = 0; pageno < xobjs.pages; pageno++) {
      if (pageno == loctop) continue;
      if (pagelist[pageno] > 0) {
	 if (xobjs.pagelist[pageno]->filename != NULL)
	    free(xobjs.pagelist[pageno]->filename);
	 xobjs.pagelist[pageno]->filename =
	    strdup(xobjs.pagelist[loctop]->filename);
      }
   } 
   free((char *)pagelist);
}

/*------------------------------------------------------*/
/* compare_qualified(str1, str2) ---			*/
/*							*/
/* 	String comparison for technology-qualified	*/
/*	names.  str2" must be a fully-qualified name.	*/
/*	"str1" may be qualified or unqualified.  If	*/
/*	unqualified, compare_qualified will return TRUE	*/
/*	for any "str2" that matches the name part of	*/
/*	the technology::name format.			*/
/*							*/
/*	Return value:  TRUE if match, FALSE if not.	*/
/*------------------------------------------------------*/

Boolean compare_qualified(char *str1, char *str2)
{
   char *sptr1, *sptr2;
   Boolean qual1, qual2;

   sptr2 = strstr(str2, "::");
   qual2 = (sptr2 == NULL) ? FALSE : TRUE;

   if (!qual2) return (!strcmp(str1, str2));

   sptr1 = strstr(str1, "::");
   qual1 = (sptr1 == NULL) ? FALSE : TRUE;

   if (qual1) return (!strcmp(str1, str2));

   sptr2 += 2;
   return (!strcmp(str1, sptr2));
}

/*-------------------------------------------------------*/
/* Check if top-level page is the same name as a library */
/* object; if so, connect it.				 */
/* Note that it is not an error not to find the matching */
/* symbol/schematic.  "is_schematic" and "is_symbol"	 */
/* comments in the .ps file are complementary, so the	 */
/* first one encountered will always fail, and the other */
/* will succeed.					 */
/*-------------------------------------------------------*/

int checkschem(objectptr thisobj, char *cname)
{
   objectptr *tlib;
   short i, j;

   if (thisobj->symschem != NULL) return 0;

   for (i = 0; i < xobjs.numlibs; i++) {
      for (j = 0; j < xobjs.userlibs[i].number; j++) {
	 tlib = xobjs.userlibs[i].library + j;

         if (compare_qualified(cname, (*tlib)->name)) {
	    thisobj->symschem = (*tlib);
	    thisobj->schemtype = PRIMARY;
	    (*tlib)->symschem = thisobj;
	    (*tlib)->schemtype = SYMBOL;
	    return 1;
	 }
      }
   }
   return 0;
}

/*--------------------------------------------------------*/
/* Complement to the above routine:  If a library object  */
/* name is the same as a top-level page, connect them.	  */
/*--------------------------------------------------------*/

int checksym(objectptr symobj, char *cname)
{
   short cpage;
   objectptr checkpage;

   if (symobj->symschem != NULL) return 0;

   for (cpage = 0; cpage < xobjs.pages; cpage++) {
      if (xobjs.pagelist[cpage]->pageinst != NULL) {
         checkpage = xobjs.pagelist[cpage]->pageinst->thisobject;
         if (compare_qualified(cname, checkpage->name)) {
	    symobj->symschem = checkpage;
	    symobj->schemtype = SYMBOL;
	    checkpage->symschem = symobj;
	    checkpage->schemtype = PRIMARY;
	    return 1;
	 }
      }
   }
   return 0;
}

/*----------------------------------------------------------------------*/
/* Find location of corresponding pin in symbol/schematic and change it */
/* to the text of the indicated label.					*/
/*									*/
/* Return the number of other pins changed.  zero indicates that no	*/
/* corresponding pins were found, and therefore nothing was changed.	*/
/*----------------------------------------------------------------------*/

int changeotherpins(labelptr newlabel, stringpart *oldstring)
{
   objectptr other = topobject->symschem;
   genericptr *tgen;
   labelptr tlab;
   int rval = 0;

   if (other == NULL) return rval;

   for (tgen = other->plist; tgen < other->plist + other->parts; tgen++) {
      if (IS_LABEL(*tgen)) {
	 tlab = TOLABEL(tgen);
	 if (tlab->pin != LOCAL) continue;
	 if (!stringcomp(tlab->string, oldstring)) {
	    if (newlabel != NULL) {
	       free(tlab->string);
	       tlab->string = stringcopy(newlabel->string);
	       rval++;
	    }
	 }
      }
   }
   return rval;
}

/*----------------------------------------------------------------------*/
/* Xt wrapper for swapschem()						*/
/*----------------------------------------------------------------------*/

void xlib_swapschem(xcWidget w, pointertype mode, caddr_t calldata)
{
   swapschem((int)mode, -1, NULL);
}

/*----------------------------------------------------------------------*/
/* Swap object schematic and symbol pages.   				*/
/*  allow_create = 0 disallows creation of a new schematic or symbol;	*/
/*  i.e., if there is no corresponding schematic/symbol, nothing	*/
/*  happens.								*/
/*----------------------------------------------------------------------*/

void swapschem(int allow_create, int libnum, char *fullname)
{
   objectptr savepage = topobject;
   labelptr  *pinlab;
   genericptr *plab;
   Boolean lflag;
   pushlistptr stacktop;
   short loclibnum = libnum;
   char *canonname;

   if (libnum == -1) loclibnum = USERLIB - LIBRARY;

   /* Create symbol or schematic, if allowed by allow_create */

   if ((topobject->symschem == NULL) && (allow_create != 0)
		&& (topobject->schemtype != SECONDARY)) {

      if (topobject->schemtype != PRIMARY) {
	 int tpage;

	 /* create a new page for the new schematic */

	 for (tpage = 0; tpage < xobjs.pages; tpage++)
	    if (xobjs.pagelist[tpage]->pageinst == NULL) break;

	 /* Push the current instance onto the push stack */
	 /* Change the page without destroying the pushlist */

	 push_stack(&areawin->stack, areawin->topinstance, NULL);
	 stacktop = areawin->stack;
	 areawin->stack = NULL;
         changepage(tpage);
	 areawin->stack = stacktop;
      }
      else {
	 objectptr *newobject;

	 /* create a new library object for the new symbol */

	 xobjs.userlibs[loclibnum].library = (objectptr *)
		realloc(xobjs.userlibs[loclibnum].library,
		++xobjs.userlibs[loclibnum].number * sizeof(objectptr));
	 newobject = xobjs.userlibs[loclibnum].library
		+ xobjs.userlibs[loclibnum].number - 1;
         *newobject = (objectptr) malloc(sizeof(object));
	 initmem(*newobject);
	 (*newobject)->schemtype = SYMBOL;
	 (*newobject)->hidden = False;

	 incr_changes(*newobject);

	 if (eventmode == MOVE_MODE || eventmode == COPY_MODE)
	    standard_element_delete(ERASE);
	 else
	    unselect_all();

	 /* Generate a library instance for this object and set the */
	 /* top instance to point to it.			    */

	 topobject->viewscale = areawin->vscale;
	 topobject->pcorner = areawin->pcorner;
	 push_stack(&areawin->stack, areawin->topinstance, NULL);
	 areawin->topinstance = addtoinstlist(loclibnum, *newobject, FALSE);

	 /* Generate the default bounding box for a size-zero object */
	 calcbbox(areawin->topinstance);
      }

      /* set links between the two objects */

      savepage->symschem = topobject;
      topobject->symschem = savepage;

      /* Make the name of the new object equal to that of the old, */
      /* except that symbols get the full name while schematics	   */
      /* get the canonical name (without the technology prefix)	   */

      if (fullname == NULL)
	 canonname = GetCanonicalName(savepage->name);
      else {
         canonname = strstr(fullname, "::");
	 if ((canonname == NULL) || (topobject->schemtype != PRIMARY))
	    canonname = fullname;
	 else
	    canonname += 2;
      }
      strcpy(topobject->name, canonname);
      checkname(topobject);

      /* copy all pin labels into the new object */

      for (plab = savepage->plist; plab < savepage->plist + savepage->parts;
		plab++) {
	 if (IS_LABEL(*plab)) {
	    genericptr *tgen;
	    labelptr tlab, lpin = (labelptr)*plab;

	    if (lpin->pin == LOCAL) {

      	       /* Only make one copy of each pin name */

	       lflag = False;
               for (tgen = topobject->plist; tgen <
			topobject->plist + topobject->parts; tgen++) {
		  if (IS_LABEL(*tgen)) {
		     tlab = TOLABEL(tgen);
	             if (!stringcomp(tlab->string, lpin->string)) lflag = True;
		  }
      	       }
	       if (lflag == True) continue;

	       NEW_LABEL(pinlab, topobject);
	       (*pinlab)->pin = lpin->pin;
	       (*pinlab)->color = lpin->color;
	       (*pinlab)->rotation = 0.0;
	       (*pinlab)->scale = 1.0;
	       (*pinlab)->anchor = areawin->anchor; 
	       (*pinlab)->position.x = 0;
	       (*pinlab)->position.y = topobject->parts * (TEXTHEIGHT + 10);
	       (*pinlab)->passed = NULL;
	       (*pinlab)->cycle = NULL;
	       u2u_snap(&((*pinlab)->position));
	       (*pinlab)->string = stringcopy(lpin->string);
	       incr_changes(topobject);
	    }
         }
      }
      calcbbox(areawin->topinstance);

      /* Recreate the user library with the new symbol */
      if (savepage->schemtype != SYMBOL) composelib(loclibnum + LIBRARY);
   }
   else if (topobject->symschem != NULL) {

      /* If symschem matches the last entry on the push stack, then we	*/
      /* pop; otherwise, we push.					*/

      if (areawin->stack && areawin->stack->thisinst->thisobject
			== topobject->symschem) {
	 topobject->viewscale = areawin->vscale;
	 topobject->pcorner = areawin->pcorner;
	 areawin->topinstance = areawin->stack->thisinst;
	 pop_stack(&areawin->stack);
      }
      else {
	 int p;
	 objinstptr syminst = NULL;
	 liblistptr symlist;

	 /* If symschem is a schematic, find the appropriate page */

	 for (p = 0; p < xobjs.pages; p++) {
	    syminst = xobjs.pagelist[p]->pageinst;
	    if (syminst != NULL)
	       if (syminst->thisobject == topobject->symschem)
		  break;
	 }
	 if (p == xobjs.pages) {

	    /* If symschem is a symbol, and it wasn't on the push stack, */
	    /* get the library default symbol and go there.		 */

	    for (p = 0; p < xobjs.numlibs; p++) {
	       for (symlist = xobjs.userlibs[p].instlist; symlist != NULL;
			symlist = symlist->next) {
	          syminst = symlist->thisinst;
	          if (syminst->thisobject == topobject->symschem &&
			symlist->virtual == FALSE)
		     break;
	       }
	       if (symlist != NULL) break;
	    }
	    if (p == xobjs.numlibs) {
	       Fprintf(stderr, "swapschem(): BAD SYMSCHEM\n");
	       return;
	    }
         }
	       
	 if (eventmode == MOVE_MODE || eventmode == COPY_MODE)
	    delete_for_xfer(NORMAL, areawin->selectlist, areawin->selects);

	 topobject->viewscale = areawin->vscale;
	 topobject->pcorner = areawin->pcorner;
	 push_stack(&areawin->stack, areawin->topinstance, NULL);
	 areawin->topinstance = syminst;
      }
   }

   /* If there was no action, then there is nothing more to do. */

   if (topobject == savepage) return;

   setpage(TRUE);
   transferselects();
   refresh(NULL, NULL, NULL);
   setsymschem();
}

#ifndef TCL_WRAPPER

/*----------------------------------------------------------------------*/
/* Wrapper for swapschem() when generating a new symbol.		*/
/*----------------------------------------------------------------------*/

void makesymbol(xcWidget w, caddr_t calldata)
{
   /* copy name from popup prompt buffer and check */

   swapschem(1, -1, _STR2);
}

/*----------------------------------------------------------------------*/
/* Check name before doing a swap:  If name begins with "Page", prompt	*/
/* for the object name, as you would when doing selectsave().		*/ 
/*----------------------------------------------------------------------*/

void dobeforeswap(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   buttonsave *popdata = (buttonsave *)malloc(sizeof(buttonsave));

   /* Check for requirement to change the name before creating the symbol */

   if ((topobject->symschem == NULL) && (topobject->schemtype == PRIMARY)
		&& (strstr(topobject->name, "Page ") != NULL)) {

      /* Get a name for the new object */

      eventmode = NORMAL_MODE;
      popdata->dataptr = NULL;
      popdata->button = NULL; /* indicates that no button is assc'd w/ the popup */
      popupprompt(w, "Enter name for new object:", "\0", makesymbol, popdata, NULL);
   }
   else
      swapschem(1, -1, NULL);
}

#endif /* !TCL_WRAPPER */

/*------------------------------------------*/
/* Disassociate a symbol from its schematic */
/*------------------------------------------*/

void schemdisassoc()
{
   if (eventmode != NORMAL) {
      Wprintf("Cannot disassociate schematics in this mode");
   }
   else {
      topobject->symschem->symschem = NULL;
      topobject->symschem = NULL;
      setsymschem();
      Wprintf("Schematic and symbol are now unlinked.");
   }
}

/*--------------------------------------------------------------*/
/* Schematic<-->symbol association.  Determine whether action	*/
/* acts on a symbol or a schematic from context.		*/
/* mode == 0 associate only.					*/
/* mode == 1 determine action (associate or disassociate) from 	*/
/* context (toggle)						*/
/*--------------------------------------------------------------*/

void startschemassoc(xcWidget w, pointertype mode, caddr_t calldata)
{
   if ((topobject->symschem != NULL) && (mode == 1))
      schemdisassoc();
   else if ((topobject->symschem != NULL) && (mode == 0)) {
      Wprintf("Refusing to undo current association.");
   }
   else if (topobject->schemtype == SECONDARY) {
      Wprintf("Cannot attach symbol to a secondary schematic page.");
   }
   else {
      eventmode = ASSOC_MODE;
      if (topobject->schemtype == PRIMARY) {
	 /* Find a symbol to associate */
	 startcatalog(w, LIBLIB, NULL);
	 Wprintf("Select library page, then symbol to associate.");
      }
      else {
	 /* Find a schematic (page) to associate */
	 startcatalog(w, PAGELIB, NULL);
	 Wprintf("Select schematic page to associate.");
      }
   }
}

/*--------------------------------------------------------------*/
/* Callback procedures on the schematic/symbol buttons.		*/
/*--------------------------------------------------------------*/

Boolean schemassoc(objectptr schemobj, objectptr symbolobj)
{
   if (schemobj->symschem != NULL || symbolobj->symschem != NULL) {
      Wprintf("Both objects must be disassociated first.");
#if TCL_WRAPPER
      Tcl_SetResult(xcinterp, "Both objects must be disassociated first.", NULL);
#endif
      return False;
   }
   else {
      schemobj->symschem = symbolobj;
      symbolobj->symschem = schemobj;
      if (symbolobj->schemtype == TRIVIAL)
	 symbolobj->schemtype = SYMBOL;

      /* Schematic takes the name of its associated symbol, by default */
      /* Don't copy any technology prefix. */
      strcpy(schemobj->name, GetCanonicalName(symbolobj->name));

      /* Ensure that schematic (page) name is unique */
      while (checkpagename(schemobj) < 0);
      setsymschem();	/* Set buttons and menu items appropriately */
   }
   return True;
}

/*-------------------------------------------------------------------------*/
