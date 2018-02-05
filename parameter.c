/*----------------------------------------------------------------------*/
/* parameter.c								*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*      written by Tim Edwards, 10/26/99    				*/
/*	revised for segmented strings, 3/8/01				*/
/*----------------------------------------------------------------------*/

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
#endif
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include "xcircuit.h"
#include "menudep.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Externally declared global variables					*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#endif
extern Globaldata xobjs;
extern XCWindowData *areawin;
#ifndef TCL_WRAPPER
extern Widget     menuwidgets[];
#endif
extern char _STR[150];

/*----------------------------------------------------------------------*/
/* The following u_char array matches parameterization types to element	*/
/* types which are able to accept the given parameterization.		*/
/*----------------------------------------------------------------------*/

u_char param_select[] = {
   ALL_TYPES,					/* P_NUMERIC */
   LABEL,					/* P_SUBSTRING */
   POLYGON | SPLINE | LABEL | OBJINST | ARC,	/* P_POSITION_X */
   POLYGON | SPLINE | LABEL | OBJINST | ARC,	/* P_POSITION_Y */
   POLYGON | SPLINE | ARC | PATH,		/* P_STYLE */
   LABEL,					/* P_ANCHOR */
   ARC,						/* P_ANGLE1 */
   ARC,						/* P_ANGLE2 */
   ARC,						/* P_RADIUS */
   ARC,						/* P_MINOR_AXIS */
   LABEL | OBJINST,				/* P_ROTATION */
   LABEL | OBJINST,				/* P_SCALE */
   POLYGON | SPLINE | ARC | PATH,		/* P_LINEWIDTH */
   ALL_TYPES, 					/* P_COLOR */
   ALL_TYPES, 					/* P_EXPRESSION */
   POLYGON | SPLINE | LABEL | OBJINST | ARC 	/* P_POSITION */
};

#ifdef TCL_WRAPPER
#ifndef _MSC_VER
xcWidget *param_buttons[] = { NULL              /* (jdk) */
   /* To be done---map buttons to Tk_Windows! */
};
#else
xcWidget *param_buttons[];
#endif
#else
Widget *param_buttons[] = {
   &ParametersNumericButton,		/* P_NUMERIC */
   &ParametersSubstringButton,		/* P_SUBSTRING */
   &ParametersPositionButton,		/* P_POSITION_X */
   &ParametersPositionButton,		/* P_POSITION_Y */
   &ParametersStyleButton,		/* P_STYLE */
   &ParametersAnchoringButton,		/* P_ANCHOR */
   &ParametersStartAngleButton,		/* P_ANGLE1 */
   &ParametersEndAngleButton,		/* P_ANGLE2 */
   &ParametersRadiusButton,		/* P_RADIUS */
   &ParametersMinorAxisButton,		/* P_MINOR_AXIS */
   &ParametersRotationButton,		/* P_ROTATION */
   &ParametersScaleButton,		/* P_SCALE */
   &ParametersLinewidthButton,		/* P_LINEWIDTH */
   &ParametersColorButton,		/* P_COLOR */
   &ParametersPositionButton,		/* P_POSITION */
};
#endif

/*----------------------------------------------------------------------*/
/* Basic routines for matching parameters by key values.  Note that	*/
/* this really, really ought to be replaced by a hash table search!	*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Check for the existance of a parameter with key "key" in object	*/
/* "thisobj".   Return true if the parameter exists.			*/
/*----------------------------------------------------------------------*/

Boolean check_param(objectptr thisobj, char *key)
{
   oparamptr tops;

   for (tops = thisobj->params; tops != NULL; tops = tops->next)
      if (!strcmp(tops->key, key))
	 return TRUE;

   return FALSE;
}

/*----------------------------------------------------------------------*/
/* Create a new parameter;  allocate memory for the parameter and the	*/
/* key.									*/
/*----------------------------------------------------------------------*/

oparamptr make_new_parameter(char *key)
{
   oparamptr newops;

   newops = (oparamptr)malloc(sizeof(oparam));
   newops->next = NULL;
   newops->key = (char *)malloc(1 + strlen(key));
   strcpy(newops->key, key);
   return newops;
}

/*----------------------------------------------------------------------*/
/* Create a new element (numeric) parameter.  Fill in essential values	*/
/*----------------------------------------------------------------------*/

eparamptr make_new_eparam(char *key)
{
   eparamptr newepp;

   newepp = (eparamptr)malloc(sizeof(eparam));
   newepp->next = NULL;
   newepp->key = (char *)malloc(1 + strlen(key));
   strcpy(newepp->key, key);
   newepp->pdata.refkey = NULL;		/* equivalently, sets pointno=0 */
   newepp->flags = 0;

   return newepp;
}

/*----------------------------------------------------------------------*/
/* Determine if a parameter is indirectly referenced.  If so, return	*/
/* the parameter name.  If not, return NULL.				*/
/*----------------------------------------------------------------------*/

char *find_indirect_param(objinstptr thisinst, char *refkey)
{
   eparamptr epp;

   for (epp = thisinst->passed; epp != NULL; epp = epp->next) {
      if ((epp->flags & P_INDIRECT) && !strcmp(epp->pdata.refkey, refkey))
	 return epp->key;
   }
   return NULL;
}

/*----------------------------------------------------------------------*/
/* Find the parameter in the indicated object by key			*/
/*----------------------------------------------------------------------*/

oparamptr match_param(objectptr thisobj, char *key)
{
   oparamptr fparam;

   for (fparam = thisobj->params; fparam != NULL; fparam = fparam->next)
      if (!strcmp(fparam->key, key))
	 return fparam;

   return NULL;		/* No parameter matched the key---error condition */
}

/*----------------------------------------------------------------------*/
/* Find the parameter in the indicated instance by key.  If no such 	*/
/* instance value exists, return NULL.					*/
/*----------------------------------------------------------------------*/

oparamptr match_instance_param(objinstptr thisinst, char *key)
{
   oparamptr fparam;

   for (fparam = thisinst->params; fparam != NULL; fparam = fparam->next)
      if (!strcmp(fparam->key, key))
	 return fparam;

   return NULL;		/* No parameter matched the key---error condition */
}

/*----------------------------------------------------------------------*/
/* Find the parameter in the indicated instance by key.  If no such 	*/
/* instance value exists, return the object (default) parameter.	*/
/*									*/
/* find_param() hides instances of expression parameters, returning the	*/
/* default parameter value.  In cases where the instance value (last	*/
/* evaluated expression result) is needed, use match_instance_param().	*/
/* An exception is made when the instance param has type XC_EXPR,	*/
/* indicating that the instance redefines the entire expression.	*/
/*----------------------------------------------------------------------*/

oparamptr find_param(objinstptr thisinst, char *key)
{
   oparamptr fparam, ops;
   fparam = match_instance_param(thisinst, key);
   ops = match_param(thisinst->thisobject, key);
   if ((fparam == NULL) || ((ops->type == XC_EXPR) && (fparam->type != XC_EXPR)))
      fparam = ops;

   return fparam;
}

/*----------------------------------------------------------------------*/
/* Find the total number of parameters in an object			*/
/*----------------------------------------------------------------------*/

int get_num_params(objectptr thisobj)
{
   oparamptr fparam;
   int nparam = 0;

   for (fparam = thisobj->params; fparam != NULL; fparam = fparam->next)
      nparam++;
   return nparam;
}

/*----------------------------------------------------------------------*/
/* Remove all element parameters from an element			*/
/*----------------------------------------------------------------------*/

void free_all_eparams(genericptr thiselem)
{
   while (thiselem->passed != NULL)
      free_element_param(thiselem, thiselem->passed);
}

/*----------------------------------------------------------------------*/
/* Remove an element parameter (eparam) and free memory associated with	*/
/* the parameter key.							*/
/*----------------------------------------------------------------------*/

void free_element_param(genericptr thiselem, eparamptr thisepp)
{
   eparamptr epp, lastepp = NULL;

   for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
      if (epp == thisepp) {
	 if (lastepp != NULL)
	    lastepp->next = epp->next;
	 else
	    thiselem->passed = epp->next;

	 /* If object is an instance and the pdata record is not NULL,	*/
	 /* then this is an indirect reference with the reference key	*/
	 /* stored as an allocated string in pdata.refkey, which needs	*/
	 /* to be free'd.						*/

	 if ((epp->flags & P_INDIRECT) && (epp->pdata.refkey != NULL))
	    free(epp->pdata.refkey);

	 free(epp->key);
	 free(epp);
	 break;
      }
      lastepp = epp; 
   }
}

/*----------------------------------------------------------------------*/
/* Free an instance parameter.  Note that this routine does not free	*/
/* any strings associated with string parameters!			*/
/*									*/
/* Return a pointer to the entry before the one deleted, so we can use	*/
/* free_instance_param() inside a loop over an instance's parameters	*/
/* without having to keep track of the previous pointer position.	*/
/*----------------------------------------------------------------------*/

oparamptr free_instance_param(objinstptr thisinst, oparamptr thisparam)
{
   oparamptr ops, lastops = NULL;

   for (ops = thisinst->params; ops != NULL; ops = ops->next) {
      if (ops == thisparam) {
	 if (lastops != NULL)
	    lastops->next = ops->next;
	 else
	    thisinst->params = ops->next;
	 free(ops->key);
	 free(ops);
	 break;
      }
      lastops = ops; 
   }
   return lastops;
}

/*----------------------------------------------------------------------*/
/* Convenience function used by files.c to set a color parameter.	*/
/*----------------------------------------------------------------------*/

void std_eparam(genericptr gen, char *key)
{
   eparamptr epp;

   if (key == NULL) return;

   epp = make_new_eparam(key);
   epp->next = gen->passed;
   gen->passed = epp;
}

/*----------------------------------------------*/
/* Draw a circle at all parameter positions	*/
/*----------------------------------------------*/

void indicateparams(genericptr thiselem)
{
   int k;
   oparamptr ops;
   eparamptr epp;
   genericptr *pgen;

   if (thiselem != NULL) {
      for (epp = thiselem->passed; epp != NULL; epp = epp->next) { 
	 ops = match_param(topobject, epp->key);
	 if (ops == NULL) continue;	/* error condition */
	 if (ELEMENTTYPE(thiselem) == PATH)
	    k = epp->pdata.pathpt[1];
	 else
	    k = epp->pdata.pointno;
	 if (k < 0) k = 0;
	 switch(ops->which) {
	    case P_POSITION: case P_POSITION_X: case P_POSITION_Y:
	       switch(thiselem->type) {
		  case ARC:
	             UDrawCircle(&TOARC(&thiselem)->position, ops->which);
		     break;
		  case LABEL:
	             UDrawCircle(&TOLABEL(&thiselem)->position, ops->which);
		     break;
		  case OBJINST:
	             UDrawCircle(&TOOBJINST(&thiselem)->position, ops->which);
		     break;
		  case POLYGON:
	             UDrawCircle(TOPOLY(&thiselem)->points + k, ops->which);
		     break;
		  case SPLINE:
	             UDrawCircle(&TOSPLINE(&thiselem)->ctrl[k], ops->which);
		     break;
		  case PATH:
		     if (epp->pdata.pathpt[0] < 0)
		        pgen = ((pathptr)thiselem)->plist;
		     else
		        pgen = ((pathptr)thiselem)->plist + epp->pdata.pathpt[0];
		     if (ELEMENTTYPE(*pgen) == POLYGON)
	                 UDrawCircle(TOPOLY(pgen)->points + k, ops->which);
		     else	/* spline */
	                 UDrawCircle(&TOSPLINE(pgen)->ctrl[k], ops->which);
		     break;
	       }
	       break;
	 }
      }
   }
}

/*----------------------------------------------*/
/* Set the menu marks according to properties	*/
/* which are parameterized.  Unmanage the	*/
/* buttons which do not apply.			*/
/*						*/
/* pgen = NULL returns menu to default settings */
/*----------------------------------------------*/

#ifdef TCL_WRAPPER
void setparammarks(genericptr thiselem)
{
   /* Set GUI variables associated with the "parameter" menu.	*/

   int i;
   oparamptr ops;
   eparamptr epp;
   Boolean ptest[NUM_PARAM_TYPES];

   for (i = 0; i < NUM_PARAM_TYPES; i++)
      ptest[i] = FALSE;

   /* For each parameter declared, set the corresponding Tcl variable */
   if (thiselem != NULL) {
      for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
	 ops = match_param(topobject, epp->key);
	 if (ops == NULL) continue;	/* error condition */
	 XcInternalTagCall(xcinterp, 3, "parameter", "make",
			translateparamtype(ops->which));
	 ptest[ops->which] = TRUE;
      }
   }

   /* Now reset all of those parameters that were not set above.		*/
   /* Note that the parameters that we want to mark ignore the following types:	*/
   /* "numeric", "substring", "expression", and "position".			*/

   for (i = P_POSITION_X; i <= P_COLOR; i++)
      if (ptest[i] != TRUE)
	 XcInternalTagCall(xcinterp, 3, "parameter", "replace", translateparamtype(i));
}
#else

void setparammarks(genericptr thiselem)
{
   Widget w;
   Arg	wargs[1];
   const int rlength = sizeof(param_buttons) / sizeof(Widget *);
   int i, j, paramno;
   oparamptr ops;
   eparamptr epp;

   /* Clear all checkmarks */

   for (i = 0; i < rlength; i++) {
      XtSetArg(wargs[0], XtNsetMark, False);
      XtSetValues(*param_buttons[i], wargs, 1);
   }

   /* Check those properties which are parameterized in the element */

   if (thiselem != NULL) {
      for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
         ops = match_param(topobject, epp->key);
         w = *param_buttons[ops->which];
         XtSetArg(wargs[0], XtNsetMark, True);
         XtSetValues(w, wargs, 1);
      }
   }

   /* Unmanage widgets which do not apply to the element type */

   for (i = 0; i < rlength; i++) {
      if ((thiselem == NULL) || (param_select[i] & thiselem->type))
	 XtManageChild(*param_buttons[i]);
      else
	 XtUnmanageChild(*param_buttons[i]);
   }
}

#endif

/*------------------------------------------------------*/
/* This function is like epsubstitute() below it, but	*/
/* only substitutes those values that are expression	*/
/* types.  This allows constraints to be applied when	*/
/* editing elements.					*/
/*------------------------------------------------------*/

void exprsub(genericptr thiselem)
{
   genericptr *pgen;
   eparamptr epp;
   int k, ival;
   oparamptr dps, ops;
   float fval;
   XPoint *setpt;
   char *promoted;

   for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
      ops = match_param(topobject, epp->key);
      dps = find_param(areawin->topinstance, epp->key);
      if (dps != NULL) {
	 switch(dps->type) {
	    case XC_EXPR:
	       if ((promoted = evaluate_expr(topobject, dps, areawin->topinstance))
			== NULL) continue;
	       if (sscanf(promoted, "%g", &fval) == 1)
		  ival = (int)(fval + 0.5);
	       free(promoted);
	       if (ELEMENTTYPE(thiselem) == PATH)
		  k = epp->pdata.pathpt[1];
	       else
		  k = epp->pdata.pointno;
	       if (ops->which == P_POSITION_X) {
		  switch(thiselem->type) {
		     case PATH:
			pgen = TOPATH(&thiselem)->plist + epp->pdata.pathpt[0];
			if (ELEMENTTYPE(*pgen) == POLYGON) {
			   setpt = TOPOLY(pgen)->points + k;
			   setpt->x = ival;
			}
			else { /* spline */
			   TOSPLINE(pgen)->ctrl[k].x = ival;
			}
			break;
		     case POLYGON:
			setpt = TOPOLY(&thiselem)->points + k;
			setpt->x = ival;
			break;
		     case SPLINE:
			TOSPLINE(&thiselem)->ctrl[k].x = ival;
			break;
		  }
	       }
	       else if (ops->which == P_POSITION_Y) {
		  switch(thiselem->type) {
		     case PATH:
			pgen = TOPATH(&thiselem)->plist + epp->pdata.pathpt[0];
			if (ELEMENTTYPE(*pgen) == POLYGON) {
			   setpt = TOPOLY(pgen)->points + k;
			   setpt->y = ival;
			}
			else { /* spline */
			   TOSPLINE(pgen)->ctrl[k].y = ival;
			}
			break;
		     case POLYGON:
			setpt = TOPOLY(&thiselem)->points + k;
			setpt->y = ival;
			break;
		     case SPLINE:
			TOSPLINE(&thiselem)->ctrl[k].y = ival;
			break;
		  }
	       }
	 }
      }
   }
}

/*------------------------------------------------------*/
/* Make numerical parameter substitutions into an	*/
/* element.						*/
/*------------------------------------------------------*/

int epsubstitute(genericptr thiselem, objectptr thisobj, objinstptr pinst,
   Boolean *needrecalc)
{
   genericptr *pgen;
   eparamptr epp;
   oparamptr dps, ops;
   int retval = -1;
   int i, k, ival, diff;
   char *key;
   float fval;
   XPoint *setpt;
   char *promoted;

   for (epp = thiselem->passed; epp != NULL; epp = epp->next) {

      /* Use the parameter from the instance, if available.	*/
      /* Otherwise, revert to the type of the object.	*/
      /* Normally they will be the same.			*/

      ops = match_param(thisobj, epp->key);
      dps = (pinst != NULL) ?  find_param(pinst, epp->key) : ops;

      if (dps != NULL) {

	 /* Get integer and float values.  Promote types if necessary */

	 switch(dps->type) {
	    case XC_INT:
	       ival = dps->parameter.ivalue;
	       fval = (float)(ival);
	       break;
	    case XC_FLOAT:
	       fval = dps->parameter.fvalue;
	       ival = (int)(fval + ((fval < 0) ? -0.5 : 0.5));
	       break;
	    case XC_STRING:
	       promoted = textprint(dps->parameter.string, pinst);
	       if (sscanf(promoted, "%g", &fval) == 1)
	          ival = (int)(fval + ((fval < 0) ? -0.5 : 0.5));
	       else
		  ival = 0;
	       free(promoted);
	       break;
	    case XC_EXPR:
	       if ((promoted = evaluate_expr(thisobj, dps, pinst)) == NULL) continue;
	       if (sscanf(promoted, "%g", &fval) == 1)
	          ival = (int)(fval + ((fval < 0) ? -0.5 : 0.5));
	       free(promoted);
	       break;
	 }
      }
      else if (ops == NULL)
	 continue;

      if ((epp->flags & P_INDIRECT) && (epp->pdata.refkey != NULL)) {
	 key = epp->pdata.refkey;
	 if (key != NULL) {
	    objinstptr thisinst;
	    oparamptr refop, newop;

	    thisinst = (objinstptr)thiselem;

	    /* Sanity check: refkey must exist in object */
	    refop = match_param(thisinst->thisobject, key);
	    if (refop == NULL) {
	       Fprintf(stderr, "Error:  Reference key %s does not"
			" exist in object %s\n",
			key, thisinst->thisobject->name);
	       continue;
	    }

	    /* If an instance value already exists, remove it */
	    newop = match_instance_param(thisinst, refop->key);
	    if (newop != NULL)
	       free_instance_param(thisinst, newop);
	       
	    /* Create a new instance parameter */
	    newop = copyparameter(dps);
	    newop->next = thisinst->params;
	    thisinst->params = newop;

	    /* Change the key from the parent to the child */
	    if (strcmp(ops->key, refop->key)) {
	       free(newop->key);
	       newop->key = strdup(refop->key);
	    }
	    continue;
	 }
      }

      if (ELEMENTTYPE(thiselem) == PATH)
	 k = epp->pdata.pathpt[1];
      else
	 k = epp->pdata.pointno;
      switch(ops->which) {
	 case P_POSITION_X:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case PATH:
		  if (k < 0) {
		     pgen = TOPATH(&thiselem)->plist;
		     if (ELEMENTTYPE(*pgen) == POLYGON) {
		        setpt = TOPOLY(pgen)->points;
		        diff = ival - setpt->x;
		     }
		     else { /* spline */
		        diff = ival - TOSPLINE(pgen)->ctrl[0].x;
		     }
		     for (pgen = TOPATH(&thiselem)->plist; pgen <
				TOPATH(&thiselem)->plist +
				TOPATH(&thiselem)->parts; pgen++) {
		        if (ELEMENTTYPE(*pgen) == POLYGON) {
			   for (i = 0; i < TOPOLY(pgen)->number; i++) {
		              setpt = TOPOLY(pgen)->points + i;
		              setpt->x += diff;
			   }
		        }
		        else { /* spline */
			   for (i = 0; i < 4; i++) {
		              TOSPLINE(pgen)->ctrl[i].x += diff;
			   }
		           if (needrecalc) *needrecalc = True;
		        }
		     }
		  }
		  else {
		     pgen = TOPATH(&thiselem)->plist + epp->pdata.pathpt[0];
		     if (ELEMENTTYPE(*pgen) == POLYGON) {
		        setpt = TOPOLY(pgen)->points + k;
		        setpt->x = ival;
		     }
		     else { /* spline */
		        TOSPLINE(pgen)->ctrl[k].x = ival;
		        if (needrecalc) *needrecalc = True;
		     }
		  }
		  break;
	       case POLYGON:
		  if (k < 0) {
		     setpt = TOPOLY(&thiselem)->points;
		     diff = ival - setpt->x;
		     for (i = 0; i < TOPOLY(&thiselem)->number; i++) {
			setpt = TOPOLY(&thiselem)->points + i;
			setpt->x += diff;
		     }
		  }
		  else {
	             setpt = TOPOLY(&thiselem)->points + k;
		     setpt->x = ival;
		  }
		  break;
	       case SPLINE:
		  if (k < 0) {
		     setpt = &(TOSPLINE(&thiselem)->ctrl[0]);
		     diff = ival - setpt->x;
		     for (i = 0; i < 4; i++) {
			setpt = &(TOSPLINE(&thiselem)->ctrl[i]);
			setpt->x += diff;
		     }
		  }
		  else {
		     TOSPLINE(&thiselem)->ctrl[k].x = ival;
		  }
		  if (needrecalc) *needrecalc = True;
		  break;
	       case LABEL:
		  TOLABEL(&thiselem)->position.x = ival;
		  break;
	       case OBJINST:
		  TOOBJINST(&thiselem)->position.x = ival;
		  break;
	       case ARC:
		  TOARC(&thiselem)->position.x = ival;
		  break;
	    }
	    break;
	 case P_POSITION_Y:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case PATH:
		  if (k < 0) {
		     pgen = TOPATH(&thiselem)->plist;
		     if (ELEMENTTYPE(*pgen) == POLYGON) {
		        setpt = TOPOLY(pgen)->points;
		        diff = ival - setpt->y;
		     }
		     else { /* spline */
		        diff = ival - TOSPLINE(pgen)->ctrl[0].y;
		     }
		     for (pgen = TOPATH(&thiselem)->plist; pgen <
				TOPATH(&thiselem)->plist +
				TOPATH(&thiselem)->parts; pgen++) {
		        if (ELEMENTTYPE(*pgen) == POLYGON) {
			   for (i = 0; i < TOPOLY(pgen)->number; i++) {
		              setpt = TOPOLY(pgen)->points + i;
		              setpt->y += diff;
			   }
		        }
		        else { /* spline */
			   for (i = 0; i < 4; i++) {
		              TOSPLINE(pgen)->ctrl[i].y += diff;
			   }
		           if (needrecalc) *needrecalc = True;
		        }
		     }
		  }
		  else {
		     pgen = TOPATH(&thiselem)->plist + epp->pdata.pathpt[0];
		     if (ELEMENTTYPE(*pgen) == POLYGON) {
		        setpt = TOPOLY(pgen)->points + k;
		        setpt->y = ival;
		     }
		     else { /* spline */
		        TOSPLINE(pgen)->ctrl[k].y = ival;
		        if (needrecalc) *needrecalc = True;
		     }
		  }
		  break;
	       case POLYGON:
		  if (k < 0) {
		     setpt = TOPOLY(&thiselem)->points;
		     diff = ival - setpt->y;
		     for (i = 0; i < TOPOLY(&thiselem)->number; i++) {
			setpt = TOPOLY(&thiselem)->points + i;
			setpt->y += diff;
		     }
		  }
		  else {
	             setpt = TOPOLY(&thiselem)->points + k;
		     setpt->y = ival;
		  }
		  break;
	       case SPLINE:
		  if (k < 0) {
		     setpt = &(TOSPLINE(&thiselem)->ctrl[0]);
		     diff = ival - setpt->y;
		     for (i = 0; i < 4; i++) {
			setpt = &(TOSPLINE(&thiselem)->ctrl[i]);
			setpt->y += diff;
		     }
		  }
		  else {
		     TOSPLINE(&thiselem)->ctrl[k].y = ival;
		  }
		  if (needrecalc) *needrecalc = True;
		  break;
	       case LABEL:
		  TOLABEL(&thiselem)->position.y = ival;
		  break;
	       case OBJINST:
		  TOOBJINST(&thiselem)->position.y = ival;
		  break;
	       case ARC:
		  TOARC(&thiselem)->position.y = ival;
		  break;
	    }
	    break;
	 case P_STYLE:
	    retval = max(retval, 0);
	    switch(thiselem->type) {
	       case POLYGON:
		  TOPOLY(&thiselem)->style = ival;
		  break;
	       case SPLINE:
		  TOSPLINE(&thiselem)->style = ival;
		  break;
	       case ARC:
		  TOARC(&thiselem)->style = ival;
		  break;
	       case PATH:
		  TOPATH(&thiselem)->style = ival;
		  break;
	    }
	    break;
	 case P_ANCHOR:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case LABEL:
		  TOLABEL(&thiselem)->anchor = ival;
		  break;
	    }
	    break;
	 case P_ANGLE1:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case ARC:
		  TOARC(&thiselem)->angle1 = fval;
		  if (needrecalc) *needrecalc = True;
		  break;
	    }
	    break;
	 case P_ANGLE2:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case ARC:
		  TOARC(&thiselem)->angle1 = fval;
		  if (needrecalc) *needrecalc = True;
		  break;
	    }
	    break;
	 case P_RADIUS:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case ARC:
		  TOARC(&thiselem)->radius = ival;
		  TOARC(&thiselem)->yaxis = ival;
		  if (needrecalc) *needrecalc = True;
		  break;
	    }
	    break;
	 case P_MINOR_AXIS:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case ARC:
		  TOARC(&thiselem)->yaxis = ival;
		  if (needrecalc) *needrecalc = True;
		  break;
	    }
	    break;
	 case P_ROTATION:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case LABEL:
		  TOLABEL(&thiselem)->rotation = fval;
		  break;
	       case OBJINST:
		  TOOBJINST(&thiselem)->rotation = fval;
		  break;
	    }
	    break;
	 case P_SCALE:
	    retval = max(retval, 1);
	    switch(thiselem->type) {
	       case LABEL:
		  TOLABEL(&thiselem)->scale = fval;
		  break;
	       case OBJINST:
		  TOOBJINST(&thiselem)->scale = fval;
		  break;
	    }
	    break;
	 case P_LINEWIDTH:
	    retval = max(retval, 0);
	    switch(thiselem->type) {
	       case POLYGON:
		  TOPOLY(&thiselem)->width = fval;
		  break;
	       case SPLINE:
		  TOSPLINE(&thiselem)->width = fval;
		  break;
	       case ARC:
		  TOARC(&thiselem)->width = fval;
		  break;
	       case PATH:
		  TOPATH(&thiselem)->width = fval;
		  break;
	    }
	    break;
	 case P_COLOR:
	    retval = max(retval, 0);
	    thiselem->color = ival;
	    break;
      }
   }

   return retval;
}

/*------------------------------------------------------*/
/* Make numerical parameter substitutions into all	*/
/* elements of an object.  "thisinst" may be NULL, in	*/
/* which case all default values are used in the	*/
/* substitution.					*/ 
/*							*/
/* Return values:					*/
/*   -1 if the instance declares no parameters		*/
/*    0 if parameters do not change the instance's bbox */
/*    1 if parameters change instance's bbox		*/
/*    2 if parameters change instance's netlist		*/
/*------------------------------------------------------*/

int opsubstitute(objectptr thisobj, objinstptr pinst)
{
   genericptr *eptr, *pgen, thiselem;
   stringpart *strptr;
   int retval = -1;
   Boolean needrecalc;	/* for arcs and splines */

   /* Perform expression parameter substitutions on all labels.	*/
   /* Note that this used to be done on an immediate basis as	*/
   /* labels were parsed.  The main difference is that only one	*/
   /* expression parameter can be used per label if it is to	*/
   /* compute the result of some aspect of the label, such as	*/
   /* position; this is a tradeoff for much simplified handling	*/
   /* of expression results, like having to avoid infinite	*/
   /* recursion in an expression result.			*/

   for (eptr = thisobj->plist; eptr < thisobj->plist + thisobj->parts; eptr++)
      if ((*eptr)->type == LABEL)
	 for (strptr = (TOLABEL(eptr))->string; strptr != NULL; strptr =
			nextstringpartrecompute(strptr, pinst));

   if (thisobj->params == NULL)
      return -1;			    /* object has no parameters */

   for (eptr = thisobj->plist; eptr < thisobj->plist + thisobj->parts; eptr++) {

      needrecalc = False;
      thiselem = *eptr;
      if (thiselem->passed == NULL) continue;	/* Nothing to substitute */
      retval = epsubstitute(thiselem, thisobj, pinst, &needrecalc);

      /* substitutions into arcs and splines require that the	*/
      /* line segments be recalculated.				*/

      if (needrecalc) {
	 switch(thiselem->type) {
	    case ARC:
	       calcarc((arcptr)thiselem);
	       break;
	    case SPLINE:
	       calcspline((splineptr)thiselem);
	       break;
	    case PATH:
	       for (pgen = ((pathptr)thiselem)->plist; pgen < ((pathptr)thiselem)->plist
				+ ((pathptr)thiselem)->parts; pgen++)
		  if (ELEMENTTYPE(*pgen) == SPLINE)
		     calcspline((splineptr)*pgen);
	       break;
	 }
      }
   }
   return retval;
}

/*------------------------------------------------------*/
/* Same as above, but determines the object from the	*/
/* current page hierarchy.				*/
/*------------------------------------------------------*/

int psubstitute(objinstptr thisinst)
{
   objinstptr pinst;
   objectptr thisobj;

   pinst = (thisinst == areawin->topinstance) ? areawin->topinstance : thisinst;
   if (pinst == NULL) return -1;		/* there is no instance */
   thisobj = pinst->thisobject;

   return opsubstitute(thisobj, pinst);
}

/*----------------------------------------------*/
/* Check if an element contains a parameter.	*/
/*----------------------------------------------*/

Boolean has_param(genericptr celem)
{
   if (IS_LABEL(celem)) {
      stringpart *cstr;
      labelptr clab = (labelptr)celem;
      for (cstr = clab->string; cstr != NULL; cstr = cstr->nextpart)
	 if (cstr->type == PARAM_START)
	    return TRUE;
   }
   if (celem->passed != NULL) return TRUE;
   return FALSE;
}

/*------------------------------------------------------*/
/* Find "current working values" in the element list of	*/
/* an object, and write them into the instance's	*/
/* parameter list.					*/
/* This is just the opposite of "psubstitute()", except	*/
/* that instance values are created prior to writeback, */
/* and resolved afterward.				*/
/*------------------------------------------------------*/

void pwriteback(objinstptr thisinst)
{
   genericptr *eptr, *pgen, thiselem;
   objectptr thisobj;
   objinstptr pinst;
   eparamptr epp;
   oparamptr ops, ips;
   int k, type, *destivalptr, found;
   XPoint *setpt;
   Boolean changed, need_redraw = FALSE;
   union {
      int ival;
      float fval;
   } wtemp;

   pinst = thisinst;
   thisobj = (pinst == NULL) ? topobject : pinst->thisobject;

   /* Make sure that all instance values exist */
   if (pinst != NULL) copyparams(pinst, pinst);

   /* Because more than one element can point to the same parameter, we search	*/
   /* through each (numerical) parameter declared in the object.  If any	*/
   /* element has a different value, the parameter is changed to match.   This	*/
   /* operates on the assumption that no more than one element will change the	*/
   /* value of any one parameter on a single call to pwriteback().		*/

   for (ops = thisobj->params; ops != NULL; ops = ops->next) {
      /* handle pre-assigned numeric parameters only */
      if ((ops->which == P_SUBSTRING) || (ops->which == P_EXPRESSION) ||
		(ops->which == P_NUMERIC))
	 continue;
      found = 0;
      changed = FALSE;
      ips = (pinst != NULL) ? match_instance_param(pinst, ops->key) : NULL;
      for (eptr = thisobj->plist; eptr < thisobj->plist + thisobj->parts; eptr++) {
         thiselem = *eptr;
         if (thiselem->passed == NULL) continue;	/* Nothing to write back */
         for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
	    if (!strcmp(epp->key, ops->key)) {
	       found++;
	       if (ELEMENTTYPE(thiselem) == PATH)
		  k = epp->pdata.pathpt[1];
	       else
	          k = epp->pdata.pointno;
	       if (k < 0) k = 0;
               switch(ops->which) {
	          case P_POSITION_X:
	             switch(thiselem->type) {
	                case OBJINST:
		           wtemp.ival = TOOBJINST(eptr)->position.x;
		           break;
	                case LABEL:
		           wtemp.ival = TOLABEL(eptr)->position.x;
		           break;
	                case POLYGON:
	                   setpt = TOPOLY(eptr)->points + k;
		           wtemp.ival = setpt->x;
		           break;
	                case ARC:
		           wtemp.ival = TOARC(eptr)->position.x;
		           break;
	                case SPLINE:
		           wtemp.ival = TOSPLINE(eptr)->ctrl[k].x;
		           break;
			case PATH:
			   if (epp->pdata.pathpt[0] < 0)
			      pgen = TOPATH(eptr)->plist;
			   else
			      pgen = TOPATH(eptr)->plist + epp->pdata.pathpt[0];
			   if (ELEMENTTYPE(*pgen) == POLYGON) {
	                      setpt = TOPOLY(pgen)->points + k;
		              wtemp.ival = setpt->x;
			   }
			   else
		              wtemp.ival = TOSPLINE(pgen)->ctrl[k].x;
			   break;
	             }
	             break;
	          case P_POSITION_Y:
	             switch(thiselem->type) {
	                case OBJINST:
		           wtemp.ival = TOOBJINST(eptr)->position.y;
		           break;
	                case LABEL:
		           wtemp.ival = TOLABEL(eptr)->position.y;
		           break;
	                case POLYGON:
	                   setpt = TOPOLY(eptr)->points + k;
		           wtemp.ival = setpt->y;
		           break;
	                case ARC:
		           wtemp.ival = TOARC(eptr)->position.y;
		           break;
	                case SPLINE:
		           wtemp.ival = TOSPLINE(eptr)->ctrl[k].y;
		           break;
			case PATH:
			   if (epp->pdata.pathpt[0] < 0)
			      pgen = TOPATH(eptr)->plist;
			   else
			      pgen = TOPATH(eptr)->plist + epp->pdata.pathpt[0];
			   if (ELEMENTTYPE(*pgen) == POLYGON) {
	                      setpt = TOPOLY(pgen)->points + k;
		              wtemp.ival = setpt->y;
			   }
			   else
		              wtemp.ival = TOSPLINE(pgen)->ctrl[k].y;
			   break;
	             }
	             break;
	          case P_STYLE:
	             switch(thiselem->type) {
	                case POLYGON:
		           wtemp.ival = TOPOLY(eptr)->style;
		           break;
	                case ARC:
		           wtemp.ival = TOARC(eptr)->style;
		           break;
	                case SPLINE:
		           wtemp.ival = TOSPLINE(eptr)->style;
		           break;
	                case PATH:
		           wtemp.ival = TOPATH(eptr)->style;
		           break;
	             }
	             break;
	          case P_ANCHOR:
	             switch(thiselem->type) {
	                case LABEL:
		           wtemp.ival = TOLABEL(eptr)->anchor;
		           break;
	             }
	             break;
	          case P_ANGLE1:
	             switch(thiselem->type) {
	                case ARC:
		           wtemp.fval = TOARC(eptr)->angle1;
		           break;
	             }
	             break;
	          case P_ANGLE2:
	             switch(thiselem->type) {
	                case ARC:
		           wtemp.fval = TOARC(eptr)->angle1;
		           break;
	             }
	             break;
	          case P_RADIUS:
	             switch(thiselem->type) {
	                case ARC:
		           wtemp.ival = TOARC(eptr)->radius;
		           break;
	             }
	             break;
	          case P_MINOR_AXIS:
	             switch(thiselem->type) {
	                case ARC:
		           wtemp.ival = TOARC(eptr)->yaxis;
		           break;
	             }
	             break;
	          case P_ROTATION:
	             switch(thiselem->type) {
	                case OBJINST:
		           wtemp.fval = TOOBJINST(eptr)->rotation;
		           break;
	                case LABEL:
		           wtemp.fval = TOLABEL(eptr)->rotation;
		           break;
	             }
	             break;
	          case P_SCALE:
	             switch(thiselem->type) {
	                case OBJINST:
		           wtemp.fval = TOOBJINST(eptr)->scale;
		           break;
	                case LABEL:
		           wtemp.fval = TOLABEL(eptr)->scale;
		           break;
	             }
	             break;
	          case P_LINEWIDTH:
	             switch(thiselem->type) {
	                case POLYGON:
		           wtemp.fval = TOPOLY(eptr)->width;
		           break;
	                case ARC:
		           wtemp.fval = TOARC(eptr)->width;
		           break;
	                case SPLINE:
		           wtemp.fval = TOSPLINE(eptr)->width;
		           break;
	                case PATH:
		           wtemp.fval = TOPATH(eptr)->width;
		           break;
	             }
	             break;
		  case P_COLOR:
		     wtemp.ival = thiselem->color;
		     break;
               }
	       type = (ips != NULL) ? ips->type : ops->type;
	       if (type != XC_FLOAT && type != XC_INT) break;

	       destivalptr = (ips != NULL) ? &ips->parameter.ivalue
			: &ops->parameter.ivalue;
	       if ((!changed) && (wtemp.ival != *destivalptr)) {
		  *destivalptr = wtemp.ival;
		  changed = TRUE;
	       }
	       else if (found > 1) need_redraw = TRUE;
	       break;
	    }
	 }
      }
   }

   /* Any instance values which are identical to the default value	*/
   /* get erased (so they won't be written to the output unnecessarily) */

   if (pinst != NULL) resolveparams(pinst);

   if (need_redraw) {
      incr_changes(thisobj);
      invalidate_netlist(thisobj);
   }

   /* Because more than one element may use the same parameter,		*/
   /* pwriteback checks for cases in which a change in one element	*/
   /* precipitates a change in another.  If so, force a redraw.		*/

   if (need_redraw && (thisinst == areawin->topinstance))
      drawarea(NULL, NULL, NULL);
}

/*------------------------------------------------------*/
/* If the instance comes from the library, replace the	*/
/* default value with the instance value.		*/
/*------------------------------------------------------*/

void replaceparams(objinstptr thisinst)
{
   objectptr thisobj;
   oparamptr ops, ips;
   /* int i, nullparms = 0; (jdk) */

   thisobj = thisinst->thisobject;

   for (ops = thisobj->params; ops != NULL; ops = ops->next) {
      ips = match_instance_param(thisinst, ops->key);
      if (ips == NULL) continue;  /* this parameter is already default */

      switch(ops->type) {
	 case XC_STRING:
	    if (stringcomp(ops->parameter.string, ips->parameter.string)) {
	       freelabel(ops->parameter.string);
	       ops->parameter.string = ips->parameter.string;
	       free_instance_param(thisinst, ips);
	    }
	    break;
	 case XC_EXPR:
	    /* Expression parameters should be replaced *only* if the
	     * instance value is also an expression, and not an evaluated
	     * result.
	     */
	    if ((ips->type == XC_EXPR) &&
			strcmp(ops->parameter.expr, ips->parameter.expr)) {
	       free(ops->parameter.expr);
	       ops->parameter.expr = ips->parameter.expr;
	       free_instance_param(thisinst, ips);
	    }
	    break;
	 case XC_INT: case XC_FLOAT:
	    if (ops->parameter.ivalue != ips->parameter.ivalue) {
	       ops->parameter.ivalue = ips->parameter.ivalue;
	       free_instance_param(thisinst, ips);
	    }
	    break;
      }
   }
}

/*------------------------------------------------------*/
/* Resolve differences between the object instance	*/
/* parameters and the default parameters.  If they	*/
/* are the same for any parameter, delete that instance	*/
/* such that the instance reverts to the default value.	*/
/*------------------------------------------------------*/

void resolveparams(objinstptr thisinst)
{
   objectptr thisobj;
   liblistptr spec;
   oparamptr ops, ips;
   int i;

   /* If the instance has no parameters itself, ignore it. */
   if (thisinst == NULL || thisinst->params == NULL) return;

   /* If the object was pushed into from a library, we want to change	*/
   /* the default, not the instanced, parameter values.  However, this	*/
   /* is not true for "virtual" library objects (in the instlist)	*/

   if ((i = checklibtop()) >= 0) {
      for (spec = xobjs.userlibs[i].instlist; spec != NULL;
                spec = spec->next)
         if (spec->thisinst == thisinst)
	    break;

      if ((spec == NULL) || (spec->virtual == FALSE)) {
         /* Fprintf(stdout, "Came from library:  changing default value\n"); */
         replaceparams(thisinst);
         return;
      }
   }

   /* Parameters which are changed on a top-level page must also change	*/
   /* the default value; otherwise, the instance value shadows the page	*/
   /* object's value but the page object's value is the one written to	*/
   /* the output file.							*/

   else if (is_page(thisinst->thisobject) >= 0) {
      replaceparams(thisinst);
      return;
   }

   thisobj = thisinst->thisobject;

   for (ops = thisobj->params; ops != NULL; ops = ops->next) {
      ips = match_instance_param(thisinst, ops->key);
      if (ips == NULL) continue;  /* this parameter is already default */

      /* If type or which fields do not match, then we don't need to look */
      /* any further;  object and instance have different parameters.	  */
      if ((ips->type != ops->type) || (ips->which != ops->which)) continue;

      switch(ops->type) {
	 case XC_STRING:
	    if (!stringcomp(ops->parameter.string, ips->parameter.string)) {
	       freelabel(ips->parameter.string);
	       free_instance_param(thisinst, ips);
	    }
	    break;
	 case XC_EXPR:
	    if (!strcmp(ops->parameter.expr, ips->parameter.expr)) {
	       free(ips->parameter.expr);
	       free_instance_param(thisinst, ips);
	    }
	    break;
	 case XC_INT: case XC_FLOAT:
	    if (ops->parameter.ivalue == ips->parameter.ivalue) {
	       free_instance_param(thisinst, ips);
	    }
	    break;
      }
   }

   if (thisinst->params != NULL) {
      /* Object must recompute bounding box if any instance	*/
      /* uses a non-default parameter.				*/

      calcbboxvalues(thisinst, NULL);
   }
}

/*--------------------------------------------------------------*/
/* Return a copy of the single eparameter "cepp"		*/
/*--------------------------------------------------------------*/

eparamptr copyeparam(eparamptr cepp, genericptr thiselem)
{
   eparamptr newepp;

   newepp = make_new_eparam(cepp->key);
   if ((cepp->flags & P_INDIRECT) && (cepp->pdata.refkey != NULL))
      newepp->pdata.refkey = strdup(cepp->pdata.refkey);
   else
      newepp->pdata.pointno = cepp->pdata.pointno;  /* also covers pathpt[] */
   newepp->flags = cepp->flags;
   return newepp;
}

/*------------------------------------------------------*/
/* Copy all element parameters from source to dest	*/
/*------------------------------------------------------*/

void copyalleparams(genericptr destinst, genericptr sourceinst)
{
   eparamptr cepp, newepp;

   for (cepp = sourceinst->passed; cepp != NULL; cepp = cepp->next) {
      newepp = copyeparam(cepp, sourceinst);
      newepp->next = destinst->passed;
      destinst->passed = newepp;
   }
}

/*--------------------------------------------------------------*/
/* Return a copy of the single parameter "cops"			*/
/*--------------------------------------------------------------*/

oparamptr copyparameter(oparamptr cops)
{
   oparamptr newops;

   newops = make_new_parameter(cops->key);
   newops->type = cops->type;
   newops->which = cops->which;
   switch(cops->type) {
      case XC_STRING:
	 newops->parameter.string = stringcopy(cops->parameter.string);
	 break;
      case XC_EXPR:
	 newops->parameter.expr = strdup(cops->parameter.expr);
	 break;
      case XC_INT: case XC_FLOAT:
	 newops->parameter.ivalue = cops->parameter.ivalue;
	 break;
      default:
	 Fprintf(stderr, "Error:  bad parameter\n");
	 break;
   }
   return newops;
}

/*------------------------------------------------------*/
/* Fill any NULL instance parameters with the values	*/
/* from the calling instance, or from the instance	*/
/* object's defaults if destinst = sourceinst.		*/ 
/*							*/
/* Expression parameters get special treatment because	*/
/* the instance value may be holding the last evaluated	*/
/* expression, not an instance value of the expression.	*/
/* If so, its type will be XC_STRING or XC_FLOAT, not	*/
/* XC_EXPR.						*/
/*------------------------------------------------------*/

void copyparams(objinstptr destinst, objinstptr sourceinst)
{
   oparamptr psource, cops, newops, ips;

   if (sourceinst == NULL) return;
   if (destinst == sourceinst)
      psource = sourceinst->thisobject->params;
   else
      psource = sourceinst->params;

   for (cops = psource; cops != NULL; cops = cops->next) {
      if ((ips = match_instance_param(destinst, cops->key)) == NULL) {
         newops = copyparameter(cops);
	 newops->next = destinst->params;
	 destinst->params = newops;
      }
      else if ((cops->type == XC_EXPR) && (ips->type != XC_EXPR))
	 free_instance_param(destinst, ips);
   }
}

/*--------------------------------------------------------------*/
/* Make an unreferenced parameter expression in the object	*/
/* refobject.							*/
/* The expression may have either a numeric or string result.	*/
/* the proper "which" value is passed as an argument.		*/
/*								*/
/* Return NULL if unsuccessful, the parameter key (which may	*/
/* have been modified by checkvalidname()) otherwise.		*/
/*--------------------------------------------------------------*/

char *makeexprparam(objectptr refobject, char *key, char *value, int which)
{
   oparamptr newops;
   char *newkey, stkey[20];
   int pidx;

   /* Check against object names, which are reserved words */

   if (key == NULL) {
      strcpy(stkey, getnumericalpkey(which));
      pidx = 0;
      while (check_param(refobject, stkey)) {
         pidx++;
         sprintf(stkey, "%s%d", getnumericalpkey(which), pidx);
      }
      newkey = stkey;
   }
   else {
      /* Check parameter key for valid name syntax */

      newkey = checkvalidname(key, NULL);
      if (newkey == NULL) newkey = key;

      /* Ensure that no two parameters have the same name! */

      if (check_param(refobject, newkey)) {
         Wprintf("There is already a parameter named %s!", newkey);
         if (newkey != key) free(newkey);
         return NULL;
      }
   }

   newops = make_new_parameter(newkey);
   newops->next = refobject->params;
   refobject->params = newops;
   newops->type = XC_EXPR;		/* expression requiring evaluation */
   newops->which = which;
   newops->parameter.expr = strdup(value);
   incr_changes(refobject);
   if ((newkey != key) && (newkey != stkey)) free(newkey);

   return newops->key;
}

/*------------------------------------------------------------------*/
/* Make an unreferenced numerical parameter in the object refobject */
/*								    */
/* Return FALSE if unsuccessful, TRUE otherwise.		    */
/*------------------------------------------------------------------*/

Boolean makefloatparam(objectptr refobject, char *key, float value)
{
   oparamptr newops;
   char *newkey;

   /* Check against object names, which are reserved words */

   newkey = checkvalidname(key, NULL);
   if (newkey == NULL) newkey = key;

   /* Ensure that no two parameters have the same name! */

   if (check_param(refobject, newkey)) {
      Wprintf("There is already a parameter named %s!", newkey);
      if (newkey != key) free(newkey);
      return FALSE;
   }

   newops = make_new_parameter(key);
   newops->next = refobject->params;
   refobject->params = newops;
   newops->type = XC_FLOAT;		/* general-purpose numeric */
   newops->which = P_NUMERIC;
   newops->parameter.fvalue = value;
   incr_changes(refobject);
   if (newkey != key) free(newkey);

   return TRUE;
}

/*----------------------------------------------------------------*/
/* Make an unreferenced string parameter in the object refobject. */
/* Return FALSE if unsuccessful, TRUE otherwise.		  */
/*----------------------------------------------------------------*/

Boolean makestringparam(objectptr refobject, char *key, stringpart *strptr)
{
   oparamptr newops;
   char *newkey;

   /* Check against object names, which are reserved words */

   newkey = checkvalidname(key, NULL);
   if (newkey == NULL) newkey = key;

   /* Ensure that no two parameters have the same name! */

   if (check_param(refobject, newkey)) {
      Wprintf("There is already a parameter named %s!", newkey);
      if (newkey != key) free(newkey);
      return FALSE;
   }

   newops = make_new_parameter(newkey);
   newops->next = refobject->params;
   refobject->params = newops;
   newops->type = XC_STRING;
   newops->which = P_SUBSTRING;
   newops->parameter.string = strptr;
   incr_changes(refobject);
   if (newkey != key) free(newkey);

   return TRUE;
}

/*--------------------------------------------------------------*/
/* Return the built-in parameter key corresponding to a 	*/
/* parameter type (as defined in xcircuit.h).			*/
/*								*/
/* Numerical parameters have designated keys to avoid the	*/
/* necessity of having to specify a key, to avoid conflicts	*/
/* with PostScript predefined keys, and other good reasons.	*/
/*--------------------------------------------------------------*/

char *getnumericalpkey(u_int mode)
{
   static char *param_keys[] = {
	"p_gps", "p_str", "p_xps", "p_yps", "p_sty", "p_jst", "p_an1",
	"p_an2", "p_rad", "p_axs", "p_rot", "p_scl", "p_wid", "p_col",
	"p_bad"
   };

   if (mode < 0 || mode > 13) return param_keys[14];
   return param_keys[mode];
}

/*--------------------------------------------------------------*/
/* Make a numerical (integer or float) parameter.		*/
/* If "key" is non-NULL, then the parameter key will be set	*/
/* from this rather than from the list "param_keys".  If the	*/
/* key is an existing key with the same type as "mode", then	*/
/* the new parameter will be linked to the existing one.	*/
/*--------------------------------------------------------------*/

void makenumericalp(genericptr *gelem, u_int mode, char *key, short cycle)
{
   genericptr pgen, *pathpgen;
   oparamptr ops, newops;
   eparamptr epp;
   XPoint *pptr;
   char new_key[7], *keyptr;
   int pidx, i;
   short loccycle = cycle;

   /* Parameterized strings are handled by makeparam() */

   if (IS_LABEL(*gelem) && mode == P_SUBSTRING) {
      Fprintf(stderr, "Error: String parameter passed to makenumericalp()\n");
      return;
   }

   /* Make sure the parameter doesn't already exist.	   */

   for (epp = (*gelem)->passed; epp != NULL; epp = epp->next) {
      ops = match_param(topobject, epp->key);
      if (ops->which == (u_char)mode) {
	 if ((mode == P_POSITION_X || mode == P_POSITION_Y) &&
		((*gelem)->type == POLYGON || (*gelem)->type == SPLINE) &&
		(TOPOLY(gelem)->cycle != NULL))
	 {
	    if ((cycle < 0) || (TOPOLY(gelem)->cycle->number != cycle)) {
	       Fprintf(stderr, "Cannot duplicate a point parameter!\n");
	       return;
	    }
	 }
	 else {
	    Fprintf(stderr, "Cannot duplicate a parameter!\n");
	    return;
	 }
      }
   }

   /* Ensure that no two parameters have the same name! */

   if (key) {
      keyptr = checkvalidname(key, NULL);
      if (keyptr == NULL) keyptr = key;
   }
   else {
      strcpy(new_key, getnumericalpkey(mode));
      pidx = 0;
      while (check_param(topobject, new_key)) {
         pidx++;
         sprintf(new_key, "%s%d", getnumericalpkey(mode), pidx);
      }
      keyptr = new_key;
   }

   /* Add the parameter to the element's parameter list */

   epp = make_new_eparam(keyptr);
   epp->next = (*gelem)->passed;
   (*gelem)->passed = epp;

   /* If keyptr does not point to an existing parameter, then we need	*/
   /* to create it in the object's parameter list and set the default	*/
   /* value to the existing value of the element.			*/

   ops = match_param(topobject, keyptr);
   if (ops == NULL) {
      newops = make_new_parameter(keyptr);
      newops->next = topobject->params;
      topobject->params = newops;
      newops->type = XC_INT;		/* most commonly used value */
      newops->which = (u_char)mode;	/* what kind of parameter */
      incr_changes(topobject);
   }
   else {
      if (ops->which != (u_char)mode) {
	 free_element_param(*gelem, epp);
	 Fprintf(stderr, "Error: Attempt to link a parameter to "
			"a parameter of a different type\n");
	 goto param_done;
      }
      else if (ops->type == XC_EXPR)
	 goto param_done;

      if ((newops = match_instance_param(areawin->topinstance, keyptr)) == NULL) {
         newops = make_new_parameter(keyptr);
         newops->next = areawin->topinstance->params;
         areawin->topinstance->params = newops;
         newops->type = ops->type;
         newops->which = ops->which;
      }
      else {
         /* If the parameter exists and the instance has a non-default	*/
         /* value for it, we will not change the instance record.  If	*/
         /* the element value is different, then it will change, so we	*/
	 /* should redraw.						*/
         drawarea(NULL, NULL, NULL);
         newops = NULL;
      }
   }

   if (newops) {
      if (mode == P_COLOR)
	 newops->parameter.ivalue = (int)((*gelem)->color);

      switch((*gelem)->type) {
         case LABEL:
	    switch(mode) {
	       case P_POSITION_X:
	          newops->parameter.ivalue = (int)TOLABEL(gelem)->position.x;
	          break;
	       case P_POSITION_Y:
	          newops->parameter.ivalue = (int)TOLABEL(gelem)->position.y;
	          break;
	       case P_ANCHOR:
	          newops->parameter.ivalue = (int)TOLABEL(gelem)->anchor;
	          break;
	       case P_ROTATION:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOLABEL(gelem)->rotation;
	          break;
	       case P_SCALE:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOLABEL(gelem)->scale;
	          break;
	    }
	    break;
         case ARC:
	    switch(mode) {
	       case P_POSITION_X:
	          newops->parameter.ivalue = (int)TOARC(gelem)->position.x;
	          break;
	       case P_POSITION_Y:
	          newops->parameter.ivalue = (int)TOARC(gelem)->position.y;
	          break;
	       case P_ANGLE1:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOARC(gelem)->angle1;
	          break;
	       case P_ANGLE2:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOARC(gelem)->angle2;
	          break;
	       case P_RADIUS:
	          newops->parameter.ivalue = (int)TOARC(gelem)->radius;
	          break;
	       case P_MINOR_AXIS:
	          newops->parameter.ivalue = (int)TOARC(gelem)->yaxis;
	          break;
	       case P_STYLE:
	          newops->parameter.ivalue = (int)TOARC(gelem)->style;
	          break;
	       case P_LINEWIDTH:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOARC(gelem)->width;
	          break;
	    }
	    break;
         case OBJINST:
	    switch(mode) {
	       case P_POSITION_X:
	          newops->parameter.ivalue = (int)TOOBJINST(gelem)->position.x;
	          break;
	       case P_POSITION_Y:
	          newops->parameter.ivalue = (int)TOOBJINST(gelem)->position.y;
	          break;
	       case P_ROTATION:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOOBJINST(gelem)->rotation;
	          break;
	       case P_SCALE:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOOBJINST(gelem)->scale;
	          break;
	    }
	    break;
         case POLYGON:
	    if (loccycle == -1)
	       loccycle = (TOPOLY(gelem)->cycle != NULL) ?
			TOPOLY(gelem)->cycle->number : -1;
	    switch(mode) {
	       case P_POSITION_X:
		  if (loccycle == -1) {
	             pptr = TOPOLY(gelem)->points;
	             newops->parameter.ivalue = (int)pptr->x;
		     for (i = 0; i < TOPOLY(gelem)->number; i++) {
	                pptr = TOPOLY(gelem)->points + i;
			pptr->x -= newops->parameter.ivalue;
		     }
		  } else {
	             pptr = TOPOLY(gelem)->points + loccycle;
	             newops->parameter.ivalue = (int)pptr->x;
		  }
	          epp->pdata.pointno = loccycle;
	          break;
	       case P_POSITION_Y:
		  if (loccycle == -1) {
	             pptr = TOPOLY(gelem)->points;
	             newops->parameter.ivalue = (int)pptr->y;
		     for (i = 0; i < TOPOLY(gelem)->number; i++) {
	                pptr = TOPOLY(gelem)->points + i;
			pptr->y -= newops->parameter.ivalue;
		     }
		  } else {
	             pptr = TOPOLY(gelem)->points + loccycle;
	             newops->parameter.ivalue = (int)pptr->y;
		  }
	          epp->pdata.pointno = loccycle;
	          break;
	       case P_STYLE:
	          newops->parameter.ivalue = (int)TOPOLY(gelem)->style;
	          break;
	       case P_LINEWIDTH:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOPOLY(gelem)->width;
	          break;
	    }
	    break;
         case SPLINE:
	    if (loccycle == -1)
	       loccycle = (TOSPLINE(gelem)->cycle != NULL) ?
			TOSPLINE(gelem)->cycle->number : -1;
	    switch(mode) {
	       case P_POSITION_X:
		  if (loccycle == -1) {
	             pptr = &(TOSPLINE(gelem)->ctrl[0]);
	             newops->parameter.ivalue = (int)pptr->x;
		     for (i = 0; i < 4; i++) {
	                pptr = &(TOSPLINE(gelem)->ctrl[i]);
			pptr->x -= newops->parameter.ivalue;
		     }
		  } else {
	             pptr = TOSPLINE(gelem)->ctrl + loccycle;
	             newops->parameter.ivalue = (int)pptr->x;
		  }
	          epp->pdata.pointno = loccycle;
	          break;
	       case P_POSITION_Y:
		  if (loccycle == -1) {
	             pptr = &(TOSPLINE(gelem)->ctrl[0]);
	             newops->parameter.ivalue = (int)pptr->y;
		     for (i = 0; i < 4; i++) {
	                pptr = &(TOSPLINE(gelem)->ctrl[i]);
			pptr->y -= newops->parameter.ivalue;
		     }
		  } else {
	             pptr = TOSPLINE(gelem)->ctrl + loccycle;
	             newops->parameter.ivalue = (int)pptr->y;
		  }
	          epp->pdata.pointno = loccycle;
	          break;
	       case P_STYLE:
	          newops->parameter.ivalue = (int)TOSPLINE(gelem)->style;
	          break;
	       case P_LINEWIDTH:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOSPLINE(gelem)->width;
	          break;
	    }
	    break;
         case PATH:
	    if (loccycle == -1 && (mode == P_POSITION_X || mode == P_POSITION_Y)) {
	       pgen = getsubpart(TOPATH(gelem), &pidx);
	       if (ELEMENTTYPE(pgen) == POLYGON)
	          loccycle = (((polyptr)pgen)->cycle != NULL) ?
			((polyptr)pgen)->cycle->number : -1;
	       else
	          loccycle = (((splineptr)pgen)->cycle != NULL) ?
			((splineptr)pgen)->cycle->number : -1;
	    }
	    else {
	       Fprintf(stderr, "Can't parameterize a path point from "
			"the command line.\n");
	       break;
	    }
	    switch(mode) {
	       case P_STYLE:
	          newops->parameter.ivalue = (int)TOPATH(gelem)->style;
	          break;
	       case P_LINEWIDTH:
	          newops->type = XC_FLOAT;
	          newops->parameter.fvalue = TOPATH(gelem)->width;
	          break;
	       case P_POSITION_X:
		  newops->type = XC_INT;
		  if (loccycle == -1) {
		     pathpgen = TOPATH(gelem)->plist;
		     if (ELEMENTTYPE(*pathpgen) == POLYGON) {
	                pptr = TOPOLY(pathpgen)->points;
	                newops->parameter.ivalue = (int)pptr->x;
		     }
		     else {
	                pptr = &(TOSPLINE(pathpgen)->ctrl[0]);
	                newops->parameter.ivalue = (int)pptr->x;
		     }
		     for (pathpgen = TOPATH(gelem)->plist; pathpgen <
				TOPATH(gelem)->plist + TOPATH(gelem)->parts;
				pathpgen++) {
			if (ELEMENTTYPE(*pathpgen) == POLYGON) {
			   for (i = 0; i < TOPOLY(pathpgen)->number; i++) {
	                      pptr = TOPOLY(pathpgen)->points + i;
			      pptr->x -= newops->parameter.ivalue;
			   }
		 	}
			else {
			   for (i = 0; i < 4; i++) {
	                      pptr = &(TOSPLINE(pathpgen)->ctrl[i]);
			      pptr->x -= newops->parameter.ivalue;
			   }
			}
		     }	
		  }
		  else {
		     if (ELEMENTTYPE(pgen) == POLYGON)
		        newops->parameter.ivalue = ((polyptr)pgen)->points[loccycle].x;
		     else
		        newops->parameter.ivalue = ((splineptr)pgen)->ctrl[loccycle].x;
	             epp->pdata.pathpt[1] = loccycle;
	             epp->pdata.pathpt[0] = pidx;
		  }
		  break;
	       case P_POSITION_Y:
		  newops->type = XC_INT;
		  if (loccycle == -1) {
		     pathpgen = TOPATH(gelem)->plist;
		     if (ELEMENTTYPE(*pathpgen) == POLYGON) {
	                pptr = TOPOLY(pathpgen)->points;
	                newops->parameter.ivalue = (int)pptr->y;
		     }
		     else {
	                pptr = &(TOSPLINE(pathpgen)->ctrl[0]);
	                newops->parameter.ivalue = (int)pptr->y;
		     }
		     for (pathpgen = TOPATH(gelem)->plist; pathpgen <
				TOPATH(gelem)->plist + TOPATH(gelem)->parts;
				pathpgen++) {
			if (ELEMENTTYPE(*pathpgen) == POLYGON) {
			   for (i = 0; i < TOPOLY(pathpgen)->number; i++) {
	                      pptr = TOPOLY(pathpgen)->points + i;
			      pptr->y -= newops->parameter.ivalue;
			   }
		 	}
			else {
			   for (i = 0; i < 4; i++) {
	                      pptr = &(TOSPLINE(pathpgen)->ctrl[i]);
			      pptr->y -= newops->parameter.ivalue;
			   }
			}
		     }	
		  }
		  else {
		     if (ELEMENTTYPE(pgen) == POLYGON)
		        newops->parameter.ivalue = ((polyptr)pgen)->points[loccycle].y;
		     else
		        newops->parameter.ivalue = ((splineptr)pgen)->ctrl[loccycle].y;
	             epp->pdata.pathpt[1] = loccycle;
	             epp->pdata.pathpt[0] = pidx;
		  }
		  break;
	    }
	    break;
      }
   }

param_done:
   if ((keyptr != new_key) && (keyptr != key)) free(keyptr);
}

/*--------------------------------------------------------------*/
/* Remove a numerical (integer or float) parameter.  Remove by	*/
/* type, rather than key.  There may be several keys associated	*/
/* with a particular type, so we want to remove all of them.	*/
/*--------------------------------------------------------------*/

void removenumericalp(genericptr *gelem, u_int mode)
{
   genericptr *pgen;
   eparamptr epp;
   oparamptr ops;
   char *key;
   Boolean done = False, is_last = True;
   
   /* Parameterized strings are handled by makeparam() */
   if (mode == P_SUBSTRING) {
      Fprintf(stderr, "Error: Unmakenumericalp called on a string parameter.\n");
      return;
   }

   /* Avoid referencing the object by only looking at the element.	*/
   /* But, avoid dereferencing the pointer!				*/

   while (!done) {
      key = NULL;
      done = True;
      for (epp = (*gelem)->passed; epp != NULL; epp = epp->next) {
	 ops = match_param(topobject, epp->key);
	 if (ops == NULL) break;	/* Error---no such parameter */
         else if (ops->which == (u_char)mode) {
	    key = ops->key;
	    free_element_param(*gelem, epp);

	    /* check for any other references to the parameter.  If there */
	    /* are none, remove all instance records of the eparam, then  */
	    /* remove the parameter itself from the object.		  */

	    for (pgen = topobject->plist; pgen < topobject->plist
			+ topobject->parts; pgen++) {
	       if (*pgen == *gelem) continue;
	       for (epp = (*pgen)->passed; epp != NULL; epp = epp->next) {
		  if (!strcmp(epp->key, key)) {
		     is_last = False;
		     break;
		  }
	       }
	       if (!is_last) break;
	    }
	    if (is_last)
	       free_object_param(topobject, ops);

	    done = False;
	    break;
	 }
      }
   }
}

#ifndef TCL_WRAPPER

/*--------------------------------------------------------------*/
/* Insert an existing parameter into a string.			*/
/* This code needs to be replaced in the non-Tcl version with	*/
/* a new pop-up window using a callback to labeltext().		*/
/*								*/
/* This routine has been replaced in the Tcl version with a	*/
/* callback to command "label insert parameter" from the 	*/
/* parameter-select pop-up window.				*/
/*--------------------------------------------------------------*/

void insertparam()
{
   labelptr tlab;
   oparamptr ops;
   int result, nparms;
   char *selparm;
   char *newstr, *sptr;
   char *sstart = (char *)malloc(1024);
   oparamptr chosen_ops=NULL;

   /* Don't allow nested parameters */

   tlab = TOLABEL(EDITPART);
   if (paramcross(topobject, tlab)) {
      Wprintf("Parameters cannot be nested!");
      return;
   }

   nparms = 0;
   strcpy(sstart, "Choose: ");
   sptr = sstart + 8;
   for (ops = topobject->params; ops != NULL; ops = ops->next) {
      if (ops->type == XC_STRING) {
	 chosen_ops = ops;
	 nparms++;
	 if (nparms != 1) {
	    strcat(sptr, ", ");
	    sptr += 2;
	 }
	 newstr = stringprint(ops->parameter.string, NULL);
	 sprintf(sptr, "%d = %s = <%s", nparms, ops->key, newstr);
	 free(newstr);
	 newstr = NULL;
         sptr += strlen(sptr);
      }
   }

   /* If only one parameter, then automatically use it.  Otherwise,  */
   /* prompt for which parameter to use.                             */

   if (nparms > 1) {
      int i=0, select_int;
      chosen_ops = NULL;
      Wprintf("%s", sstart);
      select_int = getkeynum();
      for (ops = topobject->params; ops != NULL; ops = ops->next) {
	 if (ops->type == XC_STRING) {
	    if (i==select_int) chosen_ops = ops;
	    i++;
	 }
      }
   }

   free(sstart);
   ops = chosen_ops;
   if (ops != NULL) selparm = ops->key;

   if (ops != NULL)
      labeltext(PARAM_START, selparm);
   else
      Wprintf("No such parameter.");
}

#endif

/*--------------------------------------------------------------*/
/* Parameterize a label string.					*/
/*--------------------------------------------------------------*/

void makeparam(labelptr thislabel, char *key)
{
   oparamptr newops;
   stringpart *begpart, *endpart;
   char *newkey;

   /* Ensure that no two parameters have the same name! */

   if (check_param(topobject, key)) {
      Wprintf("There is already a parameter named %s!", key);
      areawin->textend = 0;
      return;
   }

   /* make sure this does not overlap another parameter */

   if (paramcross(topobject, thislabel)) {
      Wprintf("Parameters cannot be nested!");
      areawin->textend = 0;
      return;
   }

   /* Check parameter for valid name syntax */

   newkey = checkvalidname(key, NULL);
   if (newkey == NULL) newkey = key;

   /* First, place PARAM_START and PARAM_END structures at the	*/
   /* intended parameter boundaries				*/

   if (areawin->textend > 0 && areawin->textend < areawin->textpos) {
      /* partial string */
      splitstring(areawin->textend, &thislabel->string, areawin->topinstance);
      splitstring(areawin->textpos, &thislabel->string, areawin->topinstance);

      /* Because "splitstring" changes all the pointers, find the    */
      /* stringpart structures at textend and textpos positions.     */

      begpart = findstringpart(areawin->textend, NULL, thislabel->string,
		areawin->topinstance);
      endpart = findstringpart(areawin->textpos, NULL, thislabel->string,
		areawin->topinstance);

      /* Make the new segments for PARAM_START and PARAM_END.	*/

      begpart = makesegment(&thislabel->string, begpart);
      endpart = makesegment(&thislabel->string, endpart);
   }
   else {				/* full string */
      /* Don't include the first font designator as part of the		*/
      /* parameter or else havoc ensues.				*/
      if (thislabel->string->type == FONT_NAME && thislabel->string->nextpart
		!= NULL) {
         makesegment(&thislabel->string, thislabel->string->nextpart);
         begpart = thislabel->string->nextpart;
      }
      else {
         makesegment(&thislabel->string, thislabel->string);
         begpart = thislabel->string;
      }
      endpart = makesegment(&thislabel->string, NULL);
   }
   begpart->type = PARAM_START;
   begpart->data.string = (char *)malloc(1 + strlen(newkey));
   strcpy(begpart->data.string, newkey);
   endpart->type = PARAM_END;
   endpart->data.string = (u_char *)NULL;

   /* Now move the sections of string to the object parameter */

   newops = make_new_parameter(newkey);
   newops->next = topobject->params;
   topobject->params = newops;
   newops->type = XC_STRING;
   newops->which = P_SUBSTRING;
   newops->parameter.string = begpart->nextpart;
   begpart->nextpart = endpart->nextpart;
   endpart->nextpart = NULL;

   areawin->textend = 0;
   incr_changes(topobject);
   if (newkey != key) free(newkey);
}

/*--------------------------------------------------------------*/
/* Destroy the selected parameter in the indicated instance 	*/
/*--------------------------------------------------------------*/

void destroyinst(objinstptr tinst, objectptr refobj, char *key)
{
   oparamptr ops;
   /* short k; (jdk) */

   if (tinst->thisobject == refobj) {
      ops = match_instance_param(tinst, key);
      if (ops != NULL) {
	 if (ops->type == XC_STRING)
	    freelabel(ops->parameter.string);
	 else if (ops->type == XC_EXPR)
	    free(ops->parameter.expr);
	 free_instance_param(tinst, ops);
      }
   }
}
	
/*--------------------------------------------------------------*/
/* Search and destroy the selected parameter in all instances 	*/
/* of the specified object.					*/
/*--------------------------------------------------------------*/

void searchinst(objectptr topobj, objectptr refobj, char *key)
{
   objinstptr tinst;
   genericptr *pgen;

   if (topobj == NULL) return;

   for (pgen = topobj->plist; pgen < topobj->plist + topobj->parts; pgen++) {
      if (IS_OBJINST(*pgen)) {
	 tinst = TOOBJINST(pgen);
	 destroyinst(tinst, refobj, key);
      }
   }
}

/*--------------------------------------------------------------*/
/* Destroy the object parameter with key "key" in the object	*/
/* "thisobj".  This requires first tracking down and removing	*/
/* all instances of the parameter which may exist anywhere in	*/
/* the database.						*/
/*--------------------------------------------------------------*/

void free_object_param(objectptr thisobj, oparamptr thisparam)
{
   int k, j, l = -1;
   liblistptr spec;
   oparamptr ops, lastops = NULL;
   genericptr *pgen;
   char *key = thisparam->key;

   /* Find all instances of this object and remove any parameter */
   /* substitutions which may have been made.		         */

   for (k = 0; k < xobjs.pages; k++) {
      if (xobjs.pagelist[k]->pageinst != NULL)
         searchinst(xobjs.pagelist[k]->pageinst->thisobject, thisobj, key);
   }
   for (j = 0; j < xobjs.numlibs; j++) {
      for (k = 0; k < xobjs.userlibs[j].number; k++) {
	 if (*(xobjs.userlibs[j].library + k) == thisobj)
	    l = j;
	 else
            searchinst(*(xobjs.userlibs[j].library + k), thisobj, key);
      }
   }

   /* Ensure that this parameter is not referred to in the undo records */
   /* We could be kinder and gentler to the undo record here. . . */
   flush_undo_stack();

   /* Also check through all instances on the library page */
   if (l >= 0)
      for (spec = xobjs.userlibs[l].instlist; spec != NULL; spec = spec->next) 
         destroyinst(spec->thisinst, thisobj, key);

   /* Remove the parameter from any labels that it might occur in */

   for (pgen = thisobj->plist; pgen < thisobj->plist + thisobj->parts; pgen++) {
      if (IS_LABEL(*pgen)) {
	 Boolean pending = TRUE;
	 stringpart *strptr;
	 labelptr plab = TOLABEL(pgen);

	 while (pending) {
	    pending = FALSE;
	    for (strptr = plab->string; strptr != NULL; strptr = strptr->nextpart) {
	       if (strptr->type == PARAM_START) {
	          if (!strcmp(strptr->data.string, key)) {
		     unmakeparam(plab, NULL, strptr);
		     pending = TRUE;
		     break;
	          }
	       }
	    }
	 }
      }
   }

   /* Remove the parameter from the object itself, tidying up	*/
   /* the linked list after it.					*/

   for (ops = thisobj->params; ops != NULL; ops = ops->next) {
      if (ops == thisparam) {
	 if (lastops != NULL)
	    lastops->next = ops->next;
	 else
	    thisobj->params = ops->next;
	 free(ops->key);
	 free(ops);
	 break;
      }
      lastops = ops; 
   }
   
   incr_changes(thisobj);
}

/*--------------------------------------------------------------*/
/* Check if this string contains a parameter			*/
/*--------------------------------------------------------------*/

stringpart *searchparam(stringpart *tstr)
{
   stringpart *rval = tstr;
   for (rval = tstr; rval != NULL; rval = rval->nextpart)
      if (rval->type == PARAM_START)
	 break;
   return rval;
}

/*--------------------------------------------------------------*/
/* Remove parameterization from a label string or substring.	*/
/*--------------------------------------------------------------*/

void unmakeparam(labelptr thislabel, objinstptr thisinst, stringpart *thispart)
{
   oparamptr ops;
   oparamptr testop;
   stringpart *strptr, *lastpart, *endpart, *newstr, *subs;
   char *key;
   
   /* make sure there is a parameter here */

   if (thispart->type != PARAM_START) {
      Wprintf("There is no parameter here.");
      return;
   }
   key = thispart->data.string;

   /* Unparameterizing can cause a change in the string */
   undrawtext(thislabel);

   /* Methodology change 7/20/06:  Remove only the instance of the	*/
   /* parameter.  The parameter itself will be deleted by a different	*/
   /* method, using free_object_param().				*/

   ops = (thisinst != NULL) ? match_instance_param(thisinst, key) :
		match_param(topobject, key);

   if (ops == NULL) ops = match_param(topobject, key);

   if (ops == NULL) return;	/* Report error? */

   /* Copy the default parameter into the place we are unparameterizing */
   /* Promote first to a string type if necessary */

   if (ops->type == XC_STRING) {
      subs = ops->parameter.string;
      newstr = NULL;
      newstr = stringcopy(subs);

      /* Delete the "PARAM_END" off of the copied string and link it 	*/
      /* into the existing string.					*/
      /* (NOTE:  If parameter is an empty string, there may be nothing	*/
      /* before PARAM_END. . .)						*/

      if (newstr->type != PARAM_END) {
         for (endpart = newstr; endpart->nextpart->type != PARAM_END;
		endpart = endpart->nextpart);
         free(endpart->nextpart);
         endpart->nextpart = thispart->nextpart;
      }
      else {
         endpart = newstr;
         newstr = newstr->nextpart;
         free(endpart);
         endpart = NULL;
      }

      /* Remove dangling link from instance parameter */
      /* (If this was a global parameter, it will have no effect) */
      for (strptr = ops->parameter.string; strptr->type != PARAM_END;
		strptr = strptr->nextpart);
      strptr->nextpart = NULL;
   }
   else {
      /* This should not happen */
      Fprintf(stderr, "Error:  String contains non-string parameter!\n");
      redrawtext(thislabel);
      return;
   }

   lastpart = NULL;
   for (strptr = thislabel->string; strptr != NULL && strptr != thispart;
		strptr = strptr->nextpart) {
      lastpart = strptr;
   }
   if (lastpart == NULL)
      thislabel->string = newstr;
   else
      lastpart->nextpart = newstr;
   free(strptr);

   /* Merge strings at boundaries, if possible. */
   if (endpart) mergestring(endpart);
   mergestring(lastpart);

   redrawtext(thislabel);
}

/*----------------------------------------------------------------------*/
/* Wrapper for unmakeparam().  Remove a parameterized substring from a	*/
/* label, or remove a numeric parameter from an element.		*/
/*									*/
/* NOTE:  This routine should not combine the instance-only string	*/
/* parameter removal and the numeric parameter deletion, which is	*/
/* fundamentally different in nature.					*/
/*----------------------------------------------------------------------*/

void unparameterize(int mode)
{
   short *fselect, ptype;
   int locpos;
   stringpart *strptr, *tmpptr, *lastptr;
   labelptr settext;

   if (mode >= 0) {
      ptype = (short)param_select[mode];
      if (!checkselect(ptype)) select_element(ptype);
      if (!checkselect(ptype)) return;
   }
   else
      ptype = ALL_TYPES;

   // NOTE:  Need a different method for interactive edit;  remove only the
   // parameter under the cursor.

   if ((areawin->selects == 1) && (mode == P_SUBSTRING) && areawin->textend > 0
		&& areawin->textend < areawin->textpos) {
      if (SELECTTYPE(areawin->selectlist) != LABEL) return;	 /* Not a label */
      settext = SELTOLABEL(areawin->selectlist);
      strptr = findstringpart(areawin->textend, &locpos, settext->string,
		areawin->topinstance);
      while (strptr != NULL && strptr->type != PARAM_END)
	 strptr = strptr->nextpart;
      if (strptr == NULL) return;	/* No parameters */
      tmpptr = settext->string;
      lastptr = NULL;

      /* Search for parameter boundary, in case selection doesn't include */
      /* the whole parameter or the parameter start marker.		  */

      for (tmpptr = settext->string; tmpptr != NULL && tmpptr != strptr;
		tmpptr = nextstringpart(tmpptr, areawin->topinstance))
	 if (tmpptr->type == PARAM_START) lastptr = tmpptr;
      /* Finish search, unlinking any parameter we might be inside */
      for (; tmpptr != NULL; tmpptr = nextstringpart(tmpptr, areawin->topinstance));

      if (lastptr != NULL) unmakeparam(settext, areawin->topinstance, lastptr);
   }
   else {
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
            areawin->selects; fselect++) {
         if ((mode == P_SUBSTRING) && SELECTTYPE(fselect) == LABEL) {
	    u_char found;

            settext = SELTOLABEL(fselect);

	    // Remove all parameters from the string.  
	    found = 1;
	    while (found == (u_char)1) {
	       found = (u_char)0;
               strptr = settext->string;
	       while (strptr != NULL) {
	          if (strptr->type == PARAM_START) {
		     unmakeparam(settext, areawin->topinstance, strptr);
		     found = (u_char)1;
		     break;
		  }
	          strptr = strptr->nextpart;
	       }
	    }
	 }
	 else if (mode == P_POSITION) {
	    removenumericalp(topobject->plist + (*fselect), P_POSITION_X);
	    removenumericalp(topobject->plist + (*fselect), P_POSITION_Y);
	 }
	 else
	    removenumericalp(topobject->plist + (*fselect), mode);
      }
      setparammarks(NULL);
   }
}

/*--------------------------------------------------------------*/
/* Wrapper for makeparam()					*/
/*--------------------------------------------------------------*/

void parameterize(int mode, char *key, short cycle)
{
   short *fselect, ptype;
   labelptr settext;
   Boolean preselected;

   preselected = (areawin->selects > 0) ? TRUE : FALSE;
   if (mode >= 0) {
      ptype = (short)param_select[mode];
      if (!checkselect(ptype)) select_element(ptype);
      if (!checkselect(ptype)) return;
   }
   else
      ptype = ALL_TYPES;

   for (fselect = areawin->selectlist; fselect < areawin->selectlist +
            areawin->selects; fselect++) {
      if ((mode == P_SUBSTRING) && (areawin->selects == 1) &&
		(SELECTTYPE(fselect) == LABEL)) {
         settext = SELTOLABEL(fselect);
         makeparam(settext, key);
      }
      else if (mode == P_POSITION) {
	 makenumericalp(topobject->plist + (*fselect), P_POSITION_X, key, cycle);
	 makenumericalp(topobject->plist + (*fselect), P_POSITION_Y, key, cycle);
      }
      else
	 makenumericalp(topobject->plist + (*fselect), mode, key, cycle);
   }
   if (!preselected) unselect_all();
   setparammarks(NULL);
}

/*----------------------------------------------------------------------*/
/* Looks for a parameter overlapping the textend <--> textpos space.	*/
/* Returns True if there is a parameter in this space.			*/
/*----------------------------------------------------------------------*/

Boolean paramcross(objectptr tobj, labelptr tlab)
{
   stringpart *firstptr, *lastptr;
   int locpos;

   lastptr = findstringpart(areawin->textpos, &locpos, tlab->string,
			areawin->topinstance);

   /* This text position can't be inside another parameter */
   for (firstptr = lastptr; firstptr != NULL; firstptr = firstptr->nextpart)
      if (firstptr->type == PARAM_END) return True;

   /* The area between textend and textpos cannot contain a parameter */
   if (areawin->textend > 0)
      for (firstptr = findstringpart(areawin->textend, &locpos, tlab->string,
		areawin->topinstance); firstptr != lastptr;
		firstptr = firstptr->nextpart)
         if (firstptr->type == PARAM_START || firstptr->type == PARAM_END)
	    return True;

   return False;
}

/*----------------------------------------------------------------------*/
/* Check whether this page object was entered via a library page	*/
/*----------------------------------------------------------------------*/

int checklibtop()
{
   int i;
   pushlistptr thispush;

   for (thispush = areawin->stack; thispush != NULL; thispush = thispush->next)
      if ((i = is_library(thispush->thisinst->thisobject)) >= 0)
	 return i;

   return -1;
}

/*----------------------------------------------------------------------*/
/* Remove all parameters from an object	instance			*/
/* (Reverts all parameters to default value)				*/
/*----------------------------------------------------------------------*/

void removeinstparams(objinstptr thisinst)
{
   oparamptr ops;

   while (thisinst->params != NULL) {
      ops = thisinst->params;
      thisinst->params = ops->next;
      free(ops->key);
      if (ops->type == XC_STRING)
	 freelabel(ops->parameter.string);
      else if (ops->type == XC_EXPR)
	 free(ops->parameter.expr);
      free(ops);
   }
}

/*----------------------------------------------------------------------*/
/* Remove all parameters from an object.				*/
/*----------------------------------------------------------------------*/

void removeparams(objectptr thisobj)
{
   oparamptr ops;

   while (thisobj->params != NULL) {
      ops = thisobj->params;
      thisobj->params = ops->next;
      free(ops->key);
      if (ops->type == XC_STRING)
	 freelabel(ops->parameter.string);
      else if (ops->type == XC_EXPR)
	 free(ops->parameter.expr);
      free(ops);
   }
   thisobj->params = NULL;
}

/*-----------------------------------------------------------------------*/
