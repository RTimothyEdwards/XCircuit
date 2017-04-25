/*----------------------------------------------------------------------*/
/* undo.c 								*/
/*									*/
/* The comprehensive "undo" and "redo" command handler			*/
/*									*/
/* Copyright (c) 2004  Tim Edwards, Open Circuit Design, Inc., and	*/
/* MultiGiG, Inc.							*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*      written by Tim Edwards, 1/29/04    				*/
/*----------------------------------------------------------------------*/

#define MODE_UNDO (u_char)0
#define MODE_REDO (u_char)1

#define MAX_UNDO_EVENTS 100

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER
#include <tk.h>
#endif

#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Local structure definitions for holding undo information		*/
/*----------------------------------------------------------------------*/

typedef struct {
   float  angle1;
   float  angle2;
   short  radius;
   short  yaxis;
   XPoint position;
} arcinfo;

/* Smaller data structure used when saving paths in the undo stacks.    */
/* We don't need the rendering data from points[].                      */

typedef struct {
   u_short	type;
   int		color;
   eparamptr	passed;
   u_short	style;
   float	width;
   XPoint	ctrl[4];
} splineinfo;

typedef struct {
   int number;
   pointlist points;
} pathinfo;

typedef struct editelem {
   genericptr element;		/* element being edited */
   union {
      stringpart *string;	/* original contents of string, for label */
      pointlist points;		/* original polygon */
      arcinfo *arcspecs;	/* original arc values */
      pathinfo *pathspecs;	/* original parts of a path */
      XPoint	instpos;	/* original position, for an object instance */ 
   } save;
} editelement;

typedef struct {
   genericptr element;		/* element modified */
   float scale;			/* old scale value */
} scaleinfo;

typedef struct {
   XPoint rotpos;		/* original position */
   float rotation;		/* old rotation value */
} rotateinfo;

u_char undo_collect = (u_char)0;

/*----------------------------------------------------------------------*/
/* Externally declared variables					*/
/*----------------------------------------------------------------------*/

extern Globaldata xobjs;
extern XCWindowData *areawin;

/*----------------------------------------------------------------------*/
/* Attempt to set the window.  If the window exists, set it as current	*/
/* and return TRUE.  Otherwise, return FALSE.  This prevents the undo	*/
/* mechanism from crashing if we close a window and then try to undo	*/
/* events that were created relative to it.				*/
/*----------------------------------------------------------------------*/

Boolean setwindow(XCWindowData *trywindow)
{
   XCWindowData *chkwin;

   for (chkwin = xobjs.windowlist; chkwin != NULL; chkwin = chkwin->next) {
      if (chkwin == trywindow) {
	 areawin = trywindow;
	 return TRUE;
      }
   }
   return FALSE;
}

/*----------------------------------------------------------------------*/
/* remember_selection ---						*/
/*									*/
/*   Copy a selection list into a "uselection" record.  The uselection	*/
/*   record maintains the order of the elements as well as pointers to	*/
/*   each element, so the original element ordering can be recovered.	*/
/*----------------------------------------------------------------------*/

uselection *remember_selection(objinstptr topinst, short *slist, int number)
{
   int i, idx;
   uselection *newlist;

   newlist = (uselection *)malloc(sizeof(uselection));
   if (number > 0) {
      newlist->element = (genericptr *)malloc(number * sizeof(genericptr));
      newlist->idx = (short *)malloc(number * sizeof(short));
   }
   else {
      newlist->element = NULL;
      newlist->idx = NULL;
   }
   newlist->number = number;
   for (i = 0; i < number; i++) {
      idx = *(slist + i);
      *(newlist->element + i) = *(topinst->thisobject->plist + idx);
      *(newlist->idx + i) = idx;
   }
   return newlist; 
}

/*----------------------------------------------------------------------*/
/* Create a selection list in areawin from the saved uselection		*/
/* record.								*/
/*----------------------------------------------------------------------*/

short *regen_selection(objinstptr thisinst, uselection *srec)
{
   int i, j, k;
   genericptr egen;
   objectptr thisobj = thisinst->thisobject;
   Boolean reorder = False;
   short *slist;

   if (srec->number > 0)
      slist = (short *)malloc(srec->number * sizeof(short));

   k = 0;
   for (i = 0; i < srec->number; i++) {

      /* Use the element address, not the selection order. */
      egen = *(srec->element + i);
      if (egen == *(thisobj->plist + *(srec->idx + i)))
	 j = *(srec->idx + i);
      else {
	 reorder = True;
         for (j = 0; j < thisobj->parts; j++) {
	    if (egen == *(thisobj->plist + j))
	       break;
	 }
      }
      if (j < thisobj->parts) {
	 *(slist + k) = j;
	 k++;
      }
      else
	 Fprintf(stderr, "Error: element %p in select list but not object\n",
		egen);
   } 

   /* If the selection order is different from the order in the object,	*/
   /* then rearrange the object's list to match the selection order.	*/

   if (reorder) {
      /* (to be done) */
   }

   if (k == 0) {
      if (srec->number > 0) free(slist);
      return NULL;
   }
   else
      return slist;
}

/*----------------------------------------------------------------------*/
/* Use the selection list in the undo record to reorder parts in the	*/
/* object's part list.  Invert the selection list and return it.	*/
/*----------------------------------------------------------------------*/

void reorder_selection(Undoptr thisrecord)
{
   short *slist, *newlist, snum, i;
   genericptr *pgen, *plist, egen;
   objinstptr thisinst = thisrecord->thisinst;
   objectptr thisobj = thisinst->thisobject;

   snum = (short)thisrecord->idata;
   slist = (short *)thisrecord->undodata;
   plist = (genericptr *)malloc(snum * sizeof(genericptr));
   newlist = (short *)malloc(snum * sizeof(short));

   i = 0;
   for (pgen = plist; pgen < plist + snum; pgen++) {
      egen = *(thisobj->plist + i);
      *(plist + *(slist + i)) = egen;
      i++;
   }
   i = 0;
   for (pgen = plist; pgen < plist + snum; pgen++) {
      *(thisobj->plist + i) = *pgen;
      *(newlist + *(slist + i)) = i;
      i++;
   }
   free(plist);
   free(thisrecord->undodata);
   thisrecord->undodata = (char *)newlist;
}

/*----------------------------------------------------------------------*/
/* free_editelement ---							*/
/*									*/
/*   Free memory allocated to an undo record edit element structure.	*/
/*----------------------------------------------------------------------*/

void free_editelement(Undoptr thisrecord)
{
   editelement *erec;
   pathinfo *ppi;

   erec = (editelement *)thisrecord->undodata;
   switch (erec->element->type) {
      case LABEL:
	 freelabel(erec->save.string);
         break;
      case POLYGON: case SPLINE:
	 free(erec->save.points);
	 break;
      case ARC:
	 free(erec->save.arcspecs);
	 break;
      case PATH:
	 for (ppi = erec->save.pathspecs; ppi < erec->save.pathspecs +
		thisrecord->idata; ppi++)
	    free(ppi->points);
	 free(erec->save.pathspecs);
	 break;
   }
   free(erec);
}

/*----------------------------------------------------------------------*/
/* free_selection ---							*/
/*									*/
/*   Free memory allocated to an undo record selection list.		*/
/*----------------------------------------------------------------------*/

void free_selection(uselection *selrec)
{
   if (selrec->number > 0) {
      free(selrec->element);
      free(selrec->idx);
   }
   free(selrec);
}

/*----------------------------------------------------------------------*/
/* get_original_string ---						*/
/*									*/
/* Find the original version of the given label.			*/
/*----------------------------------------------------------------------*/

stringpart *get_original_string(labelptr thislab)
{
   Undoptr chkrecord, thisrecord;
   editelement *erec;
   labelptr elab;

   thisrecord = xobjs.undostack;
   if (thisrecord == NULL) return NULL;

   for (chkrecord = thisrecord; chkrecord != NULL; chkrecord = chkrecord->next) {
      switch (chkrecord->type) {
	 case XCF_Edit:
	    erec = (editelement *)(chkrecord->undodata);
	    elab = (labelptr)(erec->element);
	    if (elab != thislab) return NULL;
	    return erec->save.string;
	    
	 default:
	    return NULL;
      }
   }
   return NULL;  /* yes, this can be reached, if thisrecord->next == NULL */
}

/*----------------------------------------------------------------------*/
/* select_previous ---							*/
/*									*/
/* Set the selection to what was previously selected in the undo list.	*/
/* Return 0 on success, -1 if no previous selection was found.		*/
/*----------------------------------------------------------------------*/

int select_previous(Undoptr thisrecord)
{
   Undoptr chkrecord;
   uselection *srec;

   clearselects_noundo();
   for (chkrecord = thisrecord->next; chkrecord != NULL; chkrecord = chkrecord->next) {

      /* Selections may cross page changes, but only if in the same series */
      if ((chkrecord->thisinst != thisrecord->thisinst) &&
		(chkrecord->idx != thisrecord->idx))
	 break;

      switch (chkrecord->type) {
	 case XCF_Delete: case XCF_Pop: case XCF_Push:
	    /* Delete/Push/Pop records imply a canceled selection */
	    return 0;
	 case XCF_Copy:
	 case XCF_Select:
	    srec = (uselection *)chkrecord->undodata;
	    areawin->selectlist = regen_selection(thisrecord->thisinst, srec);
	    areawin->selects = (areawin->selectlist) ? srec->number : 0;
	    return 0;
      }
   }
   return -1;
}

/*----------------------------------------------------------------------*/
/* This is similar to the above routine, but we just return a pointer	*/
/* to the index list in the uselection record.  This lets the undelete	*/
/* function restore the original ordering of parts that were deleted.	*/
/*----------------------------------------------------------------------*/

short *recover_selectlist(Undoptr thisrecord)
{
   Undoptr chkrecord;
   uselection *srec;

   for (chkrecord = thisrecord->next; chkrecord != NULL; chkrecord = chkrecord->next) {

      /* Selections may cross page changes, but only if in the same series */
      if ((chkrecord->thisinst != thisrecord->thisinst) &&
		(chkrecord->idx != thisrecord->idx))
	 break;

      switch (chkrecord->type) {
	 case XCF_Delete: case XCF_Pop: case XCF_Push:
	    /* Delete/Push/Pop records imply a canceled selection */
	    return NULL;
	 case XCF_Copy:
	    /* Copy is the same as Select, but the copied objects are	*/
	    /* always at the end of the element list.  Returning NULL	*/
	    /* is the same as declaring that elements should be		*/
	    /* appended to the end of the object's element list.	*/
	    return NULL;
	 case XCF_Select:
	    srec = (uselection *)chkrecord->undodata;
	    return srec->idx;
      }
   }
   return NULL;
}

/*----------------------------------------------------------------------*/
/* Put element "thiselem" back into object "thisobj".  This is only	*/
/* used in the case where the elements were previously at the end of	*/
/* the object's element list.						*/
/*----------------------------------------------------------------------*/

void undelete_one_element(objinstptr thisinst, genericptr thiselem)
{
   objectptr thisobj = thisinst->thisobject;

   PLIST_INCR(thisobj);
   *(thisobj->plist + thisobj->parts) = thiselem;
   thisobj->parts++;
}

/*----------------------------------------------------------------------*/
/* register_for_undo ---						*/
/*									*/
/*   Register an event with the undo handler.  This creates a record	*/
/*   based on the type, which is one of the XCF_* bindings (see		*/
/*   xcircuit.h for the list).  This is a variable-argument routine,	*/
/*   with the arguments dependent on the command.			*/
/*									*/
/*   thisinst is the instance of the object in which the event occurred	*/
/*   and is registered for every event type.				*/
/*									*/
/*   "mode" is UNDO_MORE if one or more undo records are expected to 	*/
/*   follow in a series, or UNDO_DONE if this completes a series, or is	*/
/*   as single undo event. The command-line command "undo series start"	*/
/*   forces all events to be UNDO_MORE until "undo series end" is	*/
/*   issued.								*/
/*----------------------------------------------------------------------*/

void register_for_undo(u_int type, u_char mode, objinstptr thisinst, ...)
{
   va_list args;
   int drawmode, nval, oval, *idata, snum, deltax, deltay, i;
   double dir;
   short *slist;
   objectptr delobj;
   objinstptr newinst;
   Undoptr newrecord;
   uselection *srec;
   editelement *erec;
   scaleinfo *escale;
   genericptr egen;
   XPoint *fpoint;
   double scale;

   /* Do not register new events while running undo/redo actions! */
   if (eventmode == UNDO_MODE) return;
 
   /* This action invalidates everything in the "redo" stack, so flush it */
   flush_redo_stack();

   /* Create the new record and push it onto the stack */
   newrecord = (Undoptr)malloc(sizeof(Undostack));
   newrecord->next = xobjs.undostack;
   newrecord->last = NULL;
   newrecord->type = type;
   newrecord->thisinst =  thisinst;
   newrecord->window = areawin;
   newrecord->undodata = (char *)NULL;
   newrecord->idata = 0;

   if (xobjs.undostack) {
      xobjs.undostack->last = newrecord;
      if (xobjs.undostack->idx < 0) {
	 xobjs.undostack->idx = -xobjs.undostack->idx;
	 newrecord->idx = xobjs.undostack->idx;
      }
      else
	 newrecord->idx = xobjs.undostack->idx + 1;
   }
   else
      newrecord->idx = 1;

   if (mode == UNDO_MORE || undo_collect > (u_char)0)
      newrecord->idx = -newrecord->idx;
   
   xobjs.undostack = newrecord;

   va_start(args, thisinst);

   switch(type) {
      case XCF_Delete:
	 /* 2 args:							*/
	 /*    delobj = pointer to object containing deleted entries	*/
	 /*    drawmode = true if elements should be erased		*/ 
	 delobj = va_arg(args, objectptr);
	 drawmode = va_arg(args, int);
	 newrecord->undodata = (char *)delobj;
	 newrecord->idata = drawmode;
	 break;

      case XCF_Select_Save:
	 /* 1 arg:							*/
	 /*    newobj = pointer to instance of new object		*/
	 newinst = va_arg(args, objinstptr);
	 newrecord->undodata = (char *)newinst;
	 break;

      case XCF_Page:
	 /* 2 args:							*/
	 /*	oval = original integer value				*/
	 /*	nval = new integer value				*/
	 oval = va_arg(args, int);
	 nval = va_arg(args, int);
	 idata = (int *)malloc(sizeof(int));
	 *idata = nval;
	 newrecord->undodata = (char *)idata;
	 newrecord->idata = oval;
	 break;

      case XCF_Pop:
	 /* No args; instance to pop is the instance passed. 		*/
	 break;

      case XCF_Push:
	 /* 1 arg:							*/
	 /*	newinst = object instance to push			*/
	 newinst = va_arg(args, objinstptr);
	 newrecord->undodata = (char *)newinst;
	 break;

      case XCF_Copy:
      case XCF_Select:
      case XCF_Library_Pop:
	 /* 2 args:							*/
	 /*	slist = current selection list (short *)		*/
	 /*	snum  = number of selections (int)			*/
	 slist = va_arg(args, short *);
	 snum = va_arg(args, int);
	 srec = remember_selection(thisinst, slist, snum);
	 newrecord->undodata = (char *)srec;
	 /* Fprintf(stdout, "Undo: registered selection or copy action\n"); */
	 break;

      case XCF_Box: case XCF_Arc: case XCF_Wire: case XCF_Text:
      case XCF_Pin_Label: case XCF_Pin_Global: case XCF_Info_Label:
      case XCF_Spline: case XCF_Dot: case XCF_Graphic: case XCF_Join:
	 /* 1 arg:							*/
	 /*	egen = element just created (genericptr)		*/
	 egen = va_arg(args, genericptr);
	 newrecord->undodata = (char *)egen;
	 break;

      case XCF_Edit:
	 /* 1 arg:							*/
	 /*	egen = element to be edited (genericptr)		*/
	 egen = va_arg(args, genericptr);

	 /* Create a copy of the element to save */
	 erec = (editelement *)malloc(sizeof(editelement));
	 erec->element = egen;
	 switch(egen->type) {
	    case LABEL:
	       erec->save.string =
			stringcopyall(((labelptr)egen)->string,
			areawin->topinstance);
	       newrecord->idata = ((labelptr)egen)->anchor;
	       break;
	    case POLYGON:
	       newrecord->idata = ((polyptr)egen)->number;
	       erec->save.points = copypoints(((polyptr)egen)->points,
			newrecord->idata);
	       break;
	    case SPLINE:
	       erec->save.points =
			copypoints((pointlist)((splineptr)egen)->ctrl, 4);
	       break;
	    case OBJINST:
	       erec->save.instpos = ((objinstptr)egen)->position;
	       break;
	    case ARC:
	       erec->save.arcspecs = (arcinfo *)malloc(sizeof(arcinfo));
	       erec->save.arcspecs->angle1 = ((arcptr)egen)->angle1;
	       erec->save.arcspecs->angle2 = ((arcptr)egen)->angle2;
	       erec->save.arcspecs->radius = ((arcptr)egen)->radius;
	       erec->save.arcspecs->yaxis = ((arcptr)egen)->yaxis;
	       erec->save.arcspecs->position = ((arcptr)egen)->position;
	       break;
	    case PATH:
	       newrecord->idata = ((pathptr)egen)->parts;  /* not needed? */
	       erec->save.pathspecs = (pathinfo *)malloc(newrecord->idata *
			sizeof(pathinfo));
	       for (i = 0; i < newrecord->idata; i++) {
		  pathinfo *ppi = erec->save.pathspecs + i;
		  genericptr *pgen = ((pathptr)egen)->plist + i;
		  switch (ELEMENTTYPE(*pgen)) {
		     case POLYGON:
			ppi->number = (TOPOLY(pgen))->number;
			ppi->points = copypoints((TOPOLY(pgen))->points,
				ppi->number);
			break;
		     case SPLINE:
			ppi->number = 4;
			ppi->points = copypoints((pointlist)
				(TOSPLINE(pgen))->ctrl, 4);
			break;
		  }
	       }
	       break;
	 }
	 newrecord->undodata = (char *)erec;
	 break;

      case XCF_ChangeStyle:
      case XCF_Anchor:
      case XCF_Color:
	 /* 2 args:							*/
	 /*	egen = element that was changed (with new value)	*/
	 /*	oval = old style value					*/
	 egen = va_arg(args, genericptr);
	 oval = va_arg(args, int);
	 newrecord->undodata = (char *)egen;
	 newrecord->idata = oval;
	 break;

      case XCF_Rescale:
	 /* 2 args:							*/
	 /*	egen = element that was changed (with new value)	*/
	 /*	ofloat = old float value				*/

	 egen = va_arg(args, genericptr);
	 scale = va_arg(args, double); 		/* warning! only takes "double"! */

	 escale = (scaleinfo *)malloc(sizeof(scaleinfo));
	 escale->element = egen;
	 escale->scale = (float)scale;

	 newrecord->undodata = (char *)escale;
	 break;

      case XCF_Flip_X: case XCF_Flip_Y:
	 /* 1 arg:							*/
	 /*	fpoint = point of flip (XPoint *)			*/
	 fpoint = va_arg(args, XPoint *);
	 newrecord->undodata = (char *)malloc(sizeof(XPoint));
	 ((XPoint *)newrecord->undodata)->x = fpoint->x;
	 ((XPoint *)newrecord->undodata)->y = fpoint->y;
	 break;

      case XCF_Rotate:
	 /* 2 args:							*/
	 /*	fpoint = point of flip (XPoint *)			*/
	 /*	dir = direction and amount of rotation (float)		*/
	 fpoint = va_arg(args, XPoint *);
	 dir = va_arg(args, double);
	 newrecord->undodata = (char *)malloc(sizeof(rotateinfo));
	 ((rotateinfo *)newrecord->undodata)->rotpos.x = fpoint->x;
	 ((rotateinfo *)newrecord->undodata)->rotpos.y = fpoint->y;
	 ((rotateinfo *)newrecord->undodata)->rotation = (float)dir;
	 break;

      case XCF_Move:
	 /* 2 args:							*/
	 /*	deltax = change in x position (int)			*/
	 /*	deltay = change in y position (int)			*/
	 deltax = va_arg(args, int);
	 deltay = va_arg(args, int);
	 newrecord->undodata = (char *)malloc(sizeof(XPoint));
	 ((XPoint *)newrecord->undodata)->x = deltax;
	 ((XPoint *)newrecord->undodata)->y = deltay;
	 /* Fprintf(stdout, "Undo: registered move of delta (%d, %d)\n",
			deltax, deltay); */
	 break;

       case XCF_Reorder:
	 /* 2 args:							*/
	 /*	slist = "before" order of elements			*/
	 /*	snum = number of elements in list (= # parts)		*/
	 slist = va_arg(args, short *);
	 snum = va_arg(args, int);
	 newrecord->undodata = (char *)slist;
	 newrecord->idata = snum;
   }

   va_end(args);
}

/*----------------------------------------------------------------------*/
/* undo_one_action ---							*/
/*	Play undo record back one in the stack.				*/
/*----------------------------------------------------------------------*/

short undo_one_action()
{
  Undoptr thisrecord; /* , chkrecord; (jdk) */
   objectptr thisobj;
   objinstptr thisinst;
   uselection *srec;
   editelement *erec;
   scaleinfo *escale;
   short *slist;
   XPoint *delta, position;
   int i, j, snum;
   int savemode;
   float fnum;
   genericptr egen;
   labelptr thislabel;
   pathptr thispath;
   polyptr thispoly;
   arcptr thisarc;
   splineptr thisspline;
   graphicptr thisgraphic;
   Boolean need_redraw;
   XCWindowData *savewindow = areawin;

   /* Undo the recorded action and shift the undo record pointer.	*/

   thisrecord = xobjs.undostack;
   if (thisrecord == NULL) {
      Fprintf(stderr, "Nothing to undo!\n");
      return 0;
   }

   xobjs.undostack = thisrecord->next;
   xobjs.redostack = thisrecord;

   /* Set window, if event occurred in a different window */
   if (setwindow(thisrecord->window) == FALSE) {
      Wprintf("Error:  Undo event in nonexistant window!  Flushing stack.\n");
      flush_undo_stack();
      return 0;
   }

   /* Setting eventmode to UNDO_MODE prevents register_for_undo() from 	*/
   /* being called again while executing the event.			*/

   savemode = eventmode;
   eventmode = UNDO_MODE;

   /* type-dependent part */

   switch(thisrecord->type) {
      case XCF_Delete:
	 unselect_all();
	 thisobj = (objectptr)thisrecord->undodata;
	 areawin->selects = thisobj->parts;
	 areawin->selectlist = xc_undelete(thisrecord->thisinst,
			thisobj, (short)thisrecord->idata,
			recover_selectlist(thisrecord));
	 srec = remember_selection(thisrecord->thisinst, areawin->selectlist,
			areawin->selects);
	 thisrecord->undodata = (char *)srec;
	 draw_all_selected();
	 break;

      /* To be finished:  Needs to remove the object & instance from	*/
      /* the library and library page.  Should keep the empty object	*/
      /* around so we have the name.					*/

      case XCF_Select_Save:
	 unselect_all();
	 thisinst = (objinstptr)thisrecord->undodata;
	 thisobj = thisinst->thisobject;

	 /* Remove the instance */
	 i = thisrecord->thisinst->thisobject->parts - 1;
	 if ((genericptr)thisinst != *(thisrecord->thisinst->thisobject->plist + i)) {
	    Fprintf(stderr, "Error: No such element!\n");
	    thisrecord->undodata = NULL;
	    break;
	 }
	 else {
	    delete_one_element(thisrecord->thisinst, (genericptr)thisinst);
	 }

	 /* Put back all the parts */
	 areawin->selects = thisobj->parts;
	 areawin->selectlist = xc_undelete(thisrecord->thisinst,
			thisobj, (short)thisrecord->idata,
			recover_selectlist(thisrecord));
	 srec = remember_selection(thisrecord->thisinst, areawin->selectlist,
			areawin->selects);
	 thisrecord->undodata = (char *)srec;
	 draw_all_selected();
	 break;

      case XCF_Push:
	 popobject(areawin->area, 0, NULL);
	 break;

      case XCF_Pop:
	 pushobject((objinstptr)thisrecord->thisinst);
	 break;

      case XCF_Page:
	 newpage(thisrecord->idata);
	 break;

      case XCF_Anchor:
	 thislabel = (labelptr)(thisrecord->undodata);
	 snum = thisrecord->idata;
	 thisrecord->idata = thislabel->anchor;
	 thislabel->anchor = snum;
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Select:

	 /* If there was a previous selection in the undo list,	*/
	 /* revert to it.					*/
	 
	 need_redraw = (areawin->selects > 0) ? True : False;
	 select_previous(thisrecord);
	 if (need_redraw) {
	    areawin->redraw_needed = True;
	    drawarea(areawin->area, NULL, NULL);
	 }
	 else
	    draw_all_selected();
	 break;

      case XCF_Box: case XCF_Arc: case XCF_Wire: case XCF_Text:
		case XCF_Pin_Label: case XCF_Pin_Global: case XCF_Info_Label:
		case XCF_Spline: case XCF_Dot: case XCF_Graphic: case XCF_Join:
	 egen = (genericptr)thisrecord->undodata;
	 i = thisrecord->thisinst->thisobject->parts - 1;
	 if (egen != *(thisrecord->thisinst->thisobject->plist + i)) {
	    Fprintf(stderr, "Error: No such element!\n");
	    thisrecord->undodata = NULL;
	 }
	 else {
	    delete_one_element(thisrecord->thisinst, egen);
	    areawin->redraw_needed = True;
	    drawarea(areawin->area, NULL, NULL);
	 }
	 break;

      case XCF_Edit:
	 erec = (editelement *)thisrecord->undodata;
	 switch (erec->element->type) {
	    case LABEL: {
	       stringpart *tmpstr;
	       int tmpanchor;
	       labelptr elab = (labelptr)(erec->element);
	       undrawtext(elab);
	       tmpstr = elab->string;
	       tmpanchor = (int)elab->anchor;
	       elab->string = stringcopyback(erec->save.string,
				thisrecord->thisinst);
	       elab->anchor = (short)thisrecord->idata;
	       erec->save.string = tmpstr;
	       thisrecord->idata = tmpanchor;
	       resolveparams(thisrecord->thisinst);
	       redrawtext(elab);
	    } break;
	    case ARC: {
	       arcinfo tmpinfo;
	       arcptr earc = (arcptr)(erec->element);
	       tmpinfo.angle1 = earc->angle1; 
	       tmpinfo.angle2 = earc->angle2; 
	       tmpinfo.radius = earc->radius; 
	       tmpinfo.yaxis = earc->yaxis; 
	       tmpinfo.position = earc->position; 
	       earc->angle1 = erec->save.arcspecs->angle1;
	       earc->angle2 = erec->save.arcspecs->angle2;
	       earc->radius = erec->save.arcspecs->radius;
	       earc->yaxis = erec->save.arcspecs->yaxis;
	       earc->position = erec->save.arcspecs->position;
	       *(erec->save.arcspecs) = tmpinfo;
	       calcarc(earc);
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    } break;
	    case OBJINST: {
	       XPoint tmppt;
	       objinstptr einst = (objinstptr)(erec->element);
	       // Swap instance and saved positions.
	       tmppt = einst->position;
	       einst->position = erec->save.instpos;
	       erec->save.instpos = tmppt;
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    }  break;
	    case POLYGON: {
	       pointlist tmppts;
	       int tmpnum;
	       polyptr epoly = (polyptr)(erec->element);
	       tmppts = epoly->points;
	       tmpnum = epoly->number;
	       epoly->points = erec->save.points;
	       epoly->number = thisrecord->idata;
	       erec->save.points = tmppts;
	       thisrecord->idata = tmpnum;
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    } break;
	    case SPLINE: {
	       pointlist tmppts;
	       splineptr espline = (splineptr)(erec->element);
	       tmppts = copypoints((pointlist)espline->ctrl, 4);
	       for (i = 0; i < 4; i++)
		   espline->ctrl[i] = *(erec->save.points + i);
	       free(erec->save.points);
	       erec->save.points = tmppts;
	       calcspline(espline);
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    } break;
	    case PATH: {
	       pointlist tmppts;
	       int tmpnum;
	       polyptr epoly;
	       splineptr espline;
	       pathptr epath = (pathptr)(erec->element);
	       for (i = 0; i < epath->parts; i++) {
		  genericptr ggen = *(epath->plist + i);
		  switch (ELEMENTTYPE(ggen)) {
		     case POLYGON:
			epoly = (polyptr)ggen;
		        tmppts = epoly->points;
			tmpnum = epoly->number;
			epoly->points = (erec->save.pathspecs + i)->points;
			epoly->number = (erec->save.pathspecs + i)->number;
			(erec->save.pathspecs + i)->points = tmppts;
			(erec->save.pathspecs + i)->number = tmpnum;
			break;
		     case SPLINE:
			espline = (splineptr)ggen;
			tmppts = copypoints((pointlist)espline->ctrl, 4);
			for (j = 0; j < 4; j++)
			   espline->ctrl[j] = *((erec->save.pathspecs + i)->points + j);
			free((erec->save.pathspecs + i)->points);
		        (erec->save.pathspecs + i)->points = tmppts;
			calcspline(espline);
			break;
		  }
	       }
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    }
	 }
	 break;

      case XCF_Library_Pop:
	 srec = (uselection *)thisrecord->undodata;
	 slist = regen_selection(thisrecord->thisinst, srec);
	 thisobj = delete_element(thisrecord->thisinst, slist,
			srec->number, DRAW);
	 free(slist);
	 thisrecord->undodata = (char *)thisobj;
	 break;

      case XCF_Copy:
	 clearselects_noundo();
	 srec = (uselection *)thisrecord->undodata;
	 slist = regen_selection(thisrecord->thisinst, srec);
	 thisobj = delete_element(thisrecord->thisinst, slist,
			srec->number, DRAW);
	 free(slist);
	 thisrecord->undodata = (char *)thisobj;

	 /* Revert selection to previously selected */
	 select_previous(thisrecord);
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 draw_all_selected();
	 break;

      case XCF_ChangeStyle:
	 /* Style changes */
	 egen = (genericptr)thisrecord->undodata;
	 snum = thisrecord->idata;
	 switch(egen->type) {
	    case PATH:
	       thispath = (pathptr)(thisrecord->undodata);
	       thisrecord->idata = thispath->style;
	       thispath->style = snum;
	       break;
	    case POLYGON:
	       thispoly = (polyptr)(thisrecord->undodata);
	       thisrecord->idata = thispoly->style;
	       thispoly->style = snum;
	       break;
	    case ARC:
	       thisarc = (arcptr)(thisrecord->undodata);
	       thisrecord->idata = thisarc->style;
	       thisarc->style = snum;
	       break;
	    case SPLINE:
	       thisspline = (splineptr)(thisrecord->undodata);
	       thisrecord->idata = thisspline->style;
	       thisspline->style = snum;
	       break;
	 }
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Color:
	 /* Color changes */
	 egen = (genericptr)thisrecord->undodata;
	 snum = thisrecord->idata;
	 switch(egen->type) {
	    case PATH:
	       thispath = (pathptr)(thisrecord->undodata);
	       thisrecord->idata = thispath->color;
	       thispath->color = snum;
	       break;
	    case POLYGON:
	       thispoly = (polyptr)(thisrecord->undodata);
	       thisrecord->idata = thispoly->color;
	       thispoly->color = snum;
	       break;
	    case ARC:
	       thisarc = (arcptr)(thisrecord->undodata);
	       thisrecord->idata = thisarc->color;
	       thisarc->color = snum;
	       break;
	    case SPLINE:
	       thisspline = (splineptr)(thisrecord->undodata);
	       thisrecord->idata = thisspline->color;
	       thisspline->color = snum;
	       break;
	 }
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Rescale:
	 escale = (scaleinfo *)thisrecord->undodata;
	 egen = escale->element;
	 fnum = escale->scale;
	 switch(egen->type) {
	    case PATH:
	       thispath = (pathptr)egen;
	       escale->scale = thispath->width;
	       thispath->width = fnum;
	       break;
	    case POLYGON:
	       thispoly = (polyptr)egen;
	       escale->scale = thispoly->width;
	       thispoly->width = fnum;
	       break;
	    case ARC:
	       thisarc = (arcptr)egen;
	       escale->scale = thisarc->width;
	       thisarc->width = fnum;
	       break;
	    case SPLINE:
	       thisspline = (splineptr)egen;
	       escale->scale = thisspline->width;
	       thisspline->width = fnum;
	       break;
	    case OBJINST:
	       thisinst = (objinstptr)egen;
	       escale->scale = thisinst->scale;
	       thisinst->scale = fnum;
	       break;
	    case GRAPHIC:
	       thisgraphic = (graphicptr)egen;
	       escale->scale = thisgraphic->scale;
	       thisgraphic->scale = fnum;
#ifndef HAVE_CAIRO
	       thisgraphic->valid = FALSE;
#endif /* HAVE_CAIRO */
	       break;
	    case LABEL:
	       thislabel = (labelptr)egen;
	       escale->scale = thislabel->scale;
	       thislabel->scale = fnum;
	       break;
	 }
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Flip_X:
	 position = *((XPoint *)thisrecord->undodata);
	 elementflip(&position);
	 break;

      case XCF_Flip_Y:
	 position = *((XPoint *)thisrecord->undodata);
	 elementvflip(&position);
	 break;

      case XCF_Rotate:
	 position = ((rotateinfo *)thisrecord->undodata)->rotpos;
	 elementrotate(-((rotateinfo *)thisrecord->undodata)->rotation, &position);
	 break;

      case XCF_Move:
	 delta = (XPoint *)thisrecord->undodata;
	 select_connected_pins();
	 placeselects(-(delta->x), -(delta->y), NULL);
	 reset_cycles();
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 draw_all_selected();
	 break;

       case XCF_Reorder:
	 reorder_selection(thisrecord);
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      default:
	 Fprintf(stderr, "Undo not implemented for this action!\n");
	 break;
   }

   /* Does this need to be set on a per-event-type basis? */
   switch (savemode) {
      case CATALOG_MODE:
      case CATTEXT_MODE:
	 eventmode = CATALOG_MODE;
	 break;
      default:
	 eventmode = NORMAL_MODE;
	 break;
   }

   /* Diagnostic, to check if all multiple-event undo series are resolved */
   if (thisrecord->idx < 0) {
      Fprintf(stderr, "Warning:  Unfinished undo series in stack!\n");
      thisrecord->idx = -thisrecord->idx;
   }
   areawin = savewindow;
   return thisrecord->idx;
}

/*----------------------------------------------------------------------*/
/* undo_finish_series ---						*/
/*	Complete a possibly incomplete undo series by forcing the	*/
/*	topmost entry to have a positive index.				*/
/*	Note that for "undo series start|end" to work, undo_collect	*/
/*	must be set to 0 prior to calling undo_finish_series().		*/
/*----------------------------------------------------------------------*/

void undo_finish_series()
{
   if (undo_collect == (u_char)0)
      if (xobjs.undostack && xobjs.undostack->idx < 0)
         xobjs.undostack->idx = -xobjs.undostack->idx;
}

/*----------------------------------------------------------------------*/
/* undo_action ---							*/
/*	Play undo record back to the completion of a series.		*/
/*----------------------------------------------------------------------*/

void undo_action()
{
   short idx;

   // Cannot undo while in the middle of an undo series.  Failsafe.
   if (undo_collect != (u_char)0) return;

   idx = undo_one_action();
   while (xobjs.undostack && xobjs.undostack->idx == idx)
      undo_one_action();
}

/*----------------------------------------------------------------------*/
/* redo_one_action ---							*/
/*	Play undo record forward one in the stack.			*/
/*----------------------------------------------------------------------*/

short redo_one_action()
{
   Undoptr thisrecord;
   objectptr thisobj;
   genericptr egen;
   short *slist;
   XPoint *delta, position;
   uselection *srec;
   editelement *erec;
   scaleinfo *escale;
   int i, j, snum;
   int savemode;
   float fnum;
   labelptr thislabel;
   arcptr thisarc;
   pathptr thispath;
   splineptr thisspline;
   polyptr thispoly;
   graphicptr thisgraphic;
   objinstptr thisinst;
   XCWindowData *savewindow = areawin;

   /* Undo the recorded action and shift the undo record pointer.	*/

   thisrecord = xobjs.redostack;
   if (thisrecord == NULL) {
      Fprintf(stderr, "Nothing to redo!\n");
      return 0;
   }
   xobjs.undostack = thisrecord;
   xobjs.redostack = thisrecord->last;

   /* Set window, if event occurred in a different window */
   if (setwindow(thisrecord->window) == FALSE) {
      Wprintf("Error:  Undo event in nonexistant window!  Flushing stack.\n");
      flush_undo_stack();
      return 0;
   }

   savemode = eventmode;
   eventmode = UNDO_MODE;

   /* type-dependent part */

   switch(thisrecord->type) {
      case XCF_Delete:
	 srec = (uselection *)thisrecord->undodata;
	 slist = regen_selection(thisrecord->thisinst, srec);
	 thisobj = delete_element(thisrecord->thisinst, slist,
		srec->number, DRAW);
	 free(slist);
	 thisrecord->undodata = (char *)thisobj;
	 thisrecord->idata = (int)DRAW;
	 unselect_all();
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      /* Unfinished! */
      case XCF_Select_Save:
	 srec = (uselection *)thisrecord->undodata;
	 slist = regen_selection(thisrecord->thisinst, srec);
	 break;

      case XCF_Page:
	 newpage(*((int *)thisrecord->undodata));
	 break;

      case XCF_Anchor:
	 thislabel = (labelptr)(thisrecord->undodata);
	 snum = thisrecord->idata;
	 thisrecord->idata = thislabel->anchor;
	 thislabel->anchor = snum;
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Pop:
	 popobject(areawin->area, 0, NULL);
	 break;

      case XCF_Push:
	 pushobject((objinstptr)thisrecord->undodata);
	 break;

      case XCF_Select:
	 unselect_all();
	 srec = (uselection *)thisrecord->undodata;
	 areawin->selectlist = regen_selection(thisrecord->thisinst, srec);
	 areawin->selects = (areawin->selectlist) ? srec->number : 0;
	 draw_all_selected();
	 break;

      case XCF_Library_Pop:
	 thisobj = (objectptr)thisrecord->undodata;
	 if (thisobj != NULL) {
	    unselect_all();
	    snum = thisobj->parts;
	    slist = xc_undelete(thisrecord->thisinst, thisobj, DRAW, NULL);
	    thisrecord->undodata = (char *)remember_selection(thisrecord->thisinst,
			slist, snum);
	    free(slist);
	 }
	 break;

      case XCF_Copy:
	 thisobj = (objectptr)thisrecord->undodata;
	 if (thisobj != NULL) {
	    unselect_all();
	    areawin->selects = thisobj->parts;
	    areawin->selectlist = xc_undelete(thisrecord->thisinst, thisobj, DRAW,
			NULL);
	    thisrecord->undodata = (char *)remember_selection(thisrecord->thisinst,
			areawin->selectlist, areawin->selects);
	    draw_all_selected();
	 }
	 break;

      case XCF_Box: case XCF_Arc: case XCF_Wire: case XCF_Text:
      case XCF_Pin_Label: case XCF_Pin_Global: case XCF_Info_Label:
      case XCF_Spline: case XCF_Dot: case XCF_Graphic: case XCF_Join:
	 egen = (genericptr)thisrecord->undodata;
	 undelete_one_element(thisrecord->thisinst, egen);
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Edit:
	 erec = (editelement *)thisrecord->undodata;
	 switch (erec->element->type) {
	    case LABEL: {
	       stringpart *tmpstr;
	       int tmpanchor;
	       labelptr elab = (labelptr)(erec->element);
	       undrawtext(elab);
	       tmpstr = elab->string;
	       tmpanchor = (int)elab->anchor;
	       elab->string = stringcopyback(erec->save.string,
				thisrecord->thisinst);
	       elab->anchor = (short)thisrecord->idata;
	       erec->save.string = tmpstr;
	       thisrecord->idata = tmpanchor;
	       resolveparams(thisrecord->thisinst);
	       redrawtext(elab);
	    } break;
	    case OBJINST: {
	       XPoint tmppt;
	       objinstptr einst = (objinstptr)(erec->element);
	       // Swap instance position and saved position
	       tmppt = einst->position;
	       einst->position = erec->save.instpos;
	       erec->save.instpos = tmppt;
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    }  break;
	    case ARC: {
	       arcinfo tmpinfo;
	       arcptr earc = (arcptr)(erec->element);
	       tmpinfo.angle1 = earc->angle1; 
	       tmpinfo.angle2 = earc->angle2; 
	       tmpinfo.radius = earc->radius; 
	       tmpinfo.yaxis = earc->yaxis; 
	       tmpinfo.position = earc->position; 
	       earc->angle1 = erec->save.arcspecs->angle1;
	       earc->angle2 = erec->save.arcspecs->angle2;
	       earc->radius = erec->save.arcspecs->radius;
	       earc->yaxis = erec->save.arcspecs->yaxis;
	       earc->position = erec->save.arcspecs->position;
	       *(erec->save.arcspecs) = tmpinfo;
	       calcarc(earc);
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    } break;
	    case POLYGON: {
	       pointlist tmppts;
	       int tmpnum;
	       polyptr epoly = (polyptr)(erec->element);
	       tmppts = epoly->points;
	       tmpnum = epoly->number;
	       epoly->points = erec->save.points;
	       epoly->number = thisrecord->idata;
	       erec->save.points = tmppts;
	       thisrecord->idata = tmpnum;
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    } break;
	    case SPLINE: {
	       pointlist tmppts;
	       splineptr espline = (splineptr)(erec->element);
	       tmppts = copypoints((pointlist)espline->ctrl, 4);
	       for (i = 0; i < 4; i++)
		   espline->ctrl[i] = *(erec->save.points + i);
	       free(erec->save.points);
	       erec->save.points = tmppts;
	       calcspline(espline);
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    } break;
	    case PATH: {
	       pointlist tmppts;
	       int tmpnum;
	       polyptr epoly;
	       splineptr espline;
	       pathptr epath = (pathptr)(erec->element);
	       for (i = 0; i < epath->parts; i++) {
		  genericptr ggen = *(epath->plist + i);
		  switch (ELEMENTTYPE(ggen)) {
		     case POLYGON:
			epoly = (polyptr)ggen;
		        tmppts = epoly->points;
			tmpnum = epoly->number;
			epoly->points = (erec->save.pathspecs + i)->points;
			epoly->number = (erec->save.pathspecs + i)->number;
			(erec->save.pathspecs + i)->points = tmppts;
			(erec->save.pathspecs + i)->number = tmpnum;
			break;
		     case SPLINE:
			espline = (splineptr)ggen;
			tmppts = copypoints((pointlist)espline->ctrl, 4);
			for (j = 0; j < 4; j++)
			   espline->ctrl[j] = *((erec->save.pathspecs + i)->points + j);
			free((erec->save.pathspecs + i)->points);
		        (erec->save.pathspecs + i)->points = tmppts;
			calcspline(espline);
			break;
		  }
	       }
	       areawin->redraw_needed = True;
	       drawarea(areawin->area, NULL, NULL);
	    }
	 }
	 break;

      case XCF_Flip_X:
	 position = *((XPoint *)thisrecord->undodata);
	 elementflip(&position);
	 break;

      case XCF_Flip_Y:
	 position = *((XPoint *)thisrecord->undodata);
	 elementvflip(&position);
	 break;

      case XCF_ChangeStyle:
	 /* Style changes */
	 egen = (genericptr)thisrecord->undodata;
	 snum = thisrecord->idata;
	 switch(egen->type) {
	    case PATH:
	       thispath = (pathptr)(thisrecord->undodata);
	       thisrecord->idata = thispath->style;
	       thispath->style = snum;
	       break;
	    case POLYGON:
	       thispoly = (polyptr)(thisrecord->undodata);
	       thisrecord->idata = thispoly->style;
	       thispoly->style = snum;
	       break;
	    case ARC:
	       thisarc = (arcptr)(thisrecord->undodata);
	       thisrecord->idata = thisarc->style;
	       thisarc->style = snum;
	       break;
	    case SPLINE:
	       thisspline = (splineptr)(thisrecord->undodata);
	       thisrecord->idata = thisspline->style;
	       thisspline->style = snum;
	       break;
	 }
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Color:
	 /* Color changes */
	 egen = (genericptr)thisrecord->undodata;
	 snum = thisrecord->idata;
	 switch(egen->type) {
	    case PATH:
	       thispath = (pathptr)(thisrecord->undodata);
	       thisrecord->idata = thispath->color;
	       thispath->color = snum;
	       break;
	    case POLYGON:
	       thispoly = (polyptr)(thisrecord->undodata);
	       thisrecord->idata = thispoly->color;
	       thispoly->color = snum;
	       break;
	    case ARC:
	       thisarc = (arcptr)(thisrecord->undodata);
	       thisrecord->idata = thisarc->color;
	       thisarc->color = snum;
	       break;
	    case SPLINE:
	       thisspline = (splineptr)(thisrecord->undodata);
	       thisrecord->idata = thisspline->color;
	       thisspline->color = snum;
	       break;
	 }
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Rescale:
	 escale = (scaleinfo *)thisrecord->undodata;
	 egen = escale->element;
	 fnum = escale->scale;
	 switch(egen->type) {
	    case PATH:
	       thispath = (pathptr)egen;
	       escale->scale = thispath->width;
	       thispath->width = fnum;
	       break;
	    case POLYGON:
	       thispoly = (polyptr)egen;
	       escale->scale = thispoly->width;
	       thispoly->width = fnum;
	       break;
	    case ARC:
	       thisarc = (arcptr)egen;
	       escale->scale = thisarc->width;
	       thisarc->width = fnum;
	       break;
	    case SPLINE:
	       thisspline = (splineptr)egen;
	       escale->scale = thisspline->width;
	       thisspline->width = fnum;
	       break;
	    case OBJINST:
	       thisinst = (objinstptr)egen;
	       escale->scale = thisinst->scale;
	       thisinst->scale = fnum;
	       break;
	    case GRAPHIC:
	       thisgraphic = (graphicptr)egen;
	       escale->scale = thisgraphic->scale;
	       thisgraphic->scale = fnum;
#ifndef HAVE_CAIRO
	       thisgraphic->valid = FALSE;
#endif /* HAVE_CAIRO */
	       break;
	    case LABEL:
	       thislabel = (labelptr)egen;
	       escale->scale = thislabel->scale;
	       thislabel->scale = fnum;
	       break;
	 }
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Rotate:
	 position = ((rotateinfo *)thisrecord->undodata)->rotpos;
	 elementrotate(((rotateinfo *)thisrecord->undodata)->rotation, &position);
	 break;

      case XCF_Move:
	 delta = (XPoint *)thisrecord->undodata;
	 select_connected_pins();
	 placeselects(delta->x, delta->y, NULL);
	 reset_cycles();
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case XCF_Reorder:
	 reorder_selection(thisrecord);
	 areawin->redraw_needed = True;
	 drawarea(areawin->area, NULL, NULL);
	 break;

      default:
	 Fprintf(stderr, "Undo not implemented for this action!\n");
	 break;
   }

   /* Does this need to be set on a per-event-type basis? */
   switch (savemode) {
      case CATALOG_MODE:
      case CATTEXT_MODE:
	 eventmode = CATALOG_MODE;
	 break;
      default:
	 eventmode = NORMAL_MODE;
	 break;
   }

   areawin = savewindow;

   return thisrecord->idx;
}

/*----------------------------------------------------------------------*/
/* redo_action ---							*/
/*	Play undo record forward to the completion of a series.		*/
/*----------------------------------------------------------------------*/

void redo_action()
{
   short idx;

   // Cannot redo while in the middle of an undo series.  Failsafe.
   if (undo_collect != (u_char)0) return;

   idx = redo_one_action();
   while (xobjs.redostack && xobjs.redostack->idx == idx)
      redo_one_action();
}

/*----------------------------------------------------------------------*/
/* flush_redo_stack ---							*/
/*	Free all memory allocated to the redo stack due to the		*/
/* 	insertion of a new undo record.					*/
/*----------------------------------------------------------------------*/

void flush_redo_stack()
{
   Undoptr thisrecord, nextrecord;

   if (xobjs.redostack == NULL) return;	/* no redo stack */

   thisrecord = xobjs.redostack;

   while (thisrecord != NULL) {
      nextrecord = thisrecord->last;
      free_redo_record(thisrecord);
      thisrecord = nextrecord;
   }
   xobjs.redostack = NULL;

   if (xobjs.undostack)
      xobjs.undostack->last = NULL;
}

/*----------------------------------------------------------------------*/
/* flush_undo_stack ---							*/
/*	Free all memory allocated to the undo and redo stacks.		*/
/*----------------------------------------------------------------------*/

void flush_undo_stack()
{
   Undoptr thisrecord, nextrecord;

   flush_redo_stack();

   thisrecord = xobjs.undostack;

   while (thisrecord != NULL) {
      nextrecord = thisrecord->next;
      free_undo_record(thisrecord);
      thisrecord = nextrecord;
   }
   xobjs.undostack = NULL;
}

/*----------------------------------------------------------------------*/
/* free_undo_data ---							*/
/*	Free memory allocated to the "undodata" part of the undo	*/
/*	record, based on the record type.				*/
/*									*/
/* "mode" specifies whether this is for an "undo" or a "redo" event.	*/
/*									*/
/*	Note that the action taken for a specific record may *NOT* be	*/
/*	the same for a record in the undo stack as it is for a record	*/
/*	in the redo stack, because the data types are changed when	*/
/*	moving from one record to the next.				*/
/*----------------------------------------------------------------------*/

void free_undo_data(Undoptr thisrecord, u_char mode)
{
   u_int type;
   objectptr uobj;
   uselection *srec;
   editelement *erec;

   type = thisrecord->type;
   switch (type) {
      case XCF_Delete:
	 if (mode == MODE_UNDO) {
	    uobj = (objectptr)thisrecord->undodata;
	    reset(uobj, DESTROY);
	 }
	 else { /* MODE_REDO */
	    srec = (uselection *)thisrecord->undodata;
	    free_selection(srec);
	 }
	 break;

      case XCF_Box: case XCF_Arc: case XCF_Wire: case XCF_Text:
      case XCF_Pin_Label: case XCF_Pin_Global: case XCF_Info_Label:
      case XCF_Spline: case XCF_Dot: case XCF_Graphic: case XCF_Join:
	 /* if MODE_UNDO, the element is on the page, so don't destroy it! */
	 if (mode == MODE_REDO)
	    free(thisrecord->undodata);
	 break;

      case XCF_Edit:
	 erec = (editelement *)thisrecord->undodata;
	 free_editelement(thisrecord);
	 break;

      case XCF_Copy:
      case XCF_Library_Pop:
	 if (mode == MODE_UNDO) {
	    srec = (uselection *)thisrecord->undodata;
	    free_selection(srec);
	 }
	 else { /* MODE_REDO */
	    uobj = (objectptr)thisrecord->undodata;
	    reset(uobj, DESTROY);
	 }
	 break;

      case XCF_Push:
      case XCF_ChangeStyle:
      case XCF_Anchor:
      case XCF_Color:
	 /* Do nothing --- undodata points to a valid element */
	 break;

      case XCF_Select:
	 srec = (uselection *)thisrecord->undodata;
	 free_selection(srec);
	 break;

      default:
	 if (thisrecord->undodata != NULL)
	    free(thisrecord->undodata);
	 break;
   }
   thisrecord->undodata = NULL;
}


/*----------------------------------------------------------------------*/
/* free_undo_record ---							*/
/*	Free allocated memory for one record in the undo stack.		*/
/*----------------------------------------------------------------------*/

void free_undo_record(Undoptr thisrecord)
{
  /* Undoptr nextrecord, lastrecord; (jdk) */

   /* Reset the master list pointers */

   if (xobjs.undostack == thisrecord)
      xobjs.undostack = thisrecord->next;

   /* Relink the stack pointers */

   if (thisrecord->last)
      thisrecord->last->next = thisrecord->next;

   if (thisrecord->next)
      thisrecord->next->last = thisrecord->last;

   /* Free memory allocated to the record */

   free_undo_data(thisrecord, MODE_UNDO);
   free(thisrecord);
}

/*----------------------------------------------------------------------*/
/* free_redo_record ---							*/
/*	Free allocated memory for one record in the redo stack.		*/
/*----------------------------------------------------------------------*/

void free_redo_record(Undoptr thisrecord)
{
  /* Undoptr nextrecord, lastrecord; (jdk) */

   /* Reset the master list pointers */

   if (xobjs.redostack == thisrecord)
      xobjs.redostack = thisrecord->last;

   /* Relink the stack pointers */

   if (thisrecord->next)
      thisrecord->next->last = thisrecord->last;

   if (thisrecord->last)
      thisrecord->last->next = thisrecord->next;

   /* Free memory allocated to the record */

   free_undo_data(thisrecord, MODE_REDO);
   free(thisrecord);
}

/*----------------------------------------------------------------------*/
/* truncate_undo_stack ---						*/
/*	If the limit MAX_UNDO_EVENTS has been reached, discard the	*/
/*	last undo series on the stack (index = 1) and renumber the	*/
/*	others by decrementing.						*/
/*----------------------------------------------------------------------*/

void truncate_undo_stack()
{
   Undoptr thisrecord, nextrecord;

   thisrecord = xobjs.undostack;
   while (thisrecord != NULL) {
      nextrecord = thisrecord->next;
      if (thisrecord->idx > 1)
	 thisrecord->idx--;
      else
         free_undo_record(thisrecord);
      thisrecord = nextrecord;
   }
}

#ifndef TCL_WRAPPER

/* Calls from the Xt menus (see menus.h)				*/
/* These are wrappers for undo_action and redo_action			*/

void undo_call(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   undo_action();
}

void redo_call(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   redo_action();
}

#endif
/*----------------------------------------------------------------------*/
