/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        MapEvents.c
 **
 **   Project:     X Widgets
 **
 **   Description: Menu Manager Support Routines
 **
 **         This file contains a series of support routines, used by
 **         the menu manager meta-class and any of its subclasses,
 **         to convert an event string into an internal format.  The
 **         internal format may then be used to compare against real
 **         X events.  These routines are capable of handling a subset
 **         of the string formats supported by the translation manager.
 **       
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 **   Permission to use, copy, modify, and distribute this software 
 **   and its documentation for any purpose and without fee is hereby 
 **   granted, provided that the above copyright notice appear in all 
 **   copies and that both that copyright notice and this permission 
 **   notice appear in supporting documentation, and that the names of 
 **   Hewlett-Packard, Digital or  M.I.T.  not be used in advertising or 
 **   publicity pertaining to distribution of the software without 
 **   written prior permission.
 **   
 **   DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 **   ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 **   DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 **   ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 **   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 **   ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 **   SOFTWARE.
 **   
 *****************************************************************************
 *************************************<+>*************************************/



#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xutil.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>


static Boolean ParseImmed();
static Boolean ParseKeySym();



typedef struct {
   char       * event;
   XrmQuark     signature;
   unsigned int eventType;
   Boolean      (*parseProc)();
   unsigned int closure;
} EventKey;

static EventKey modifierStrings[] = {

  /* Modifier,  Quark,  Mask */

    {"None",    NULLQUARK,      0,    NULL,	None},
    {"Shift",	NULLQUARK,      0,    NULL,	ShiftMask},
    {"Lock",	NULLQUARK,      0,    NULL,	LockMask},
    {"Ctrl",	NULLQUARK,      0,    NULL,	ControlMask},
    {"Mod1",	NULLQUARK,      0,    NULL,	Mod1Mask},
    {"Mod2",	NULLQUARK,      0,    NULL,	Mod2Mask},
    {"Mod3",	NULLQUARK,      0,    NULL,	Mod3Mask},
    {"Mod4",	NULLQUARK,      0,    NULL,	Mod4Mask},
    {"Mod5",	NULLQUARK,      0,    NULL,	Mod5Mask},
    {"Meta",	NULLQUARK,      0,    NULL,	Mod1Mask},
    {NULL,	NULLQUARK,	0,    NULL,	None},
};


static EventKey buttonEvents[] = {

/* Event Name,	  Quark, Event Type,	DetailProc	Closure */

{"Btn1Down",	    NULLQUARK, ButtonPress,	ParseImmed,	Button1},
{"Btn2Down", 	    NULLQUARK, ButtonPress,	ParseImmed,	Button2},
{"Btn3Down", 	    NULLQUARK, ButtonPress,	ParseImmed,	Button3},
{"Btn4Down", 	    NULLQUARK, ButtonPress,	ParseImmed,	Button4},
{"Btn5Down", 	    NULLQUARK, ButtonPress,	ParseImmed,	Button5},
{"Btn1Up", 	    NULLQUARK, ButtonRelease,   ParseImmed,	Button1},
{"Btn2Up", 	    NULLQUARK, ButtonRelease,   ParseImmed,	Button2},
{"Btn3Up", 	    NULLQUARK, ButtonRelease,   ParseImmed,	Button3},
{"Btn4Up", 	    NULLQUARK, ButtonRelease,   ParseImmed,	Button4},
{"Btn5Up", 	    NULLQUARK, ButtonRelease,   ParseImmed,	Button5},
{ NULL,		    NULLQUARK, 0,		NULL,		None}};


static EventKey keyEvents[] = {

/* Event Name,	  Quark, Event Type,	DetailProc	Closure */

{"KeyPress",	    NULLQUARK, KeyPress,	ParseKeySym,	None},
{"Key", 	    NULLQUARK, KeyPress,	ParseKeySym,	None},
{"KeyDown", 	    NULLQUARK, KeyPress,	ParseKeySym,	None},
{"KeyUp", 	    NULLQUARK, KeyRelease,   	ParseKeySym,	None},
{"KeyRelease", 	    NULLQUARK, KeyRelease,   	ParseKeySym,	None},
{ NULL,		    NULLQUARK, 0,		 NULL,		None}};


static unsigned int buttonModifierMasks[] = {
    0, Button1Mask, Button2Mask, Button3Mask, Button4Mask, Button5Mask
};

static initialized = FALSE;



/*************************************<->*************************************
 *
 *  Numeric convertion routines
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static unsigned int StrToHex(str)
    String str;
{
    register char   c;
    register int    val = 0;

    while (c = *str) {
	if ('0' <= c && c <= '9') val = val*16+c-'0';
	else if ('a' <= c && c <= 'z') val = val*16+c-'a'+10;
	else if ('A' <= c && c <= 'Z') val = val*16+c-'A'+10;
	else return -1;
	str++;
    }

    return val;
}

static unsigned int StrToOct(str)
    String str;
{
    register char c;
    register int  val = 0;

    while (c = *str) {
        if ('0' <= c && c <= '7') val = val*8+c-'0'; else return -1;
	str++;
    }

    return val;
}

static unsigned int StrToNum(str)
    String str;
{
    register char c;
    register int val = 0;

    if (*str == '0') {
	str++;
	if (*str == 'x' || *str == 'X') return StrToHex(++str);
	else return StrToOct(str);
    }

    while (c = *str) {
	if ('0' <= c && c <= '9') val = val*10+c-'0';
	else return -1;
	str++;
    }

    return val;
}


/*************************************<->*************************************
 *
 *  FillInQuarks (parameters)
 *
 *   Description:
 *   -----------
 *     Converts each string entry in the modifier/event tables to a
 *     quark, thus facilitating faster comparisons.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void FillInQuarks (table)

    EventKey * table;

{
    register int i;

    for (i=0; table[i].event; i++)
        table[i].signature = XrmStringToQuark(table[i].event);
}


/*************************************<->*************************************
 *
 *  LookupModifier (parameters)
 *
 *   Description:
 *   -----------
 *     Compare the passed in string to the list of valid modifiers.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean LookupModifier (name, valueP)

    String name;
    unsigned int *valueP;

{
    register int i;
    register XrmQuark signature = XrmStringToQuark(name);

    for (i=0; modifierStrings[i].event != NULL; i++)
	if (modifierStrings[i].signature == signature) {
	    *valueP = modifierStrings[i].closure;
	    return TRUE;
	}

    return FALSE;
}


/*************************************<->*************************************
 *
 *  ScanAlphanumeric (parameters)
 *
 *   Description:
 *   -----------
 *     Scan string until a non-alphanumeric character is encountered.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static String ScanAlphanumeric (str)

    register String str;

{
    while (
        ('A' <= *str && *str <= 'Z') || ('a' <= *str && *str <= 'z')
	|| ('0' <= *str && *str <= '9')) str++;
    return str;
}


/*************************************<->*************************************
 *
 *  ScanWhitespace (parameters)
 *
 *   Description:
 *   -----------
 *     Scan the string, skipping over all white space characters.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static String ScanWhitespace(str)
    register String str;
{
    while (*str == ' ' || *str == '\t') str++;
    return str;
}


/*************************************<->*************************************
 *
 *  ParseImmed (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean ParseImmed (str, closure, detail)

   String str;
   unsigned int closure;
   unsigned int * detail;

{
   *detail = closure;
   return (TRUE);
}


/*************************************<->*************************************
 *
 *  ParseKeySym (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean ParseKeySym (str, closure, detail)

   String str;
   unsigned int closure;
   unsigned int * detail;

{
    char keySymName[100], *start;

    str = ScanWhitespace(str);

    if (*str == '\\') {
	str++;
	keySymName[0] = *str;
	if (*str != '\0' && *str != '\n') str++;
	keySymName[1] = '\0';
	*detail = XStringToKeysym(keySymName);
    } else if (*str == ',' || *str == ':') {
       /* No detail; return a failure */
       *detail = NoSymbol;
       return (FALSE);
    } else {
	start = str;
	while (
		*str != ','
		&& *str != ':'
		&& *str != ' '
		&& *str != '\t'
                && *str != '\n'
		&& *str != '\0') str++;
	(void) strncpy(keySymName, start, str-start);
	keySymName[str-start] = '\0';
	*detail = XStringToKeysym(keySymName);
    }

    if (*detail == NoSymbol)
    {
       if (( '0' <= keySymName[0]) && (keySymName[0] <= '9'))
       {
          *detail = StrToNum(keySymName);
          return (TRUE);
       }
       return (FALSE);
    }
    else
       return (TRUE);
}


/*************************************<->*************************************
 *
 *  ParseModifiers (parameters)
 *
 *   Description:
 *   -----------
 *     Parse the string, extracting all modifier specifications.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static String ParseModifiers(str, modifiers, status)

    register String str;
    unsigned int  * modifiers;
    Boolean       * status;

{
    register String start;
    char modStr[100];
    Boolean notFlag, exclusive;
    unsigned int maskBit;

    /* Initially assume all is going to go well */
    *status = TRUE;
    *modifiers = 0;
 
    /* Attempt to parse the first button modifier */
    str = ScanWhitespace(str);
    start = str;
    str = ScanAlphanumeric(str);
    if (start != str) {
         (void) strncpy(modStr, start, str-start);
          modStr[str-start] = '\0';
          if (LookupModifier(modStr, &maskBit))
          {
	    if (maskBit== None) {
		*modifiers = 0;
                str = ScanWhitespace(str);
	        return str;
            }
         }
         str = start;
    }

   
    /* Keep parsing modifiers, until the event specifier is encountered */
    while ((*str != '<') && (*str != '\0')) {
        if (*str == '~') {
             notFlag = TRUE;
             str++;
          } else 
              notFlag = FALSE;

	start = str;
        str = ScanAlphanumeric(str);
        if (start == str) {
           /* ERROR: Modifier or '<' missing */
           *status = FALSE;
           return str;
        }
        (void) strncpy(modStr, start, str-start);
        modStr[str-start] = '\0';

        if (!LookupModifier(modStr, &maskBit))
        {
           /* Unknown modifier name */
           *status = FALSE;
           return str;
        }

	if (notFlag) 
           *modifiers &= ~maskBit;
	else 
           *modifiers |= maskBit;
        str = ScanWhitespace(str);
    }

    return str;
}


/*************************************<->*************************************
 *
 *  ParseEventType (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static String ParseEventType(str, table, eventType, index, status)

    register String str;
    EventKey * table;
    unsigned int * eventType;
    Cardinal     * index;
    Boolean      * status;

{
    String start = str;
    char eventTypeStr[100];
    register Cardinal   i;
    register XrmQuark	signature;

    /* Parse out the event string */
    str = ScanAlphanumeric(str);
    (void) strncpy(eventTypeStr, start, str-start);
    eventTypeStr[str-start] = '\0';

    /* Attempt to match the parsed event against our supported event set */
    signature = XrmStringToQuark(eventTypeStr);
    for (i = 0; table[i].signature != NULLQUARK; i++)
        if (table[i].signature == signature)
        {
           *index = i;
           *eventType = table[*index].eventType;

           *status = TRUE;
           return str; 
        }

    /* Unknown event specified */
    *status = FALSE;
    return (str);
}


/*************************************<->*************************************
 *
 *  _XwMapEvent (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean 
_XwMapEvent (str, table, eventType, detail, modifiers)

    register String str;
    EventKey     * table;
    unsigned int * eventType;
    unsigned int * detail;
    unsigned int * modifiers;

{
    Cardinal index;
    Boolean  status;
 
    /* Initialize, if first time called */
    if (!initialized)
    {
       initialized = TRUE;
       FillInQuarks (buttonEvents);
       FillInQuarks (modifierStrings);
       FillInQuarks (keyEvents);
    }

    /* Parse the modifiers */
    str = ParseModifiers(str, modifiers, &status);
    if ( status == FALSE || *str != '<') 
       return (FALSE);
    else 
       str++;

    /* Parse the event type and detail */
    str = ParseEventType(str, table, eventType, &index, &status);
    if (status == FALSE || *str != '>') 
       return (FALSE);
    else 
       str++;

    /* Save the detail */
    return ((*(table[index].parseProc))(str, table[index].closure, detail));
}


/*************************************<->*************************************
 *
 *  _XwMapBtnEvent (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

Boolean _XwMapBtnEvent (str, eventType, button, modifiers)

    register String str;
    unsigned int * eventType;
    unsigned int * button;
    unsigned int * modifiers;

{
    if (_XwMapEvent (str, buttonEvents, eventType, button, modifiers) == FALSE)
       return (FALSE);

    /* 
     * The following is a fix for an X11 deficiency in regards to 
     * modifiers in grabs.
     */
    if (*eventType == ButtonRelease)
    {
	/* the button that is going up will always be in the modifiers... */
	*modifiers |= buttonModifierMasks[*button];
    }

    return (TRUE);
}


/*************************************<->*************************************
 *
 *  _XwMapKeyEvent (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

Boolean _XwMapKeyEvent (str, eventType, key, modifiers)

    register String str;
    unsigned int * eventType;
    unsigned int * key;
    unsigned int * modifiers;

{
    return (_XwMapEvent (str, keyEvents, eventType, key, modifiers));
}


/*************************************<->*************************************
 *
 *  _XwMatchBtnEvent (parameters)
 *
 *   Description:
 *   -----------
 *     Compare the passed in event to the event described by the parameter
 *     list.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

Boolean _XwMatchBtnEvent (event, eventType, button, modifiers)

   XButtonEvent * event;
   unsigned int   eventType;
   unsigned int   button;
   unsigned int   modifiers;

{
   if ((event->type == eventType) &&
       (event->button == button) &&
       (event->state == modifiers))
      return (TRUE);
   else
      return (FALSE);
}


/*************************************<->*************************************
 *
 *  _XwMatchKeyEvent (parameters)
 *
 *   Description:
 *   -----------
 *     Compare the passed in event to the event described by the parameter
 *     list.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

Boolean _XwMatchKeyEvent (event, eventType, key, modifiers)

   XKeyEvent    * event;
   unsigned int   eventType;
   unsigned int   key;
   unsigned int   modifiers;

{
   if ((event->type == eventType) &&
       (event->keycode == key) &&
       (event->state == modifiers))
      return (TRUE);
   else
      return (FALSE);
}


/*************************************<->*************************************
 *
 *  _XwValidModifier (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

Boolean _XwValidModifier (str)

   register String str;

{
   unsigned int modifiers;
   Boolean status;

   /*
    * Scan the string for a valid set of event modifiers; valid includes
    * a NULL string, an empty string, or all white space.  A NULL is
    * inserted into the string at the end of the modifiers.
    */

   if ((str == NULL) || (*str == '\0'))
      return (TRUE);

   str = ParseModifiers (str, &modifiers, &status);

   if (status == TRUE)
      *str = '\0';

   return (status);
}

/*************************************<->*************************************
 *
 *  _XwMapToHex (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

String _XwMapToHex (menuMgrId)

   unsigned long menuMgrId;

{
   static char buffer[(sizeof(Widget) << 1) + 1];
   int i;
   char digit;

   /* Convert the menu manager widget id into a hex string */
   for (i = (sizeof(Widget) << 1) - 1; i >= 0; i--)
   {
      digit = (char) menuMgrId & 0x0F;
      buffer[i] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
      menuMgrId = menuMgrId >> 4;
   }

   buffer[sizeof(Widget) << 1] = '\0';
   return (buffer);
}


/*************************************<->*************************************
 *
 *  _XwMapFromHex (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

unsigned long _XwMapFromHex (str)

   String str;

{
   unsigned long id = 0;
   int digit;

   /* Convert the hex string into a menu manager widget id */
   while (str && *str)
   {
      digit = (int) ((*str >= '0') && (*str <= '9')) ? (*str - '0') : 
                                                       (*str + 10 - 'A');
      id = (id * 16) + digit;
      str++;
   }

   return (id);
}
