/*--------------------------------------------------------------*/
/* keybindings.c:  List of key bindings				*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University	*/
/*--------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*      written by Tim Edwards, 2/27/01                                 */
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>	 /* for tolower(), toupper() */
#include <math.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#if !defined(XC_WIN32) || defined(TCL_WRAPPER)
#define  XK_MISCELLANY
#define  XK_LATIN1
#define  XK_XKB_KEYS
#include <X11/keysymdef.h>
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER 
#include <tk.h>
#endif

#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototypes							*/
/*----------------------------------------------------------------------*/

#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Global variables							*/
/*----------------------------------------------------------------------*/

keybinding *keylist = NULL;

extern Display *dpy;
extern char  _STR[150], _STR2[250];
extern int pressmode;
extern XCWindowData *areawin;

/*----------------------------------------------------------------------*/
/* Key modifiers (convenience definitions)				*/
/* Use Mod5Mask for a "hold" ("press") definition.			*/
/* Why Mod5Mask?  Mod2Mask is used by fvwm2 for some obscure purpose,	*/
/* so I went to the other end (Mod5Mask is just below Button1Mask).	*/
/* Thanks to John Heil for reporting the problem and confirming the	*/
/* Mod5Mask solution.							*/
/*----------------------------------------------------------------------*/

#define ALT (Mod1Mask << 16)
#define CTRL (ControlMask << 16)
#define CAPSLOCK (LockMask << 16)
#define SHIFT (ShiftMask << 16)
#define BUTTON1 (Button1Mask << 16)
#define BUTTON2 (Button2Mask << 16)
#define BUTTON3 (Button3Mask << 16)
#define BUTTON4 (Button4Mask << 16)
#define BUTTON5 (Button5Mask << 16)
#define HOLD (Mod4Mask << 16)

#define ALL_WINDOWS (xcWidget)NULL

/*--------------------------------------------------------------*/
/* This list declares all the functions which can be bound to   */
/* keys.  It must match the order of the enumeration listed in	*/
/* xcircuit.h!							*/
/*--------------------------------------------------------------*/

static char *function_names[NUM_FUNCTIONS + 1] = {
   "Page", "Anchor", "Superscript", "Subscript", "Normalscript",
   "Nextfont", "Boldfont", "Italicfont", "Normalfont", "Underline",
   "Overline", "ISO Encoding", "Halfspace", "Quarterspace",
   "Special", "Tab Stop", "Tab Forward", "Tab Backward",
   "Text Return", "Text Delete", "Text Right", "Text Left",
   "Text Up", "Text Down", "Text Split", 
   "Text Home", "Text End", "Linebreak", "Parameter",
   "Parameterize Point", "Change Style", "Delete Point", "Insert Point",
   "Append Point", "Next Point", "Attach", "Next Library", "Library Directory",
   "Library Move", "Library Copy", "Library Edit", "Library Delete",
   "Library Duplicate", "Library Hide", "Library Virtual Copy",
   "Page Directory", "Library Pop", "Virtual Copy",
   "Help", "Redraw", "View", "Zoom In", "Zoom Out", "Pan",
   "Double Snap", "Halve Snap", "Write", "Rotate", "Flip X",
   "Flip Y", "Snap", "Snap To",
   "Pop", "Push", "Delete", "Select", "Box", "Arc", "Text",
   "Exchange", "Copy", "Move", "Join", "Unjoin", "Spline", "Edit",
   "Undo", "Redo", "Select Save", "Unselect", "Dashed", "Dotted",
   "Solid", "Prompt", "Dot", "Wire", "Cancel", "Nothing", "Exit",
   "Netlist", "Swap", "Pin Label", "Pin Global", "Info Label", "Graphic",
   "Select Box", "Connectivity", "Continue Element", "Finish Element",
   "Continue Copy", "Finish Copy", "Finish", "Cancel Last", "Sim",
   "SPICE", "PCB", "SPICE Flat", "Rescale", "Reorder", "Color",
   "Margin Stop", "Text Delete Param",
   NULL			/* sentinel */
};

/*--------------------------------------------------------------*/
/* Return TRUE if the indicated key (keysym + bit-shifted state)*/
/* is bound to some function.					*/
/*--------------------------------------------------------------*/

Boolean ismacro(xcWidget window, int keywstate)
{
   keybinding *ksearch;

   for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding)
      if (ksearch->window == ALL_WINDOWS || ksearch->window == window)
         if (keywstate == ksearch->keywstate)
	    return True;

   return False;
}

/*--------------------------------------------------------------*/
/* Return the first key binding for the indicated function	*/
/*--------------------------------------------------------------*/

int firstbinding(xcWidget window, int function)
{
   keybinding *ksearch;
   int keywstate = -1;

   for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding) {
      if (ksearch->function == function) {
         if (ksearch->window == window)
	    return ksearch->keywstate;
         else if (ksearch->window == ALL_WINDOWS)
	    keywstate = ksearch->keywstate;
      }
   }
   return keywstate;
}

/*--------------------------------------------------------------*/
/* Find the first function bound to the indicated key that is	*/
/* compatible with the current state (eventmode).  Window-	*/
/* specific bindings shadow ALL_WINDOWS bindings.  Return the	*/
/* function number if found, or -1 if no compatible functions	*/
/* are bound to the key state.					*/
/*--------------------------------------------------------------*/

int boundfunction(xcWidget window, int keywstate, short *retnum)
{
   keybinding *ksearch;
   int tempfunc = -1;

   for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding) {
      if (keywstate == ksearch->keywstate) {
	 if (compatible_function(ksearch->function)) {
	    if (ksearch->window == window) {
	       if (retnum != NULL) *retnum = (ksearch->value);
	       return ksearch->function;
	    }
	    else if (ksearch->window == ALL_WINDOWS) {
	       if (retnum != NULL) *retnum = (ksearch->value);
	       tempfunc = ksearch->function;
	    }
	 }
      }
   }
   return tempfunc;
}

/*--------------------------------------------------------------*/
/* Check if an entry exists for a given key-function pair	*/
/*--------------------------------------------------------------*/

int isbound(xcWidget window, int keywstate, int function, short value)
{
   keybinding *ksearch;

   for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding)
      if (keywstate == ksearch->keywstate && function == ksearch->function)
	 if (window == ALL_WINDOWS || window == ksearch->window ||
		ksearch->window == ALL_WINDOWS)
	    if (value == -1 || value == ksearch->value || ksearch->value == -1)
	       return TRUE;

   return FALSE;
}

/*--------------------------------------------------------------*/
/* Return the string associated with a function, or NULL if the	*/
/* function value is out-of-bounds.				*/
/*--------------------------------------------------------------*/

char *func_to_string(int function)
{
   if ((function < 0) || (function >= NUM_FUNCTIONS)) return NULL;
   return function_names[function];
}

/*--------------------------------------------------------------*/
/* Identify a function with the string version of its name	*/
/*--------------------------------------------------------------*/

int string_to_func(const char *funcstring, short *value)
{
   int i;

   for (i = 0; i < NUM_FUNCTIONS; i++)
   {
      if (function_names[i] == NULL) {
	 Fprintf(stderr, "Error: resolve bindings and function strings!\n");
	 return -1;	  /* should not happen? */
      }
      if (!strcmp(funcstring, function_names[i]))
	 return i;
   }

   /* Check if this string might have a value attached */

   if (value != NULL)
      for (i = 0; i < NUM_FUNCTIONS; i++)
         if (!strncmp(funcstring, function_names[i], strlen(function_names[i]))) {
	    sscanf(funcstring + strlen(function_names[i]), "%hd", value);
	    return i;
	 }

   return -1;
}

/*--------------------------------------------------------------*/
/* Make a key sym from a string representing the key state 	*/
/*--------------------------------------------------------------*/

int string_to_key(const char *keystring)
{
   int ct, keywstate = 0;
   const char *kptr = keystring;

   while(1) {
      if (*kptr == '\0') return -1;
      if (!strncmp(kptr, "XK_", 3))
	 kptr += 3;
      else if (!strncmp(kptr, "Shift_", 6)) {
	 keywstate |= SHIFT;
	 kptr += 6;
      }
      else if (!strncmp(kptr, "Capslock_", 9)) {
	 keywstate |= CAPSLOCK;
	 kptr += 9;
      }
      else if (!strncmp(kptr, "Control_", 8)) {
	 keywstate |= CTRL;
	 kptr += 8;
      }
      else if (!strncmp(kptr, "Alt_", 4)) {
	 keywstate |= ALT;
	 kptr += 4;
      }
      else if (!strncmp(kptr, "Meta_", 5)) {
	 keywstate |= ALT;
	 kptr += 5;
      }
      else if (!strncmp(kptr, "Hold_", 5)) {
	 keywstate |= HOLD;
	 kptr += 5;
      }
      else if (*kptr == '^') {
	 kptr++;
	 ct = (int)tolower(*kptr);
	 keywstate |= CTRL | ct;
	 break;
      }
      else if (*(kptr + 1) == '\0') {
	 if ((*kptr) < 32)
	    keywstate |= CTRL | (int)('A' + (*kptr) - 1);
	 else
	    keywstate |= (int)(*kptr);
	 break;
      }
      else {
         if (!strncmp(kptr, "Button", 6)) {
	    switch (*(kptr + 6)) {
	       case '1': keywstate = (Button1Mask << 16); break;
	       case '2': keywstate = (Button2Mask << 16); break;
	       case '3': keywstate = (Button3Mask << 16); break;
	       case '4': keywstate = (Button4Mask << 16); break;
	       case '5': keywstate = (Button5Mask << 16); break;
	    }
         }
	 else {
	    /* When any modifier keys are used, presence of SHIFT */
	    /* requires that the corresponding key be uppercase,  */
	    /* and lack of SHIFT requires lowercase.  Enforce it  */
	    /* here so that it is not necessary for the user to	  */
	    /* do so.						  */
	    if (*(kptr + 1) == '\0') {
	       if (keywstate & SHIFT)
	          ct = (int)toupper(*kptr);
	       else
	          ct = (int)tolower(*kptr);
	       keywstate |= ct;
	    }
	    else
	       keywstate |= XStringToKeysym(kptr);
	 }
	 break;
      }
   }
   return keywstate;
}

/*--------------------------------------------------------------*/
/* Convert a function into a string representing all of its	*/
/* key bindings.						*/
/*--------------------------------------------------------------*/

char *function_binding_to_string(xcWidget window, int function)
{
   keybinding *ksearch;
   char *retstr, *tmpstr;
   Bool first = True;

   retstr = (char *)malloc(1);
   retstr[0] = '\0';
   for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding) {
      if (function == ksearch->function) {
         if (ksearch->window == ALL_WINDOWS || ksearch->window == window) {
	    tmpstr = key_to_string(ksearch->keywstate);
	    if (tmpstr != NULL) {
	       retstr = (char *)realloc(retstr, strlen(retstr) + strlen(tmpstr) + 
		   ((first) ? 1 : 3));
	       if (!first) strcat(retstr, ", ");
	       strcat(retstr, tmpstr);
	       free(tmpstr);
	    }
	    first = False;
	 }
      }
   }
   if (retstr[0] == '\0') {
      retstr = (char *)realloc(retstr, 10);
      strcat(retstr, "<unbound>");
   }
   return(retstr);
}

/*--------------------------------------------------------------*/
/* Convert a key into a string representing all of its function	*/
/* bindings.							*/
/*--------------------------------------------------------------*/

char *key_binding_to_string(xcWidget window, int keywstate)
{
   keybinding *ksearch;
   char *retstr, *tmpstr;
   Bool first = True;

   retstr = (char *)malloc(1);
   retstr[0] = '\0';
   for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding) {
      if (keywstate == ksearch->keywstate) {
         if (ksearch->window == ALL_WINDOWS || ksearch->window == window) {
	    tmpstr = function_names[ksearch->function];
	    if (tmpstr != NULL) {
	       retstr = (char *)realloc(retstr, strlen(retstr) + strlen(tmpstr) + 
		   ((first) ? 1 : 3));
	       if (!first) strcat(retstr, ", ");
	       strcat(retstr, tmpstr);
	    }
	    first = False;
	 }
      }
   }
   if (retstr[0] == '\0') {
      retstr = (char *)realloc(retstr, 10);
      strcat(retstr, "<unbound>");
   }
   return(retstr);
}

/*--------------------------------------------------------------*/
/* Return an allocated string name of the function that		*/
/* is bound to the indicated key state for the indicated	*/
/* window and compattible with the current event mode.		*/
/*--------------------------------------------------------------*/

char *compat_key_to_string(xcWidget window, int keywstate)
{
   char *retstr, *tmpstr;
   int function;

   function = boundfunction(window, keywstate, NULL);
   tmpstr = func_to_string(function);

   /* Pass back "Nothing" for unbound key states, since a	*/
   /* wrapper script may want to use the result as an action.	*/

   if (tmpstr == NULL) {
      retstr = (char *)malloc(8);
      strcpy(retstr, "Nothing");
   }
   else
      retstr = strdup(tmpstr);

   return(retstr);
}

/*--------------------------------------------------------------*/
/* Convert a key sym into a string				*/
/*--------------------------------------------------------------*/

char *key_to_string(int keywstate)
{
   static char hex[17] = "0123456789ABCDEF";
   char *kptr, *str = NULL;
   KeySym ks;
   int kmod;

   ks = keywstate & 0xffff;
   kmod = keywstate >> 16;
#if defined(XC_WIN32) && defined(TCL_WRAPPER)
   if (ks != NoSymbol) str = XKeysymToString_TkW32(ks);
#else
   if (ks != NoSymbol) str = XKeysymToString(ks);
#endif

   kptr = (char *)malloc(32);
   kptr[0] = '\0';
   if (kmod & Mod1Mask) strcat(kptr, "Alt_");
   if (kmod & Mod4Mask) strcat(kptr, "Hold_");
   if (kmod & ControlMask) strcat(kptr, "Control_");
   if (kmod & LockMask) strcat(kptr, "Capslock_");
   if (kmod & ShiftMask) strcat(kptr, "Shift_");

   if (str != NULL) {
      /* 33 is length of all modifiers concatenated + 1 */
      kptr = (char *)realloc(kptr, strlen(str) + 33);
      strcat(kptr, str);
   }
   else {
      kptr = (char *)realloc(kptr, 40);
      if (kmod & Button1Mask) strcat(kptr, "Button1");
      else if (kmod & Button2Mask) strcat(kptr, "Button2");
      else if (kmod & Button3Mask) strcat(kptr, "Button3");
      else if (kmod & Button4Mask) strcat(kptr, "Button4");
      else if (kmod & Button5Mask) strcat(kptr, "Button5");
      else {
	 kptr[0] = '0';
	 kptr[1] = 'x';
	 kptr[2] = hex[(kmod & 0xf)];
	 kptr[3] = hex[(keywstate & 0xf000) >> 12];
	 kptr[4] = hex[(keywstate & 0x0f00) >>  8];
	 kptr[5] = hex[(keywstate & 0x00f0) >>  4];
	 kptr[6] = hex[(keywstate & 0x000f)      ];
	 kptr[7] = '\0';
      }
   }
   return kptr;
}

/*--------------------------------------------------------------*/
/* Print the bindings for the (polygon) edit functions		*/
/*--------------------------------------------------------------*/

void printeditbindings()
{
   char *tstr;

   _STR2[0] = '\0';

   tstr = key_to_string(firstbinding(areawin->area, XCF_Edit_Delete));
   strcat(_STR2, tstr);
   strcat(_STR2, "=");
   strcat(_STR2, func_to_string(XCF_Edit_Delete));
   strcat(_STR2, ", ");
   free(tstr);

   tstr = key_to_string(firstbinding(areawin->area, XCF_Edit_Insert));
   strcat(_STR2, tstr);
   strcat(_STR2, "=");
   strcat(_STR2, func_to_string(XCF_Edit_Insert));
   strcat(_STR2, ", ");
   free(tstr);

   tstr = key_to_string(firstbinding(areawin->area, XCF_Edit_Param));
   strcat(_STR2, tstr);
   strcat(_STR2, "=");
   strcat(_STR2, func_to_string(XCF_Edit_Param));
   strcat(_STR2, ", ");
   free(tstr);

   tstr = key_to_string(firstbinding(areawin->area, XCF_Edit_Next));
   strcat(_STR2, tstr);
   strcat(_STR2, "=");
   strcat(_STR2, func_to_string(XCF_Edit_Next));
   free(tstr);

   /* Use W3printf().  In the Tcl version, this prints to the	*/
   /* message window but does not duplicate the output to 	*/
   /* stdout, where it would be just an annoyance.		*/

   W3printf("%s", _STR2);
}

/*--------------------------------------------------------------*/
/* Remove a key binding from the list				*/
/*								*/
/* Note:  This routine needs to correctly handle ALL_WINDOWS	*/
/* bindings that shadow specific window bindings.		*/
/*--------------------------------------------------------------*/

int remove_binding(xcWidget window, int keywstate, int function)
{
   keybinding *ksearch, *klast = NULL;

   for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding) {
      if (window == ALL_WINDOWS || window == ksearch->window) {
         if ((function == ksearch->function)
		&& (keywstate == ksearch->keywstate)) {
	    if (klast == NULL)
	       keylist = ksearch->nextbinding;
	    else
	       klast->nextbinding = ksearch->nextbinding;
	    free(ksearch);
	    return 0;
         }
      }
      klast = ksearch;
   }
   return -1;
}

/*--------------------------------------------------------------*/
/* Wrapper for remove_binding					*/
/*--------------------------------------------------------------*/

void remove_keybinding(xcWidget window, const char *keystring, const char *fstring)
{
   int function = string_to_func(fstring, NULL);
   int keywstate = string_to_key(keystring);

   if ((function < 0) || (remove_binding(window, keywstate, function) < 0)) {
      Wprintf("Key binding \'%s\' to \'%s\' does not exist in list.",
		keystring, fstring);
   }
}

/*--------------------------------------------------------------*/
/* Add a key binding to the list				*/
/*--------------------------------------------------------------*/

int add_vbinding(xcWidget window, int keywstate, int function, short value)
{
   keybinding *newbinding;

   /* If key is already bound to the function, ignore it */

   if (isbound(window, keywstate, function, value)) return 1;
	   
   /* Add the new key binding */

   newbinding = (keybinding *)malloc(sizeof(keybinding));
   newbinding->window = window;
   newbinding->keywstate = keywstate;
   newbinding->function = function;
   newbinding->value = value;
   newbinding->nextbinding = keylist;
   keylist = newbinding;
   return 0;
}

/*--------------------------------------------------------------*/
/* Wrapper function for key binding without any values		*/
/*--------------------------------------------------------------*/

int add_binding(xcWidget window, int keywstate, int function)
{
   return add_vbinding(window, keywstate, function, (short)-1);
}

/*--------------------------------------------------------------*/
/* Wrapper function for key binding with function as string	*/
/*--------------------------------------------------------------*/

int add_keybinding(xcWidget window, const char *keystring, const char *fstring)
{
   short value = -1;
   int function = string_to_func(fstring, &value);
   int keywstate = string_to_key(keystring);

   if (function < 0)
      return -1;
   else
      return add_vbinding(window, keywstate, function, value);
}

/*--------------------------------------------------------------*/
/* Create list of default key bindings.				*/
/* These are conditional upon any bindings set in the startup	*/
/* file .xcircuitrc.						*/
/*--------------------------------------------------------------*/

void default_keybindings()
{
   add_vbinding(ALL_WINDOWS, XK_1, XCF_Page, 1);
   add_vbinding(ALL_WINDOWS, XK_2, XCF_Page, 2);
   add_vbinding(ALL_WINDOWS, XK_3, XCF_Page, 3);
   add_vbinding(ALL_WINDOWS, XK_4, XCF_Page, 4);
   add_vbinding(ALL_WINDOWS, XK_5, XCF_Page, 5);
   add_vbinding(ALL_WINDOWS, XK_6, XCF_Page, 6);
   add_vbinding(ALL_WINDOWS, XK_7, XCF_Page, 7);
   add_vbinding(ALL_WINDOWS, XK_8, XCF_Page, 8);
   add_vbinding(ALL_WINDOWS, XK_9, XCF_Page, 9);
   add_vbinding(ALL_WINDOWS, XK_0, XCF_Page, 10);

   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_1, XCF_Anchor, 0);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_2, XCF_Anchor, 1);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_3, XCF_Anchor, 2);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_4, XCF_Anchor, 3);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_5, XCF_Anchor, 4);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_6, XCF_Anchor, 5);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_7, XCF_Anchor, 6);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_8, XCF_Anchor, 7);
   add_vbinding(ALL_WINDOWS, SHIFT | XK_KP_9, XCF_Anchor, 8);

   add_vbinding(ALL_WINDOWS, XK_KP_End, XCF_Anchor, 0);
   add_vbinding(ALL_WINDOWS, XK_KP_Down, XCF_Anchor, 1);
   add_vbinding(ALL_WINDOWS, XK_KP_Next, XCF_Anchor, 2);
   add_vbinding(ALL_WINDOWS, XK_KP_Left, XCF_Anchor, 3);
   add_vbinding(ALL_WINDOWS, XK_KP_Begin, XCF_Anchor, 4);
   add_vbinding(ALL_WINDOWS, XK_KP_Right, XCF_Anchor, 5);
   add_vbinding(ALL_WINDOWS, XK_KP_Home, XCF_Anchor, 6);
   add_vbinding(ALL_WINDOWS, XK_KP_Up, XCF_Anchor, 7);
   add_vbinding(ALL_WINDOWS, XK_KP_Prior, XCF_Anchor, 8);

   add_binding(ALL_WINDOWS, XK_Delete, XCF_Text_Delete);
   add_binding(ALL_WINDOWS, ALT | XK_Delete, XCF_Text_Delete_Param);
   add_binding(ALL_WINDOWS, XK_Return, XCF_Text_Return);
   add_binding(ALL_WINDOWS, BUTTON1, XCF_Text_Return);
   add_binding(ALL_WINDOWS, XK_BackSpace, XCF_Text_Delete);
   add_binding(ALL_WINDOWS, XK_Left, XCF_Text_Left);
   add_binding(ALL_WINDOWS, XK_Right, XCF_Text_Right);
   add_binding(ALL_WINDOWS, XK_Up, XCF_Text_Up);
   add_binding(ALL_WINDOWS, XK_Down, XCF_Text_Down);
   add_binding(ALL_WINDOWS, ALT | XK_x, XCF_Text_Split);
   add_binding(ALL_WINDOWS, XK_Home, XCF_Text_Home);
   add_binding(ALL_WINDOWS, XK_End, XCF_Text_End);
   add_binding(ALL_WINDOWS, XK_Tab, XCF_TabForward);
   add_binding(ALL_WINDOWS, SHIFT | XK_Tab, XCF_TabBackward);
#ifdef XK_ISO_Left_Tab
   add_binding(ALL_WINDOWS, SHIFT | XK_ISO_Left_Tab, XCF_TabBackward);
#endif
   add_binding(ALL_WINDOWS, ALT | XK_Tab, XCF_TabStop);
   add_binding(ALL_WINDOWS, XK_KP_Add, XCF_Superscript);
   add_binding(ALL_WINDOWS, XK_KP_Subtract, XCF_Subscript);
   add_binding(ALL_WINDOWS, XK_KP_Enter, XCF_Normalscript);
   add_vbinding(ALL_WINDOWS, ALT | XK_f, XCF_Font, 1000);
   add_binding(ALL_WINDOWS, ALT | XK_b, XCF_Boldfont);
   add_binding(ALL_WINDOWS, ALT | XK_i, XCF_Italicfont);
   add_binding(ALL_WINDOWS, ALT | XK_n, XCF_Normalfont);
   add_binding(ALL_WINDOWS, ALT | XK_u, XCF_Underline);
   add_binding(ALL_WINDOWS, ALT | XK_o, XCF_Overline);
   add_binding(ALL_WINDOWS, ALT | XK_e, XCF_ISO_Encoding);
   add_binding(ALL_WINDOWS, ALT | XK_Return, XCF_Linebreak);
   add_binding(ALL_WINDOWS, ALT | XK_h, XCF_Halfspace);
   add_binding(ALL_WINDOWS, ALT | XK_q, XCF_Quarterspace);
#ifndef TCL_WRAPPER
   add_binding(ALL_WINDOWS, ALT | XK_p, XCF_Parameter);
#endif
   add_binding(ALL_WINDOWS, XK_backslash, XCF_Special);
   add_binding(ALL_WINDOWS, ALT | XK_c, XCF_Special);
   add_binding(ALL_WINDOWS, XK_p, XCF_Edit_Param);
   add_binding(ALL_WINDOWS, XK_d, XCF_Edit_Delete);
   add_binding(ALL_WINDOWS, XK_Delete, XCF_Edit_Delete);
   add_binding(ALL_WINDOWS, XK_i, XCF_Edit_Insert);
   add_binding(ALL_WINDOWS, XK_Insert, XCF_Edit_Insert);
   add_binding(ALL_WINDOWS, XK_e, XCF_Edit_Next);
   add_binding(ALL_WINDOWS, BUTTON1, XCF_Edit_Next);
   add_binding(ALL_WINDOWS, XK_A, XCF_Attach);
   add_binding(ALL_WINDOWS, XK_V, XCF_Virtual);
   add_binding(ALL_WINDOWS, XK_l, XCF_Next_Library);
   add_binding(ALL_WINDOWS, XK_L, XCF_Library_Directory);
   add_binding(ALL_WINDOWS, XK_c, XCF_Library_Copy);
   add_binding(ALL_WINDOWS, XK_E, XCF_Library_Edit);
   add_binding(ALL_WINDOWS, XK_e, XCF_Library_Edit);
   add_binding(ALL_WINDOWS, XK_D, XCF_Library_Delete);
   add_binding(ALL_WINDOWS, XK_C, XCF_Library_Duplicate);
   add_binding(ALL_WINDOWS, XK_H, XCF_Library_Hide);
   add_binding(ALL_WINDOWS, XK_V, XCF_Library_Virtual);
   add_binding(ALL_WINDOWS, XK_M, XCF_Library_Move);
   add_binding(ALL_WINDOWS, XK_m, XCF_Library_Move);
   add_binding(ALL_WINDOWS, XK_P, XCF_Page_Directory);
   add_binding(ALL_WINDOWS, XK_less, XCF_Library_Pop);
   add_binding(ALL_WINDOWS, HOLD | BUTTON1, XCF_Library_Pop);
   add_binding(ALL_WINDOWS, XK_h, XCF_Help);
   add_binding(ALL_WINDOWS, XK_question, XCF_Help);
   add_binding(ALL_WINDOWS, XK_space, XCF_Redraw);
   add_binding(ALL_WINDOWS, XK_Redo, XCF_Redraw);
   add_binding(ALL_WINDOWS, XK_Undo, XCF_Redraw);
   add_binding(ALL_WINDOWS, XK_Home, XCF_View);
   add_binding(ALL_WINDOWS, XK_v, XCF_View);
   add_binding(ALL_WINDOWS, XK_Z, XCF_Zoom_In);
   add_binding(ALL_WINDOWS, XK_z, XCF_Zoom_Out);
   add_vbinding(ALL_WINDOWS, XK_p, XCF_Pan, 0);
   add_binding(ALL_WINDOWS, XK_plus, XCF_Double_Snap);
   add_binding(ALL_WINDOWS, XK_minus, XCF_Halve_Snap);
   add_vbinding(ALL_WINDOWS, XK_Left, XCF_Pan, 1);
   add_vbinding(ALL_WINDOWS, XK_Right, XCF_Pan, 2);
   add_vbinding(ALL_WINDOWS, XK_Up, XCF_Pan, 3);
   add_vbinding(ALL_WINDOWS, XK_Down, XCF_Pan, 4);
   add_binding(ALL_WINDOWS, XK_W, XCF_Write);
   add_vbinding(ALL_WINDOWS, XK_O, XCF_Rotate, -5);
   add_vbinding(ALL_WINDOWS, XK_o, XCF_Rotate, 5);
   add_vbinding(ALL_WINDOWS, XK_R, XCF_Rotate, -15);
   add_vbinding(ALL_WINDOWS, XK_r, XCF_Rotate, 15);
   add_binding(ALL_WINDOWS, XK_f, XCF_Flip_X);
   add_binding(ALL_WINDOWS, XK_F, XCF_Flip_Y);
   add_binding(ALL_WINDOWS, XK_S, XCF_Snap);
   add_binding(ALL_WINDOWS, XK_less, XCF_Pop);
   add_binding(ALL_WINDOWS, XK_greater, XCF_Push);
   add_binding(ALL_WINDOWS, XK_Delete, XCF_Delete);
   add_binding(ALL_WINDOWS, XK_d, XCF_Delete);
   add_binding(ALL_WINDOWS, XK_F19, XCF_Select);
   add_binding(ALL_WINDOWS, XK_b, XCF_Box);
   add_binding(ALL_WINDOWS, XK_a, XCF_Arc);
   add_binding(ALL_WINDOWS, XK_t, XCF_Text);
   add_binding(ALL_WINDOWS, XK_X, XCF_Exchange);
   add_binding(ALL_WINDOWS, XK_c, XCF_Copy);
   add_binding(ALL_WINDOWS, XK_j, XCF_Join);
   add_binding(ALL_WINDOWS, XK_J, XCF_Unjoin);
   add_binding(ALL_WINDOWS, XK_s, XCF_Spline);
   add_binding(ALL_WINDOWS, XK_e, XCF_Edit);
   add_binding(ALL_WINDOWS, XK_u, XCF_Undo);
   add_binding(ALL_WINDOWS, XK_U, XCF_Redo);
   add_binding(ALL_WINDOWS, XK_M, XCF_Select_Save);
   add_binding(ALL_WINDOWS, XK_m, XCF_Select_Save);
   add_binding(ALL_WINDOWS, XK_x, XCF_Unselect);
   add_binding(ALL_WINDOWS, XK_bar, XCF_Dashed);
   add_binding(ALL_WINDOWS, XK_colon, XCF_Dotted);
   add_binding(ALL_WINDOWS, XK_underscore, XCF_Solid);
   add_binding(ALL_WINDOWS, XK_percent, XCF_Prompt);
   add_binding(ALL_WINDOWS, XK_period, XCF_Dot);
#ifndef TCL_WRAPPER
   /* TCL_WRAPPER version req's binding to specific windows */
   add_binding(ALL_WINDOWS, BUTTON1, XCF_Wire);
#endif
   add_binding(ALL_WINDOWS, XK_w, XCF_Wire);
   add_binding(ALL_WINDOWS, CTRL | ALT | XK_q, XCF_Exit);
   add_binding(ALL_WINDOWS, HOLD | BUTTON1, XCF_Move);
   add_binding(ALL_WINDOWS, BUTTON1, XCF_Continue_Element);
   add_binding(ALL_WINDOWS, BUTTON1, XCF_Continue_Copy);
   add_binding(ALL_WINDOWS, BUTTON1, XCF_Finish);
   add_binding(ALL_WINDOWS, XK_Escape, XCF_Cancel);
   add_binding(ALL_WINDOWS, ALT | XK_r, XCF_Rescale);
   add_binding(ALL_WINDOWS, ALT | XK_s, XCF_SnapTo);
   add_binding(ALL_WINDOWS, ALT | XK_q, XCF_Netlist);
   add_binding(ALL_WINDOWS, XK_slash, XCF_Swap);
   add_binding(ALL_WINDOWS, XK_T, XCF_Pin_Label);
   add_binding(ALL_WINDOWS, XK_G, XCF_Pin_Global);
   add_binding(ALL_WINDOWS, XK_I, XCF_Info_Label);
   add_binding(ALL_WINDOWS, ALT | XK_w, XCF_Connectivity);

/* These are for test purposes only.  Menu selection is	*/
/* preferred.						*/

/* add_binding(ALL_WINDOWS, ALT | XK_d, XCF_Sim);		*/
/* add_binding(ALL_WINDOWS, ALT | XK_a, XCF_SPICE);		*/
/* add_binding(ALL_WINDOWS, ALT | XK_f, XCF_SPICEflat);	*/
/* add_binding(ALL_WINDOWS, ALT | XK_p, XCF_PCB);		*/

   /* Avoid spurious Num_Lock messages */
   add_binding(ALL_WINDOWS, XK_Num_Lock, XCF_Nothing);

   /* 2-button vs. 3-button mouse bindings (set with -2	*/
   /* commandline option; 3-button bindings default)	*/

   if (pressmode == 1) {
      add_binding(ALL_WINDOWS, BUTTON3, XCF_Text_Return);
      add_binding(ALL_WINDOWS, BUTTON3, XCF_Select);
      add_binding(ALL_WINDOWS, HOLD | BUTTON3, XCF_SelectBox);
      add_binding(ALL_WINDOWS, BUTTON3, XCF_Finish_Element);
      add_binding(ALL_WINDOWS, BUTTON3, XCF_Finish_Copy);

      add_binding(ALL_WINDOWS, XK_BackSpace, XCF_Cancel_Last);
      add_binding(ALL_WINDOWS, XK_BackSpace, XCF_Cancel);
   }
   else {
      add_binding(ALL_WINDOWS, BUTTON2, XCF_Text_Return);
      add_binding(ALL_WINDOWS, SHIFT | BUTTON1, XCF_Text_Return);
      add_binding(ALL_WINDOWS, BUTTON2, XCF_Select);
      add_binding(ALL_WINDOWS, SHIFT | BUTTON1, XCF_Select);
      add_binding(ALL_WINDOWS, HOLD | BUTTON2, XCF_SelectBox);
      add_binding(ALL_WINDOWS, SHIFT | HOLD | BUTTON1, XCF_SelectBox);
      add_binding(ALL_WINDOWS, BUTTON2, XCF_Finish_Element);
      add_binding(ALL_WINDOWS, SHIFT | BUTTON1, XCF_Finish_Element);
      add_binding(ALL_WINDOWS, BUTTON2, XCF_Finish_Copy);
      add_binding(ALL_WINDOWS, SHIFT | BUTTON1, XCF_Finish_Copy);
      add_binding(ALL_WINDOWS, BUTTON3, XCF_Cancel_Last);
      add_binding(ALL_WINDOWS, BUTTON3, XCF_Cancel);
   }
}

#ifndef TCL_WRAPPER
/*----------------------------------------------*/
/* Mode-setting rebindings (non-Tcl version)	*/
/*----------------------------------------------*/

static int button1mode = XCF_Wire;

/*--------------------------------------------------------------*/
/* Re-bind BUTTON1 to the indicated function and optional value */
/*--------------------------------------------------------------*/

void mode_rebinding(int newmode, int newvalue)
{
   xcWidget window = areawin->area;

   remove_binding(window, BUTTON1, button1mode);
   add_vbinding(window, BUTTON1, newmode, (short)newvalue);
   button1mode = newmode;
   toolcursor(newmode);
}

/*--------------------------------------------------------------*/
/* Execute the function associated with the indicated BUTTON1	*/
/* mode, but return the keybinding to its previous state.	*/
/*--------------------------------------------------------------*/

void mode_tempbinding(int newmode, int newvalue)
{
   short saveval;
   XPoint cpos;
   xcWidget window = areawin->area;

   if (boundfunction(window, BUTTON1, &saveval) == button1mode) {
      remove_binding(window, BUTTON1, button1mode);
      add_vbinding(window, BUTTON1, newmode, (short)newvalue);
      cpos = UGetCursor();
      eventdispatch(BUTTON1, (int)cpos.x, (int)cpos.y);
      remove_binding(window, BUTTON1, newmode);
      add_vbinding(window, BUTTON1, button1mode, saveval);
   }
   else
      fprintf(stderr, "Error: No such button1 binding %s\n",
		func_to_string(button1mode));
}

#endif /* TCL_WRAPPER */

#undef ALT
#undef CTRL
#undef CAPSLOCK
#undef SHIFT
#undef BUTTON1
#undef BUTTON2
#undef BUTTON3

/*--------------------------------------------------------------*/
