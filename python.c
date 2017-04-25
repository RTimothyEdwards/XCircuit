/*--------------------------------------------------------------*/
/* python.c --- Embedded python interpreter for xcircuit	*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University    */
/*--------------------------------------------------------------*/
/* NOTE: These routines work only if python-2.0 is installed.   */
/*	Requires library file libpython2.0.a.  Hopefully	*/
/*	one day there will be a standard libpython${VERSION}.so	*/
/*	so that the whole thing doesn't have to be linked	*/
/*	into the xcircuit executable. . .			*/
/*								*/
/*	Modeled after demo.c in Python2.0 source Demo/embed	*/
/*--------------------------------------------------------------*/

#if defined(HAVE_PYTHON) && !defined(TCL_WRAPPER)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>	/* for usleep() */
#include <string.h>
   
#include <Python.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "Xw/Xw.h"
#include "Xw/MenuBtn.h"
      
#ifdef HAVE_XPM
#include <X11/xpm.h>
#include "lib/pixmaps/q.xpm"
#endif

PyObject *xcmod;

/*----------------------------------------------------------------------*/
/* Local includes                                                       */
/*----------------------------------------------------------------------*/
      
#include "xcircuit.h"
#include "menudep.h"
#include "colordefs.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/

#include "prototypes.h"

static PyObject *xc_new(PyObject *, PyObject *);
static PyObject *xc_set(PyObject *, PyObject *);
static PyObject *xc_override(PyObject *, PyObject *);
static PyObject *xc_library(PyObject *, PyObject *);
static PyObject *xc_font(PyObject *, PyObject *);
static PyObject *xc_color(PyObject *, PyObject *);
static PyObject *xc_pause(PyObject *, PyObject *);
static PyObject *xc_refresh(PyObject *, PyObject *);
static PyObject *xc_bind(PyObject *, PyObject *);
static PyObject *xc_unbind(PyObject *, PyObject *);
static PyObject *xc_simplepopup(PyObject *, PyObject *);
static PyObject *xc_popupprompt(PyObject *, PyObject *);
static PyObject *xc_filepopup(PyObject *, PyObject *);
static PyObject *xc_newbutton(PyObject *, PyObject *);
static PyObject *xc_page(PyObject *, PyObject *);
static PyObject *xc_load(PyObject *, PyObject *);
static PyObject *xc_pan(PyObject *, PyObject *);
static PyObject *xc_zoom(PyObject *, PyObject *);
static PyObject *xc_getcursor();
static PyObject *xc_getwindow();
static PyObject *xc_reset();
static PyObject *xc_netlist();
#ifdef HAVE_XPM
static PyObject *xc_newtool(PyObject *, PyObject *);
#endif

int string_to_type(const char *);

/*----------------------------------------------------------------------*/
/* External variable declarations                                       */
/*----------------------------------------------------------------------*/
      
extern char _STR2[250], _STR[150];
extern fontinfo *fonts;
extern short fontcount;
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern short beeper; 
extern Widget menuwidgets[];
extern Display *dpy;
extern keybinding *keylist;
extern int number_colors;
extern colorindex *colorlist;
extern ApplicationData appdata;
#ifdef HAVE_XPM
extern Widget toolbar;
#endif

/*----------------------------------------------------------------------*/

short flags = -1;

#define LIBOVERRIDE     1
#define LIBLOADED       2
#define COLOROVERRIDE   4
#define FONTOVERRIDE    8
#define KEYOVERRIDE	16

/*----------------------------------------------------------------*/
/* Similar to "PyRun_SimpleString()" but returns a PyObject value */
/* rather than a status word.					  */
/*----------------------------------------------------------------*/

PyObject *PyRun_ObjString(char *string)
{
   PyObject *m, *d, *v;
   m = PyImport_AddModule("__main__");
   if (m == NULL) return NULL;
   d = PyModule_GetDict(m);
   v = PyRun_String(string, Py_file_input, d, d);
   return v;
}

/*--------------------------------------------------------------*/
/* Define all of the xcircuit functions available to Python	*/
/* Currently this does the bare minimum of having Python do the	*/
/* argument parsing; ideally, we would like to be doing more	*/
/* fundamental processing here.					*/
/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Extract text from a popup and call the indicated python	*/
/* function.   If there is no text, function is called without	*/
/* arguments.							*/
/*--------------------------------------------------------------*/

void pypromptvalue(Widget w, char *functionptr)
{
   if (strlen(_STR2) == 0)
      sprintf(_STR, "%s()\n", functionptr);
   else
      sprintf(_STR, "%s('%s')\n", functionptr, _STR2);

   PyRun_SimpleString(_STR);
   refresh(NULL, NULL, NULL);
}

/*--------------------------------------------------------------*/
/* Pop up a prompt, and return the value to the indicated	*/
/* python function.						*/
/*--------------------------------------------------------------*/

static PyObject *pypopupprompt(PyObject *self, PyObject *args,
	const char *dofile, char *displaystr)
{
   const char *pfunction = NULL;
   const char *prompt = NULL;
   buttonsave *savebutton;

   if (!PyArg_ParseTuple(args, "ss:popupprompt", &prompt, &pfunction))
      return PyInt_FromLong(-1L);


   savebutton = getgeneric(NULL, NULL, (void *)pfunction);
   popupprompt(areawin->area, (char *)prompt, displaystr,
		pypromptvalue, savebutton, dofile);

   return PyInt_FromLong(0L);
}

/*--------------------------------------------------------------*/
/* Wrappers for pypopuprompt()					*/
/*--------------------------------------------------------------*/

static PyObject *xc_popupprompt(PyObject *self, PyObject *args)
{
   return pypopupprompt(self, args, NULL, "");
}

static PyObject *xc_filepopup(PyObject *self, PyObject *args)
{
   return pypopupprompt(self, args, "", "");
}

static PyObject *xc_simplepopup(PyObject *self, PyObject *args)
{
   _STR2[0] = '\0';
   return pypopupprompt(self, args, False, NULL);
}

/*--------------------------------------------------------------*/
/* Create objects and return a handle to that object in the	*/
/* form of a Python integer Object.				*/
/*--------------------------------------------------------------*/

static PyObject *xc_new(PyObject *self, PyObject *args)
{
   const char *etype;
   int type;

   genericptr *newgen;
   arcptr *newarc;
   splineptr *newspline;
   polyptr *newpoly;
   labelptr *newlabel;
   pathptr *newpath;
   objinstptr *newobjinst;

   if (!PyArg_ParseTuple(args, "s:newelement", &etype))
      return NULL;
   
   switch(type = string_to_type(etype)) {
      case ARC:
         NEW_ARC(newarc, topobject);
	 arcdefaults(*newarc, 0, 0);
	 newgen = (genericptr *)newarc;
	 break;
      case SPLINE:
         NEW_SPLINE(newspline, topobject);
	 splinedefaults(*newspline, 0, 0);
	 newgen = (genericptr *)newspline;
	 break;
      case POLYGON:
         NEW_POLY(newpoly, topobject);
	 polydefaults(*newpoly, 4, 0, 0);
	 newgen = (genericptr *)newpoly;
	 break;
      case LABEL:
         NEW_LABEL(newlabel, topobject);
	 labeldefaults(*newlabel, 0, 0, 0);
	 newgen = (genericptr *)newlabel;
	 break;
      case PATH:
         NEW_PATH(newpath, topobject);
	 pathdefaults(*newpath, 0, 0);
	 newgen = (genericptr *)newpath;
	 break;
      case OBJINST:
         NEW_OBJINST(newobjinst, topobject);
	 instancedefaults(*newobjinst, NULL, 0, 0);
	 newgen = (genericptr *)newobjinst;
	 break;
      default:
         PyErr_SetString(PyExc_TypeError,
		"newelement() 2nd argument must be a valid element type");
         return NULL;
   } 
   incr_changes(topobject);
   invalidate_netlist(topobject);

   return PyInt_FromLong((long)(*newgen));
}

/*--------------------------------------------------------------*/
/* Convert object type to a string				*/
/*--------------------------------------------------------------*/

char *type_to_string(int type)
{
   char *retstr = NULL;

   switch(type) {
      case LABEL:
	 retstr = malloc(6);
	 strcpy(retstr, "Label");
	 break;
      case POLYGON:
	 retstr = malloc(8);
	 strcpy(retstr, "Polygon");
	 break;
      case SPLINE:
	 retstr = malloc(12);
	 strcpy(retstr, "Bezier Curve");
	 break;
      case OBJINST:
	 retstr = malloc(16);
	 strcpy(retstr, "Object Instance");
	 break;
      case PATH:
	 retstr = malloc(5);
	 strcpy(retstr, "Path");
	 break;
      case ARC:
	 retstr = malloc(4);
	 strcpy(retstr, "Arc");
	 break;
   }
   return retstr;	 
}

/*--------------------------------------------------------------*/
/* Convert a string to an element type				*/
/*--------------------------------------------------------------*/

int string_to_type(const char *etype)
{
   if (!strcmp(etype, "Arc"))
      return ARC;
   else if (!strcmp(etype, "Bezier Curve"))
      return SPLINE;
   else if (!strcmp(etype, "Polygon"))
      return POLYGON;
   else if (!strcmp(etype, "Label"))
      return LABEL;
   else if (!strcmp(etype, "Path"))
      return PATH;
   else if (!strcmp(etype, "Object Instance"))
      return OBJINST;
   else
      return -1;
}

/*--------------------------------------------------------------*/
/* Convert color index to 3-tuple and vice versa		*/
/* We assume that this color exists in the color table.		*/
/*--------------------------------------------------------------*/

PyObject *PyIndexToRGB(int cidx)
{
   int i;
   PyObject *RGBTuple;

   if (cidx < 0) {	/* Handle "default color" */
      return PyString_FromString("Default");
   }

   for (i = 0; i < number_colors; i++) {
      if (cidx == colorlist[i].color.pixel) {
	 RGBTuple = PyTuple_New(3);
	 PyTuple_SetItem(RGBTuple, 0,
		PyInt_FromLong((long)(colorlist[i].color.red / 256)));
	 PyTuple_SetItem(RGBTuple, 1,
		PyInt_FromLong((long)(colorlist[i].color.green / 256)));
	 PyTuple_SetItem(RGBTuple, 2,
		PyInt_FromLong((long)(colorlist[i].color.blue / 256)));
	 return RGBTuple;
      }
   }
   PyErr_SetString(PyExc_TypeError, "invalid or unknown color index");
   return NULL;
}

/*--------------------------------------------------------------*/
/* Convert color 3-tuple (RGB) or name to a color index		*/
/*--------------------------------------------------------------*/

int PyRGBToIndex(PyObject *cobj)
{
   PyObject *tobj;
   int ccolor, r, g, b;

   if (PyTuple_Check(cobj) && PyTuple_Size(cobj) == 3) {  /* RGB 3-tuple */
      tobj = PyTuple_GetItem(cobj, 0);
      if (PyFloat_Check(tobj)) {
         r = (int)(65535 * (float)PyFloat_AsDouble(tobj));
         g = (int)(65535 * (float)PyFloat_AsDouble(PyTuple_GetItem(cobj, 1)));
         b = (int)(65535 * (float)PyFloat_AsDouble(PyTuple_GetItem(cobj, 2)));
      }
      else if (PyInt_Check(tobj)) {
         r = (int)(256 * PyInt_AsLong(tobj));
         g = (int)(256 * PyInt_AsLong(PyTuple_GetItem(cobj, 1)));
         b = (int)(256 * PyInt_AsLong(PyTuple_GetItem(cobj, 2)));
      }
      else {
         PyErr_SetString(PyExc_TypeError, "tuple components must be integer or float");
         return -1;
      }
      ccolor = rgb_alloccolor(r, g, b);
   }
   else if (PyString_Check(cobj)) {  /* color name */
      ccolor = xc_alloccolor(PyString_AsString(cobj));
   }
   else if (PyInt_Check(cobj)) {  /* index (for backward compatibility) */
      ccolor = (int)PyInt_AsLong(cobj);
   }
   else {
      PyErr_SetString(PyExc_TypeError, "argument must be a string or 3-tuple");
      return -1;
   }

   addnewcolorentry(ccolor);
   return ccolor;
}

/*--------------------------------------------------------------*/
/* Convert a Python list to a stringpart* 			*/
/*--------------------------------------------------------------*/

stringpart *PySetStringParts(PyObject *dval)
{
   PyObject *lstr, *litem, *ditem, *sitem, *titem;
   int i, j, llen;
   stringpart *strptr, *newpart;
   char *string;
   
   /* If we pass a string, create a list of size 1 to hold the string.	*/
   /* Otherwise, the argument must be a list.				*/

   if (PyList_Check(dval)) {
      lstr = dval;
   }
   else {
      if (PyString_Check(dval)) {
         lstr = PyList_New(1);
	 PyList_SetItem(lstr, 0, dval);
      }
      else {
         PyErr_SetString(PyExc_TypeError, "argument must be a string or a list");
	 return NULL;
      }
   }

   strptr = NULL;
   llen = PyList_Size(lstr);
   for (i = 0; i < llen; i++) {
      newpart = makesegment(&strptr, NULL);
      newpart->nextpart = NULL;

      litem = PyList_GetItem(lstr, i);
      if (PyDict_Check(litem)) {
	 if ((ditem = PyDict_GetItemString(litem, "Parameter")) != NULL) {
	    newpart->type = PARAM_START;
	    newpart->data.string = strdup(PyString_AsString(ditem));
	 }
	 else if ((ditem = PyDict_GetItemString(litem, "Font")) != NULL) {
	    newpart->type = FONT_NAME;
	    string = PyString_AsString(ditem);
	    for (j = 0; j < fontcount; j++)
	       if (!strcmp(fonts[j].psname, string)) break;
	    if (j == fontcount) loadfontfile(string);
	    newpart->data.font = j;
	 }
	 else if ((ditem = PyDict_GetItemString(litem, "Kern")) != NULL) {
	    newpart->type = KERN;
	    newpart->data.kern[0] = (int)PyInt_AsLong(PyTuple_GetItem(ditem, 0));
	    newpart->data.kern[1] = (int)PyInt_AsLong(PyTuple_GetItem(ditem, 1));
	 }
	 else if ((ditem = PyDict_GetItemString(litem, "Color")) != NULL) {
	    newpart->type = FONT_COLOR;
	    newpart->data.color = PyRGBToIndex(ditem);
	 }
	 else if ((ditem = PyDict_GetItemString(litem, "Font Scale")) != NULL) {
	    newpart->type = FONT_SCALE;
	    newpart->data.scale = (float)PyFloat_AsDouble(ditem);
	 }
	 else if ((ditem = PyDict_GetItemString(litem, "Text")) != NULL) {
	    newpart->type = TEXT_STRING;
	    newpart->data.string = strdup(PyString_AsString(ditem));
	 }
      }
      else if (PyString_Check(litem)) {
	 string = PyString_AsString(litem);
	 if (!strcmp(string, "End Parameter")) {
	    newpart->type = PARAM_END;
	    newpart->data.string = (u_char *)NULL;
	 }
	 else if (!strcmp(string, "Tab Stop")) {
	    newpart->type = TABSTOP;
	 }
	 else if (!strcmp(string, "Tab Forward")) {
	    newpart->type = TABFORWARD;
	 }
	 else if (!strcmp(string, "Tab Backward")) {
	    newpart->type = TABBACKWARD;
	 }
	 else if (!strcmp(string, "Return")) {
	    newpart->type = RETURN;
	 }
	 else if (!strcmp(string, "Subscript")) {
	    newpart->type = SUBSCRIPT;
	 }
	 else if (!strcmp(string, "Superscript")) {
	    newpart->type = SUPERSCRIPT;
	 }
	 else if (!strcmp(string, "Normalscript")) {
	    newpart->type = NORMALSCRIPT;
	 }
	 else if (!strcmp(string, "Underline")) {
	    newpart->type = UNDERLINE;
	 }
	 else if (!strcmp(string, "Overline")) {
	    newpart->type = OVERLINE;
	 }
	 else if (!strcmp(string, "No Line")) {
	    newpart->type = NOLINE;
	 }
	 else if (!strcmp(string, "Half Space")) {
	    newpart->type = HALFSPACE;
	 }
	 else if (!strcmp(string, "Quarter Space")) {
	    newpart->type = QTRSPACE;
	 }
	 else {
	    newpart->type = TEXT_STRING;
	    newpart->data.string = strdup(string);
	 }
      }
   }
   return strptr;
}

/*--------------------------------------------------------------*/
/* Convert a stringpart* to a Python list 			*/
/*--------------------------------------------------------------*/

PyObject *PyGetStringParts(stringpart *thisstring)
{
   PyObject *lstr, *sdict, *stup;
   int i, llen;
   stringpart *strptr;
   
   llen = stringparts(thisstring);
   lstr = PyList_New(llen);
   for (strptr = thisstring, i = 0; strptr != NULL;
      strptr = strptr->nextpart, i++) {
      switch(strptr->type) {
	 case TEXT_STRING:
	    sdict = PyDict_New();
	    PyDict_SetItem(sdict, PyString_FromString("Text"),
		  PyString_FromString(strptr->data.string));
	    PyList_SetItem(lstr, i, sdict);
	    break;
	 case PARAM_START:
	    sdict = PyDict_New();
	    PyDict_SetItem(sdict, PyString_FromString("Parameter"),
		  PyString_FromString(strptr->data.string));
	    PyList_SetItem(lstr, i, sdict);
	    break;
	 case PARAM_END:
	    PyList_SetItem(lstr, i, PyString_FromString("End Parameter"));
	    break;
	 case FONT_NAME:
	    sdict = PyDict_New();
	    PyDict_SetItem(sdict, PyString_FromString("Font"),
		  PyString_FromString(fonts[strptr->data.font].psname));
	    PyList_SetItem(lstr, i, sdict);
	    break;
	 case FONT_SCALE:
	    sdict = PyDict_New();
	    PyDict_SetItem(sdict, PyString_FromString("Font Scale"),
		  PyFloat_FromDouble((double)strptr->data.scale));
	    PyList_SetItem(lstr, i, sdict);
	    break;
	 case KERN:
	    sdict = PyDict_New();
	    stup = PyTuple_New(2);
	    PyTuple_SetItem(stup, 0, PyInt_FromLong((long)strptr->data.kern[0]));
	    PyTuple_SetItem(stup, 1, PyInt_FromLong((long)strptr->data.kern[1]));
	    PyDict_SetItem(sdict, PyString_FromString("Kern"), stup);
	    PyList_SetItem(lstr, i, sdict);
	    break;
	 case FONT_COLOR:
	    stup = PyIndexToRGB(strptr->data.color);
	    if (stup != NULL) {
	       sdict = PyDict_New();
	       PyDict_SetItem(sdict, PyString_FromString("Color"), stup); 
	       PyList_SetItem(lstr, i, sdict);
	    }
	    break;
	 case TABSTOP:
	    PyList_SetItem(lstr, i, PyString_FromString("Tab Stop"));
	    break;
	 case TABFORWARD:
	    PyList_SetItem(lstr, i, PyString_FromString("Tab Forward"));
	    break;
	 case TABBACKWARD:
	    PyList_SetItem(lstr, i, PyString_FromString("Tab Backward"));
	    break;
	 case RETURN:
	    PyList_SetItem(lstr, i, PyString_FromString("Return"));
	    break;
	 case SUBSCRIPT:
	    PyList_SetItem(lstr, i, PyString_FromString("Subscript"));
	    break;
	 case SUPERSCRIPT:
	    PyList_SetItem(lstr, i, PyString_FromString("Superscript"));
	    break;
	 case NORMALSCRIPT:
	    PyList_SetItem(lstr, i, PyString_FromString("Normalscript"));
	    break;
	 case UNDERLINE:
	    PyList_SetItem(lstr, i, PyString_FromString("Underline"));
	    break;
	 case OVERLINE:
	    PyList_SetItem(lstr, i, PyString_FromString("Overline"));
	    break;
	 case NOLINE:
	    PyList_SetItem(lstr, i, PyString_FromString("No Line"));
	    break;
	 case HALFSPACE:
	    PyList_SetItem(lstr, i, PyString_FromString("Half Space"));
	    break;
	 case QTRSPACE:
	    PyList_SetItem(lstr, i, PyString_FromString("Quarter Space"));
	    break;
      }
   }
   return lstr;
}

/*--------------------------------------------------------------*/
/* Check if the handle (integer) is an existing element		*/
/*--------------------------------------------------------------*/

genericptr *CheckHandle(PyObject *ehandle)
{
   genericptr *gelem;
   int i, j;
   long int eaddr;
   objectptr thisobj;
   Library *thislib;

   eaddr = PyInt_AsLong(ehandle);
   for (gelem = topobject->plist; gelem < topobject->plist +
	topobject->parts; gelem++)
      if ((long int)(*gelem) == eaddr) goto exists;

   /* Okay, it isn't in topobject.  Try all other pages. */

   for (i = 0; i < xobjs.pages; i++) {
      if (xobjs.pagelist[i]->pageinst == NULL) continue;
      thisobj = xobjs.pagelist[i]->pageinst->thisobject;
      for (gelem = thisobj->plist; gelem < thisobj->plist + thisobj->parts; gelem++)
         if ((long int)(*gelem) == eaddr) goto exists;
   }

   /* Still not found?  Maybe in a library */

   for (i = 0; i < xobjs.numlibs; i++) {
      thislib = xobjs.userlibs + i;
      for (j = 0; j < thislib->number; j++) {
         thisobj = thislib->library[j];
         for (gelem = thisobj->plist; gelem < thisobj->plist + thisobj->parts; gelem++)
            if ((long int)(*gelem) == eaddr) goto exists;
      }
   }

   /* Either in the delete list (where we don't want to go) or	*/
   /* is an invalid number.					*/
   return NULL;

exists:
   return gelem;
}

/*--------------------------------------------------------------*/
/* Check if the handle (integer) is an existing page		*/
/*--------------------------------------------------------------*/

objectptr CheckPageHandle(PyObject *ehandle)
{
   int pageno;
   long int eaddr;

   eaddr = PyInt_AsLong(ehandle);
   pageno = is_page((objectptr)eaddr);
   if (pageno < 0) return NULL;
   return (objectptr)eaddr;
}

/*--------------------------------------------------------------*/
/* Form a 2-tuple from a pair of integers.			*/
/*--------------------------------------------------------------*/

static PyObject *make_pair(XPoint *thispoint)
{
   PyObject *dtup;

   dtup = PyTuple_New(2);

   PyTuple_SetItem(dtup, 0,
		PyInt_FromLong((long)(thispoint->x)));
   PyTuple_SetItem(dtup, 1,
		PyInt_FromLong((long)(thispoint->y)));
   return dtup;
}

/*--------------------------------------------------------------*/
/* Get the properties of a page (returned as a dictionary)	*/
/*--------------------------------------------------------------*/

static PyObject *xc_getpage(PyObject *self, PyObject *args)
{
   Pagedata *thispage;
   PyObject *rdict, *dlist;
   int pageno = areawin->page + 1, i;

   if (!PyArg_ParseTuple(args, "|d:getpage", &pageno))
      return NULL;

   if (pageno <= 0 || pageno > xobjs.pages) return NULL;
   else pageno--;

   thispage = xobjs.pagelist[pageno];

   rdict = PyDict_New();

   PyDict_SetItem(rdict, PyString_FromString("filename"),
	PyString_FromString(thispage->filename));

   PyDict_SetItem(rdict, PyString_FromString("page label"),
	PyString_FromString(thispage->pageinst->thisobject->name));

   PyDict_SetItem(rdict, PyString_FromString("output scale"),
	PyFloat_FromDouble((double)thispage->outscale));

   /* To be done:  change these from internal units to "natural" units */

   PyDict_SetItem(rdict, PyString_FromString("grid space"),
	PyFloat_FromDouble((double)thispage->gridspace));

   PyDict_SetItem(rdict, PyString_FromString("snap space"),
	PyFloat_FromDouble((double)thispage->snapspace));

   PyDict_SetItem(rdict, PyString_FromString("orientation"),
	PyInt_FromLong((long)thispage->orient));

   PyDict_SetItem(rdict, PyString_FromString("output mode"),
	PyInt_FromLong((long)thispage->pmode));

   PyDict_SetItem(rdict, PyString_FromString("coordinate style"),
	PyInt_FromLong((long)thispage->coordstyle));

   PyDict_SetItem(rdict, PyString_FromString("page size"),
	make_pair(&(thispage->pagesize)));

   PyDict_SetItem(rdict, PyString_FromString("drawing scale"),
	make_pair(&(thispage->drawingscale)));

   return rdict;
}

/*--------------------------------------------------------------*/
/* Get the properties of a library (returned as a dictionary)	*/
/*--------------------------------------------------------------*/

static PyObject *xc_getlibrary(PyObject *self, PyObject *args)
{
   PyObject *rdict, *dlist;
   const char *lname = NULL;
   objectptr *curlib = NULL, thisobj;
   char *errptr;
   int i, lpage;
   int curobjs;

   if (!PyArg_ParseTuple(args, "s:getlibrary", &lname))
      return NULL;

   lpage = strtol(lname, &errptr, 10);
   if (*lname != '\0' && *errptr == '\0' && lpage <= xobjs.numlibs &&
		lpage > 0) {	/* numerical */
      curlib = xobjs.userlibs[lpage - 1].library;
   }
   else {
      for (lpage = 0; lpage < xobjs.numlibs; lpage++) {
	 if (!strcmp(lname, xobjs.libtop[lpage + LIBRARY]->thisobject->name)) {
	    curlib = xobjs.userlibs[lpage - 1].library;
	    break;
	 }
	 else if (!strncmp(xobjs.libtop[lpage + LIBRARY]->thisobject->name,
			"Library: ", 9)) {
	    if (!strcmp(lname, xobjs.libtop[lpage + LIBRARY]->thisobject->name + 9)) {
	       curlib = xobjs.userlibs[lpage - 1].library;
	       break;
	    }
	 }
      }
   }
 
   if (curlib == NULL) {
      PyErr_SetString(PyExc_TypeError,
		"getlibrary() 2nd argument must be a library page or name");
      return NULL;
   }
   curobjs = xobjs.userlibs[lpage - 1].number;

   /* Argument to getlibrary() can be a library name or library page # */

   rdict = PyDict_New();

   PyDict_SetItem(rdict, PyString_FromString("name"),
	PyString_FromString(xobjs.libtop[lpage + LIBRARY]->thisobject->name));

   if (curobjs > 0) {
      dlist = PyList_New(curobjs);
      for (i = 0; i < curobjs; i++) {
	 thisobj = *(curlib + i);
         PyList_SetItem(dlist, i, PyString_FromString(thisobj->name));
      }
      PyDict_SetItem(rdict, PyString_FromString("objects"), dlist);
   }
   return rdict;
}

/*--------------------------------------------------------------*/
/* Get the properties of an object (returned as a dictionary)	*/
/*--------------------------------------------------------------*/

static PyObject *xc_getobject(PyObject *self, PyObject *args)
{
   objectptr thisobj = NULL, *libobj;
   oparamptr ops;
   PyObject *rdict, *dlist, *tpos;
   const char *oname;
   int i, j, k, nparam;

   if (!PyArg_ParseTuple(args, "s:getobject", &oname))
      return NULL;

   for (k = 0; k < xobjs.numlibs; k++) { 
      for (j = 0; j < xobjs.userlibs[k].number; j++) {
	 libobj = xobjs.userlibs[k].library + j;
	 if (!strcmp(oname, (*libobj)->name)) {
	    thisobj = *libobj;
	    break;
	 }
      }
      if (thisobj != NULL) break;
   }

   if (thisobj == NULL) { 	/* try the page objects */
      for (k = 0; k < xobjs.pages; k++) {
	 if (xobjs.pagelist[k]->pageinst != NULL)
	    if (!strcmp(oname, xobjs.pagelist[k]->pageinst->thisobject->name))
	       break;
      }
      if (k == xobjs.pages) return NULL;	/* not found */
   }

   /* return all the object's properties as a dictionary */

   rdict = PyDict_New();

   PyDict_SetItem(rdict, PyString_FromString("name"),
	PyString_FromString(thisobj->name));
   PyDict_SetItem(rdict, PyString_FromString("width"), 
	PyInt_FromLong((long)(thisobj->bbox.width)));
   PyDict_SetItem(rdict, PyString_FromString("height"), 
	PyInt_FromLong((long)(thisobj->bbox.height)));
   PyDict_SetItem(rdict, PyString_FromString("viewscale"), 
	PyFloat_FromDouble((double)(thisobj->viewscale)));

   tpos = PyTuple_New(2);
   PyTuple_SetItem(tpos, 0, PyInt_FromLong((long)thisobj->bbox.lowerleft.x));
   PyTuple_SetItem(tpos, 1, PyInt_FromLong((long)thisobj->bbox.lowerleft.y));
   PyDict_SetItem(rdict, PyString_FromString("boundingbox"), tpos);

   tpos = PyTuple_New(2);
   PyTuple_SetItem(tpos, 0, PyInt_FromLong((long)thisobj->pcorner.x));
   PyTuple_SetItem(tpos, 1, PyInt_FromLong((long)thisobj->pcorner.y));
   PyDict_SetItem(rdict, PyString_FromString("viewcorner"), tpos);

   dlist = PyList_New(thisobj->parts);
   for (i = 0; i < thisobj->parts; i++)
      PyList_SetItem(dlist, i, PyInt_FromLong((long)(*(thisobj->plist + i))));
   PyDict_SetItem(rdict, PyString_FromString("parts"), dlist);

   nparam = get_num_params(thisobj);
   if (nparam > 0) {
      dlist = PyList_New(nparam);
      i = 0;
      for (ops = thisobj->params; ops != NULL; ops = ops->next) {
	 i++;
	 if (ops->type == XC_INT)
            PyList_SetItem(dlist, i, PyInt_FromLong((long)(ops->parameter.ivalue)));
	 else if (ops->type == XC_FLOAT)
            PyList_SetItem(dlist, i,
		PyFloat_FromDouble((double)(ops->parameter.fvalue)));
	 else if (ops->type == XC_STRING)
            PyList_SetItem(dlist, i, PyGetStringParts(ops->parameter.string));
      }
      PyDict_SetItem(rdict, PyString_FromString("parameters"), dlist);
   }
   return rdict;
}

/*--------------------------------------------------------------*/
/* Reset the current page					*/
/*--------------------------------------------------------------*/

static PyObject *xc_reset()
{
   resetbutton(NULL, (pointertype)0, NULL);
   return PyInt_FromLong(0L);
}

/*--------------------------------------------------------------*/
/* Get a netlist						*/
/*--------------------------------------------------------------*/

extern PyObject *pyglobals(objectptr);
extern PyObject *pytoplevel(objectptr);

static PyObject *xc_netlist()
{
   PyObject *rdict = NULL;

   if (updatenets(areawin->topinstance, FALSE) <= 0) {
      PyErr_SetString(PyExc_TypeError, "Error:  Check circuit for infinite recursion.");
      return NULL;
   }

   rdict = PyDict_New();
   PyDict_SetItem(rdict, PyString_FromString("globals"), pyglobals(topobject));
   PyDict_SetItem(rdict, PyString_FromString("circuit"), pytoplevel(topobject));
   return rdict;
}

/*--------------------------------------------------------------*/
/* Load the specified file					*/
/*--------------------------------------------------------------*/

static PyObject *xc_load(PyObject *self, PyObject *args)
{
   char *filename;
   int pageno = 0, savepage;

   if (!PyArg_ParseTuple(args, "s|i:load", &filename, &pageno))
      return PyInt_FromLong((long)areawin->page);

   if (--pageno < 0) pageno = areawin->page;
   savepage = areawin->page;

   strcpy(_STR2, filename);
   if (savepage != pageno) newpage(pageno);
   startloadfile(-1);
   if (savepage != pageno) newpage(savepage);

   return PyString_FromString(filename);
}

/*--------------------------------------------------------------*/
/* Go to the specified page					*/
/*--------------------------------------------------------------*/

static PyObject *xc_page(PyObject *self, PyObject *args)
{
   int pageno;

   if (!PyArg_ParseTuple(args, "i:page", &pageno))
      return PyInt_FromLong((long)areawin->page);

   newpage(pageno - 1);
   return PyInt_FromLong((long)pageno);
}

/*--------------------------------------------------------------*/
/* Zoom								*/
/*--------------------------------------------------------------*/

static PyObject *xc_zoom(PyObject *self, PyObject *args)
{
   float factor, save;

   if (!PyArg_ParseTuple(args, "f:zoom", &factor))
      return NULL;

   if (factor <= 0) {
      PyErr_SetString(PyExc_TypeError, "Argument must be positive.");
      return NULL;
   }
   
   save = areawin->zoomfactor;

   if (factor < 1.0) {
      areawin->zoomfactor = 1.0 / factor;
      zoomout(0, 0);	/* Needs to be fixed---give x, y for drag fn */
   }
   else {
      areawin->zoomfactor = factor;
      zoomin(0, 0);	/* Needs to be fixed---see above */
   }

   areawin->zoomfactor = save;
   return PyFloat_FromDouble((double)topobject->viewscale);
}

/*--------------------------------------------------------------*/
/* Pan								*/
/*--------------------------------------------------------------*/

static PyObject *xc_pan(PyObject *self, PyObject *args)
{
   PyObject *cornerpos, *xobj, *yobj = NULL, *pval;
   int x, y;
   XPoint upoint, wpoint;

   if (!PyArg_ParseTuple(args, "O|O:pan", &xobj, &yobj))
      return NULL;

   if (yobj == NULL) {
      if (PyTuple_Check(xobj) && PyTuple_Size(xobj) == 2) {
         if ((pval = PyTuple_GetItem(xobj, 0)) != NULL)
            upoint.x = PyInt_AsLong(pval);
         if ((pval = PyTuple_GetItem(xobj, 1)) != NULL)
            upoint.y = PyInt_AsLong(pval);
      }
      else return NULL;
   }
   else {
      upoint.x = PyInt_AsLong(xobj);
      upoint.y = PyInt_AsLong(yobj);
   }

   user_to_window(upoint, &wpoint);
   panbutton((u_int)5, wpoint.x, wpoint.y, 0.33);  /* fixed fraction */

   cornerpos = PyTuple_New(2);
   PyTuple_SetItem(cornerpos, 0, PyInt_FromLong((long)areawin->pcorner.x));
   PyTuple_SetItem(cornerpos, 1, PyInt_FromLong((long)areawin->pcorner.y));

   return cornerpos;
}

/*--------------------------------------------------------------*/
/* Get the window size						*/
/*--------------------------------------------------------------*/

static PyObject *xc_getwindow()
{
   PyObject *windowsize;

   windowsize = PyTuple_New(2);
   PyTuple_SetItem(windowsize, 0, PyInt_FromLong((long)areawin->width));
   PyTuple_SetItem(windowsize, 1, PyInt_FromLong((long)areawin->height));

   return windowsize;
}

/*--------------------------------------------------------------*/
/* Get the cursor position					*/
/*--------------------------------------------------------------*/

static PyObject *xc_getcursor()
{
   PyObject *cursorpos;
   XPoint newpos;

   newpos = UGetCursorPos();
   u2u_snap(&newpos);
   
   cursorpos = PyTuple_New(2);
   PyTuple_SetItem(cursorpos, 0, PyInt_FromLong((long)newpos.x));
   PyTuple_SetItem(cursorpos, 1, PyInt_FromLong((long)newpos.y));

   return cursorpos;
}

/*--------------------------------------------------------------*/
/* Get the properties of an element (returned as a dictionary)	*/
/*--------------------------------------------------------------*/

static PyObject *xc_getattr(PyObject *self, PyObject *args)
{
   genericptr *gelem;
   PyObject *ehandle, *rdict, *dlist, *lstr, *sdict, *stup;
   int i, llen;
   char *tstr;
   stringpart *strptr;
   

   if (!PyArg_ParseTuple(args, "O:getattr", &ehandle))
      return NULL;

   /* Check to make sure that handle exists! */
   if ((gelem = CheckHandle(ehandle)) == NULL) {
      PyErr_SetString(PyExc_TypeError,
		"Argument must be a valid handle to an element.");
      return NULL;
   }

   /* return the element's properties as a dictionary	*/

   rdict = PyDict_New();
   tstr = type_to_string((*gelem)->type);
   if (tstr == NULL) {
      PyErr_SetString(PyExc_TypeError,
		"Element type is unknown.");
      return NULL;
   }
   PyDict_SetItem(rdict, PyString_FromString("type"),
        PyString_FromString(tstr));
   free(tstr);
   lstr = PyIndexToRGB((*gelem)->color);
   if (lstr != NULL)
      PyDict_SetItem(rdict, PyString_FromString("color"), lstr);

   switch(ELEMENTTYPE(*gelem)) {
      case LABEL:
	 PyDict_SetItem(rdict, PyString_FromString("position"), 
		make_pair(&(TOLABEL(gelem)->position)));
	 PyDict_SetItem(rdict, PyString_FromString("rotation"),
		PyFloat_FromDouble((double)TOLABEL(gelem)->rotation));
	 PyDict_SetItem(rdict, PyString_FromString("scale"),
		PyFloat_FromDouble((double)TOLABEL(gelem)->scale));
	 PyDict_SetItem(rdict, PyString_FromString("anchor"),
		PyInt_FromLong((long)TOLABEL(gelem)->anchor));
	 PyDict_SetItem(rdict, PyString_FromString("pin"),
		PyInt_FromLong((long)TOLABEL(gelem)->pin));
	 lstr = PyGetStringParts(TOLABEL(gelem)->string);

	 PyDict_SetItem(rdict, PyString_FromString("string"), lstr);
	 break;
      case POLYGON:
	 PyDict_SetItem(rdict, PyString_FromString("style"), 
		PyInt_FromLong((long)TOPOLY(gelem)->style));
	 PyDict_SetItem(rdict, PyString_FromString("linewidth"),
		PyFloat_FromDouble((double)TOPOLY(gelem)->width));
	 dlist = PyList_New(TOPOLY(gelem)->number);
	 for (i = 0; i < TOPOLY(gelem)->number; i++) {
	    PyList_SetItem(dlist, i, make_pair(&(TOPOLY(gelem)->points[i])));
	 }
	 PyDict_SetItem(rdict, PyString_FromString("points"), dlist);
	 break;
      case ARC:
	 PyDict_SetItem(rdict, PyString_FromString("style"), 
		PyInt_FromLong((long)TOARC(gelem)->style));
	 PyDict_SetItem(rdict, PyString_FromString("linewidth"),
		PyFloat_FromDouble((double)TOARC(gelem)->width));
	 PyDict_SetItem(rdict, PyString_FromString("radius"), 
		PyInt_FromLong((long)TOARC(gelem)->radius));
	 PyDict_SetItem(rdict, PyString_FromString("minor axis"), 
		PyInt_FromLong((long)TOARC(gelem)->yaxis));
	 PyDict_SetItem(rdict, PyString_FromString("start angle"),
		PyFloat_FromDouble((double)TOARC(gelem)->angle1));
	 PyDict_SetItem(rdict, PyString_FromString("end angle"),
		PyFloat_FromDouble((double)TOARC(gelem)->angle2));
	 PyDict_SetItem(rdict, PyString_FromString("position"),
		make_pair(&(TOARC(gelem)->position)));
	 break;
      case SPLINE:
	 PyDict_SetItem(rdict, PyString_FromString("style"), 
		PyInt_FromLong((long)TOSPLINE(gelem)->style));
	 PyDict_SetItem(rdict, PyString_FromString("linewidth"),
		PyFloat_FromDouble((double)TOSPLINE(gelem)->width));
	 dlist = PyList_New(4);
	 for (i = 0; i < 4; i++) {
	    PyList_SetItem(dlist, i, make_pair(&(TOSPLINE(gelem)->ctrl[i])));
	 }
	 PyDict_SetItem(rdict, PyString_FromString("control points"), dlist);
	 break;
      case PATH:
	 PyDict_SetItem(rdict, PyString_FromString("style"), 
		PyInt_FromLong((long)TOPATH(gelem)->style));
	 PyDict_SetItem(rdict, PyString_FromString("linewidth"),
		PyFloat_FromDouble((double)TOPATH(gelem)->width));
	 dlist = PyList_New(TOPATH(gelem)->parts);
	 for (i = 0; i < TOPATH(gelem)->parts; i++) {
	    PyList_SetItem(dlist, i,
		PyInt_FromLong((long)(*(TOPATH(gelem)->plist + i))));
	 }
	 PyDict_SetItem(rdict, PyString_FromString("parts"), dlist);
	 break;
      case OBJINST:
	 PyDict_SetItem(rdict, PyString_FromString("position"), 
		make_pair(&(TOOBJINST(gelem)->position)));
	 PyDict_SetItem(rdict, PyString_FromString("rotation"),
		PyFloat_FromDouble((double)TOOBJINST(gelem)->rotation));
	 PyDict_SetItem(rdict, PyString_FromString("scale"),
		PyFloat_FromDouble((double)TOOBJINST(gelem)->scale));
	 PyDict_SetItem(rdict, PyString_FromString("name"),
		PyString_FromString(TOOBJINST(gelem)->thisobject->name));
	 break;
   }
   return rdict;
}

/*--------------------------------------------------------------*/
/* Set properties of an element (supplied as a dictionary)	*/
/*--------------------------------------------------------------*/

static PyObject *xc_setattr(PyObject *self, PyObject *args)
{
   genericptr *gelem;
   PyObject *ehandle, *attrdict, *dval, *pval, *qval;
   int i;

   if (!PyArg_ParseTuple(args, "OO:setattr", &ehandle, &attrdict))
      return NULL;

   /* Check to make sure that handle exists! */
   if ((gelem = CheckHandle(ehandle)) == NULL) return NULL;

   /* Is the argument a dictionary? */
   if (!PyDict_Check(attrdict)) { 
      PyErr_SetString(PyExc_TypeError,
		"setatrr() 2nd argument must be a dictionary");
      return NULL;
   }

   /* First, make sure no attempt is made to change the object type. */

   if ((dval = PyDict_GetItemString(attrdict, "type")) != NULL) {
      int dtype = string_to_type(PyString_AsString(dval));
      if (dtype < 0) return NULL;
      if (dtype != ELEMENTTYPE(*gelem)) {
         PyErr_SetString(PyExc_TypeError,
		"Attempt to change the type of an object.");
         return NULL;
      }
   }

   /* Next, look for dictionary strings containing values which apply */
   /* to a number of different elements (position, color, etc.)	      */

   if ((dval = PyDict_GetItemString(attrdict, "color")) != NULL) {
      (*gelem)->color = (short)PyRGBToIndex(dval);
   }

   if ((dval = PyDict_GetItemString(attrdict, "position")) != NULL) {
      if (PyTuple_Check(dval) && PyTuple_Size(dval) == 2) {
         if ((pval = PyTuple_GetItem(dval, 0)) != NULL) {
            short xpos = (short)PyInt_AsLong(pval);
            switch(ELEMENTTYPE(*gelem)) {
	       case ARC:
	          TOARC(gelem)->position.x = xpos;
	          calcarc(TOARC(gelem));
	          break;
	       case LABEL:
	          TOLABEL(gelem)->position.x = xpos;
	          break;
	       case OBJINST:
	          TOOBJINST(gelem)->position.x = xpos;
	          break;
               default:
	          PyErr_SetString(PyExc_TypeError,
		      "attempt to set position on Spline, Polygon, or Path");
	    }
         }

         if ((pval = PyTuple_GetItem(dval, 1)) != NULL) {
            short ypos = (short)PyInt_AsLong(pval);
            switch(ELEMENTTYPE(*gelem)) {
	       case ARC:
	          TOARC(gelem)->position.y = ypos;
	          calcarc(TOARC(gelem));
	          break;
	       case LABEL:
	          TOLABEL(gelem)->position.y = ypos;
	          break;
	       case OBJINST:
	          TOOBJINST(gelem)->position.y = ypos;
	          break;
               default:
	          PyErr_SetString(PyExc_TypeError,
		      "attempt to set position on Spline, Polygon, or Path");
	    }
         }
      }
      else {
         PyErr_SetString(PyExc_TypeError,
		"position must be a tuple containing two integer values");
      }
   }

   if ((dval = PyDict_GetItemString(attrdict, "style")) != NULL) {
      short dstyle = (short)PyInt_AsLong(dval);
      switch(ELEMENTTYPE(*gelem)) {
         case POLYGON:
	    TOPOLY(gelem)->style = dstyle;
	    break;
         case PATH:
	    TOPATH(gelem)->style = dstyle;
	    break;
         case ARC:
	    TOARC(gelem)->style = dstyle;
	    break;
         case SPLINE:
	    TOSPLINE(gelem)->style = dstyle;
	    break;
         default:
            PyErr_SetString(PyExc_TypeError,
		"attempt to set style on an Object Instance or Label");
      }
   }

   if ((dval = PyDict_GetItemString(attrdict, "linewidth")) != NULL) {
      float dwidth = (float)PyFloat_AsDouble(dval);
      switch(ELEMENTTYPE(*gelem)) {
         case POLYGON:
	    TOPOLY(gelem)->width = dwidth;
	    break;
         case PATH:
	    TOPATH(gelem)->width = dwidth;
	    break;
         case ARC:
	    TOARC(gelem)->width = dwidth;
	    break;
         case SPLINE:
	    TOSPLINE(gelem)->width = dwidth;
	    break;
         default:
            PyErr_SetString(PyExc_TypeError,
		"attempt to set linewidth on an Object Instance or Label");
      }
   }

   if ((dval = PyDict_GetItemString(attrdict, "scale")) != NULL) {
      float dscale = (float)PyFloat_AsDouble(dval);
      switch(ELEMENTTYPE(*gelem)) {
         case LABEL:
	    TOLABEL(gelem)->scale = dscale;
	    break;
         case OBJINST:
	    TOOBJINST(gelem)->scale = dscale;
	    break;
         default:
            PyErr_SetString(PyExc_TypeError,
		"attempt to set scale on something not a Label or Object Instance");
      }
   }

   if ((dval = PyDict_GetItemString(attrdict, "rotation")) != NULL) {
      float drot = (float)PyFloat_AsDouble(dval);
      switch(ELEMENTTYPE(*gelem)) {
         case LABEL:
	    TOLABEL(gelem)->rotation = drot;
	    break;
         case OBJINST:
	    TOOBJINST(gelem)->rotation = drot;
	    break;
         default:
            PyErr_SetString(PyExc_TypeError,
		"attempt to set rotation on something not a Label or Object Instance");
      }
   }

   /* Dictionary entries specific to certain xcircuit types */

   switch(ELEMENTTYPE(*gelem)) {
      case LABEL:
	 if ((dval = PyDict_GetItemString(attrdict, "anchor")) != NULL) {
	    TOLABEL(gelem)->anchor = PyInt_AsLong(dval);
	 }
	 if ((dval = PyDict_GetItemString(attrdict, "string")) != NULL) {
	    freelabel(TOLABEL(gelem)->string);
	    TOLABEL(gelem)->string = PySetStringParts(dval);
	 }
	 if ((dval = PyDict_GetItemString(attrdict, "pin")) != NULL) {
	    TOLABEL(gelem)->pin = PyInt_AsLong(dval);
	 }
	 break;
      case ARC:
	 if ((dval = PyDict_GetItemString(attrdict, "start angle")) != NULL) {
	    TOARC(gelem)->angle1 = (float)PyFloat_AsDouble(dval);
	 }
	 if ((dval = PyDict_GetItemString(attrdict, "end angle")) != NULL) {
	    TOARC(gelem)->angle2 = (float)PyFloat_AsDouble(dval);
	 }
	 if ((dval = PyDict_GetItemString(attrdict, "radius")) != NULL) {
	    TOARC(gelem)->radius = PyInt_AsLong(dval);
	 }
	 if ((dval = PyDict_GetItemString(attrdict, "minor axis")) != NULL) {
	    TOARC(gelem)->yaxis = PyInt_AsLong(dval);
	 }
	 break;
      case SPLINE:
	 if ((dval = PyDict_GetItemString(attrdict, "control points")) != NULL) {
            if (PyList_Check(dval) && PyList_Size(dval) == 4) {
	       for (i = 0; i < 4; i++) {
		  pval = PyList_GetItem(dval, i);
		  if (PyTuple_Check(pval) && PyTuple_Size(pval) == 2) {
		     qval = PyTuple_GetItem(pval, 0);
		     TOSPLINE(gelem)->ctrl[i].x = (short)PyInt_AsLong(qval);
		     qval = PyTuple_GetItem(pval, 1);
		     TOSPLINE(gelem)->ctrl[i].y = (short)PyInt_AsLong(qval);
		  }
		  else {
                     PyErr_SetString(PyExc_TypeError,
				"must have a tuple of 2 values per point");
	             break;
		  }
	       }
	    }
	    else {
               PyErr_SetString(PyExc_TypeError,
			"must have 4 control points in a list");
	       break;
	    }
	 }
	 break;
      case POLYGON:
	 if ((dval = PyDict_GetItemString(attrdict, "points")) != NULL) {
            if (PyList_Check(dval)) {
	       int number = PyList_Size(dval);
	       if (TOPOLY(gelem)->number != number) {
		  TOPOLY(gelem)->points = (pointlist)realloc(
			TOPOLY(gelem)->points, number * sizeof(XPoint));
		  TOPOLY(gelem)->number = number;
	       }
	       for (i = 0; i < number; i++) {
		  pval = PyList_GetItem(dval, i);
		  if (PyTuple_Check(pval) && PyTuple_Size(pval) == 2) {
		     qval = PyTuple_GetItem(pval, 0);
		     TOPOLY(gelem)->points[i].x = (short)PyInt_AsLong(qval);
		     qval = PyTuple_GetItem(pval, 1);
		     TOPOLY(gelem)->points[i].y = (short)PyInt_AsLong(qval);
		  }
		  else {
                     PyErr_SetString(PyExc_TypeError,
				"must have a tuple of 2 values per point");
	             break;
		  }
	       }
	    }
	    else {
               PyErr_SetString(PyExc_TypeError,
			"points must be in a list of tuples");
	       break;
	    }
	 }
	 break;
   }

   calcbbox(areawin->topinstance);
   return PyInt_FromLong((long)(*gelem));
}
/*--------------------------------------------------------------*/
/* Set various options through the "set" command.		*/
/* "set <option> on|off" supercedes "enable|disable <option>"	*/
/*--------------------------------------------------------------*/

static PyObject *xc_set(PyObject *self, PyObject *args)
{
   const char *sarg1, *sarg2;
   int i;
   Boolean a = True; 
   short cpage = areawin->page;

   if (!PyArg_ParseTuple(args, "ss:set", &sarg1, &sarg2))
      return NULL;
   
   if (!strcmp(sarg2, "On") || !strcmp(sarg2, "on") || !strcmp(sarg2, "True")
	|| !strcmp(sarg2, "true"))
      a = False;     /* has to be backwards; toggle() inverts the value! */

   if (!strcmp(sarg1, "font")) {
      for (i = 0; i < fontcount; i++)
	 if (!strcmp(fonts[i].psname, sarg2)) break;

      if (i == fontcount)
	 loadfontfile((char *)sarg2);

      /* loadfontfile() may load multiple fonts, so we have to check again */
      /* to see which one matches.					   */

      for (i = 0; i < fontcount; i++) {
	 if (!strcmp(fonts[i].psname, sarg2)) {
            areawin->psfont = i;
            setdefaultfontmarks();
	    break;
	 }
      }
   }
   else if (!strcmp(sarg1, "fontscale")) {
      sscanf(sarg2, "%f", &areawin->textscale);
   }
   else if (!strcmp(sarg1, "axis") || !strcmp(sarg1, "axes")) {
      areawin->axeson = a;
      toggle(GridAxesButton, (pointertype)&areawin->axeson, NULL);
   }
   else if (!strcmp(sarg1, "grid")) {
      areawin->gridon = a;
      toggle(GridGridButton, (pointertype)&areawin->gridon, NULL);
   }  
   else if (!strcmp(sarg1, "snap") || !strcmp(sarg1, "snap-to")) {
      areawin->snapto = a;
      toggle(SnaptoSnaptoButton, (pointertype)&areawin->snapto, NULL);
   }
   else if (!strcmp(sarg1, "gridspace")) {
      sscanf(sarg2, "%f", &xobjs.pagelist[cpage]->gridspace);
   }
   else if (!strcmp(sarg1, "snapspace")) {
      sscanf(sarg2, "%f", &xobjs.pagelist[cpage]->snapspace);
   }
   else if (!strcmp(sarg1, "pagestyle")) {
      if (!strcmp(sarg2, "encapsulated") || !strcmp(sarg2, "eps"))
	 xobjs.pagelist[cpage]->pmode = 0;
      else
	 xobjs.pagelist[cpage]->pmode = 1;
   }
   else if (!strcmp(sarg1, "boxedit")) {
      if (!strcmp(sarg2, "rhomboid-x")) boxedit(NULL, RHOMBOIDX, NULL);
      else if (!strcmp(sarg2, "rhomboid-y")) boxedit(NULL, RHOMBOIDY, NULL);
      else if (!strcmp(sarg2, "rhomboid-a")) boxedit(NULL, RHOMBOIDA, NULL);
      else if (!strcmp(sarg2, "manhattan")) boxedit(NULL, MANHATTAN, NULL);
      else if (!strcmp(sarg2, "normal")) boxedit(NULL, NORMAL, NULL);
   }
   else if (!strcmp(sarg1, "linewidth")) {
      sscanf(sarg2, "%f", &areawin->linewidth);
   }
   else if (!strcmp(sarg1, "colorscheme")) {
      if (!strcmp(sarg2, "inverse"))
         areawin->invert = False;
      inversecolor(NULL, (pointertype)&areawin->invert, NULL);
   }
   else if (!strcmp(sarg1, "coordstyle")) {
      if (!strcmp(sarg2, "cm") || !strcmp(sarg2, "centimeters")) {
         xobjs.pagelist[cpage]->coordstyle = CM;
         xobjs.pagelist[cpage]->pagesize.x = 595;  /* A4 size */
         xobjs.pagelist[cpage]->pagesize.y = 842;
         togglegrid((u_short)xobjs.pagelist[cpage]->coordstyle);
      }
   }
   else if (!strcmp(sarg1, "orient")) {   /* "orient" or "orientation" */
      if (!strcmp(sarg2, "landscape"))
         xobjs.pagelist[cpage]->orient = 90; /* Landscape */
      else
         xobjs.pagelist[cpage]->orient = 0;  /* Portrait */
   }

   else if (!strcmp(sarg1, "xschema") || !strcmp(sarg1, "schema")) {
      /* Do nothing---retained for backward compatibility only */
   }
#ifdef HAVE_XPM
   else if (!strcmp(sarg1, "toolbar")) {
      areawin->toolbar_on = a;
      dotoolbar(OptionsDisableToolbarButton, NULL, NULL);
    }
#endif

   return PyString_FromString(sarg2);
}

/*--------------------------------------------------------------*/

static PyObject *xc_override(PyObject *self, PyObject *args)
{
   const char *sarg1;

   if (!PyArg_ParseTuple(args, "s:override", &sarg1))
      return NULL;

   if (!strcmp(sarg1, "library") || !strcmp(sarg1, "libraries"))
      flags |= LIBOVERRIDE;
   else if (!strcmp(sarg1, "color") || !strcmp(sarg1, "colors"))
      flags |= COLOROVERRIDE;
   else if (!strcmp(sarg1, "font") || !strcmp(sarg1, "fonts"))
      flags |= FONTOVERRIDE;
   if (!strcmp(sarg1, "key") || !strcmp(sarg1, "keybindings"))
      flags |= KEYOVERRIDE;
   
   return PyInt_FromLong(0L);
}

/*--------------------------------------------------------------*/

static PyObject *xc_library(PyObject *self, PyObject *args)
{
   const char *libname;
   int libnum = 1;

   if (!PyArg_ParseTuple(args, "s|i:library", &libname, &libnum))
      return NULL;

   /* if loading of default libraries is not overridden, load them first */

   if (!(flags & (LIBOVERRIDE | LIBLOADED))) {
      defaultscript();
      flags |= LIBLOADED;   /* Pass through a Python variable? */
   }

   if (libnum >= xobjs.numlibs || libnum < 0)
      libnum = createlibrary(FALSE);
   else
      libnum += LIBRARY - 1;

   strcpy(_STR, libname);
   loadlibrary(libnum);
   return PyInt_FromLong((long)libnum);
}

/*--------------------------------------------------------------*/

static PyObject *xc_font(PyObject *self, PyObject *args)
{
   const char *fontname;

   if (!PyArg_ParseTuple(args, "s:font", &fontname))
      return NULL;

   if (!(flags & FONTOVERRIDE)) {
      loadfontfile("Helvetica");
      flags |= FONTOVERRIDE;
   }
   loadfontfile((char *)fontname);
   return PyString_FromString(fontname);
}

/*--------------------------------------------------------------*/

static PyObject *xc_color(PyObject *self, PyObject *args)
{
   PyObject *pcolor;
   int cidx;

   if (!PyArg_ParseTuple(args, "O:color", &pcolor))
      return NULL;

   cidx = PyRGBToIndex(pcolor);
   return PyIndexToRGB(cidx);
}

/*--------------------------------------------------------------*/

static PyObject *xc_bind(PyObject *self, PyObject *args)
{
   const char *keyname = NULL;
   const char *function = NULL;
   PyObject *retobj;
   int keywstate;

   if (!PyArg_ParseTuple(args, "|ss:bind", &keyname, &function))
      return NULL;

   /* If we're in .xcircuitrc and we're not overriding the key	    */
   /* bindings, then the first binding function should precipitate  */
   /* calling the defaults, then specify the bindings as overridden */
   if (!(flags & KEYOVERRIDE)) {
      default_keybindings();
      flags |= KEYOVERRIDE;
   }

   /* No arguments?  Return a dictionary of bound pairs */
   if (!keyname) {
      PyObject *key, *value;
      char *keyname, *funcname;
      keybinding *ksearch;

      retobj = PyDict_New();
      for (ksearch = keylist; ksearch != NULL; ksearch = ksearch->nextbinding) {
	 keyname = key_to_string(ksearch->keywstate);
	 if (ksearch->value >= 0) {
	    funcname = malloc(strlen(func_to_string(ksearch->function)) + 5);
	    sprintf(funcname, "%s %d", func_to_string(ksearch->function),
		ksearch->value);
	 }
	 else
	    funcname = func_to_string(ksearch->function);
	 key = PyString_FromString(keyname);
	 value = PyString_FromString(funcname);
	 PyDict_SetItem(retobj, key, value);
	 free(keyname);
	 if (ksearch->value >= 0) free(funcname);
      }
   }
   /* One argument?  Argument is a key or a function */
   else if (!function) {
      char *binding;
      int func = -1;
      keywstate = string_to_key(keyname);
      if (keywstate == 0) { /* first argument (keyname) is function? */
	 keywstate = -1;
	 func = string_to_func(keyname, NULL);
      }
      if (keywstate == -1)
         binding = function_binding_to_string(0, func);
      else
         binding = key_binding_to_string(0, keywstate);
      retobj = PyString_FromString(binding);
      free(binding);
   }
   else {
      if (add_keybinding(0, keyname, function) < 0) {
	 /* Function may be a Python function, not a C function */
         keywstate = string_to_key(keyname);
	 sprintf(_STR2, "keydict[%d] = %s\n", keywstate, function);
	 PyRun_SimpleString(_STR2);
      }
      retobj = PyString_FromString(keyname);
   }
   return retobj;
}

/*--------------------------------------------------------------*/

static PyObject *xc_unbind(PyObject *self, PyObject *args)
{
   const char *keyname = NULL;
   const char *function = NULL;

   if (!PyArg_ParseTuple(args, "ss:unbind", &keyname, &function))
      return NULL;

   /* If we're in .xcircuitrc and we're not overriding the key	    */
   /* bindings, then the first binding function should precipitate  */
   /* calling the defaults, then specify the bindings as overridden */
   if (!(flags & KEYOVERRIDE)) {
      default_keybindings();
      flags |= KEYOVERRIDE;
   }

   remove_keybinding(areawin->area, keyname, function);
   return PyString_FromString(keyname);
}

/*--------------------------------------------------------------*/
/* active delay 						*/
/*--------------------------------------------------------------*/
   
static PyObject *xc_pause(PyObject *self, PyObject *args)
{
   float delay;

   if (!PyArg_ParseTuple(args, "f:pause", &delay))
      return NULL;

   usleep((int)(1e6 * delay));
   return PyInt_FromLong((int)(1e6 * delay));
}

/*--------------------------------------------------------------*/
/* active refresh						*/
/*--------------------------------------------------------------*/
         
static PyObject *xc_refresh(PyObject *self, PyObject *args)
{
   XEvent event;

   if (!PyArg_ParseTuple(args, ":refresh"))
      return NULL;
      
   refresh(NULL, NULL, NULL);

   while (XCheckWindowEvent(dpy, areawin->window, ~NoEventMask, &event))
         XtDispatchEvent(&event);

   return PyInt_FromLong(0L);
}

/*--------------------------------------------------------------*/
/* Callback procedure for Python-generated buttons		*/
/*--------------------------------------------------------------*/

int pybutton(Widget w, caddr_t clientdata, caddr_t calldata)
{  
   int status;

   sprintf(_STR2, "buttondict[%ld]()\n", (long int)w);
   status = PyRun_SimpleString(_STR2);
   refresh(NULL, NULL, NULL);
   return status;
}

/*--------------------------------------------------------------*/
/* Add a new button to the specified (by name) cascade menu	*/
/*--------------------------------------------------------------*/

static PyObject *xc_newbutton(PyObject *self, PyObject *args)
{
   const char *pname = NULL;
   const char *newbname = NULL;
   const char *pfunction = NULL;
   int status;
   Arg	wargs[1];
   int  i, n = 0;
   Widget newbutton, cascade, tbutton;

   if (!PyArg_ParseTuple(args, "sss:newbutton", &pname, &newbname, &pfunction))
      return NULL;

   for (i = 0; i < MaxMenuWidgets; i++) {
      tbutton = menuwidgets[i];
      cascade = XtParent(tbutton);
      if (!strcmp(XtName(cascade), pname)) break;
   }
   if (i == MaxMenuWidgets) {
      Wprintf("Cannot find specified menu.");
      return NULL;
   }

   XtnSetArg(XtNfont, appdata.xcfont);
   newbutton = XtCreateWidget(newbname, XwmenubuttonWidgetClass,
	 cascade, wargs, n);

   XtAddCallback (newbutton, XtNselect, (XtCallbackProc)pybutton, newbutton);
   XtManageChild(newbutton);

   sprintf(_STR2, "buttondict[%ld] = %s\n", (long int)newbutton, pfunction);
   status = PyRun_SimpleString(_STR2);
   return PyInt_FromLong((long)status);
}

#ifdef HAVE_XPM

/*--------------------------------------------------------------*/
/* Callback procedure for Python-generated toolbar tool		*/
/*--------------------------------------------------------------*/

int pytool(Widget w, caddr_t clientdata, caddr_t calldata)
{  
   int status;

   sprintf(_STR2, "tooldict[%ld]()\n", (long int)w);
   status = PyRun_SimpleString(_STR2);
   refresh(NULL, NULL, NULL);
   return status;
}

/*--------------------------------------------------------------*/
/* Add a new tool to the toolbar				*/
/*--------------------------------------------------------------*/

static PyObject *xc_newtool(PyObject *self, PyObject *args)
{
   const char *thint = NULL;
   const char *pixfile = NULL;
   const char *newtname = NULL;
   const char *pfunction = NULL;
   int status;
   Arg	wargs[8];
   int  i, n = 0;
   Widget newtool, cascade, tbutton;
   XImage *iret;
   XpmAttributes attr;

   attr.valuemask = XpmSize | XpmCloseness;
   attr.closeness = 65536;

   if (!PyArg_ParseTuple(args, "sss|s:newtool", &newtname, &pfunction, &pixfile,
		&thint))
      return NULL;

   if (XpmReadFileToImage(dpy, (char *)pixfile, &iret, NULL, &attr) < 0) {
      Wprintf("Cannot find or open pixmap file \'%s\'", pixfile);
      XpmCreateImageFromData(dpy, q_xpm, &iret, NULL, &attr);
   }

   XtnSetArg(XtNlabelType, XwIMAGE);
   XtnSetArg(XtNlabelImage, iret);
   XtnSetArg(XtNwidth, attr.width + 4);
   XtnSetArg(XtNheight, attr.height + 4);
   XtnSetArg(XtNborderWidth, TBBORDER);
   XtnSetArg(XtNnoPad, True);
   if (thint != NULL) {
      XtnSetArg(XtNhint, thint);
      XtnSetArg(XtNhintProc, Wprintf);
   }
   newtool = XtCreateWidget(newtname, XwmenubuttonWidgetClass, toolbar, wargs, n);

   XtAddCallback (newtool, XtNselect, (XtCallbackProc)pybutton, newtool);
   XtManageChild(newtool);

   sprintf(_STR2, "tooldict[%ld] = %s\n", (long int)newtool, pfunction);
   status = PyRun_SimpleString(_STR2);
   return PyInt_FromLong((long)status);
}

/*------------------------------------------------------------------*/
/* Return a list of the widgets in tooldict			    */
/* (memory for list is allocated here and must be free'd elsewhere) */
/*------------------------------------------------------------------*/
 
Widget *pytoolbuttons(int *num_tools)
{
   Widget *ptools;
   PyObject *rval;
   int i;

   if (xcmod == NULL) {
      *num_tools = 0;
      return NULL;
   }
   rval = PyRun_ObjString("len(tooldict)");
   if (!rval) {
      PyErr_Clear();
      return NULL;
   }
   *num_tools = (int)PyInt_AsLong(rval);
   Py_DECREF(rval);
   /* fprintf(stderr, "num_tools = %d\n", *num_tools); */

   if (*num_tools <= 0) return NULL;

   ptools = (Widget *)malloc(*num_tools * sizeof(Widget));

   for (i = 0; i < *num_tools; i++) {
      sprintf(_STR2, "tooldict[%d]", i);
      rval = PyRun_ObjString(_STR2);
      if (!rval) {
         PyErr_Clear();
	 return NULL;
      }
      ptools[i] = (Widget)PyInt_AsLong(rval);
      Py_DECREF(rval);
   }
   return ptools;
}

#endif

/*----------------------------------------------------------------*/
/* Execute a python command from the key-function pair dictionary */
/*----------------------------------------------------------------*/

int python_key_command(int keystate)
{
   PyObject *rval;
   int status;
   sprintf(_STR2, "keydict[%d]\n", keystate);
   rval = PyRun_ObjString(_STR2);
   if (!rval) {
      PyErr_Clear();
      return -1;
   }
   Py_DECREF(rval);

   sprintf(_STR2, "keydict[%d]()\n", keystate);
   PyRun_SimpleString(_STR2);
   refresh(NULL, NULL, NULL);
   return 0;
}

/*--------------------------------------------------------------*/
/* Declaration of the xcircuit Python functions			*/
/*--------------------------------------------------------------*/

static PyMethodDef xc_methods[] = {
   {"set",		xc_set,		1},
   {"override",		xc_override,	1},
   {"library",		xc_library,	1},
   {"font",		xc_font,	1},
   {"color",		xc_color,	1},
   {"newelement",	xc_new,		1},
   {"getpage",		xc_getpage,	1},
   {"getlibrary",	xc_getlibrary,	1},
   {"getobject",	xc_getobject,	1},
   {"getattr",		xc_getattr,	1},
   {"setattr",		xc_setattr,	1},
   {"refresh",		xc_refresh,	1},
   {"pause",		xc_pause,	1},
   {"bind",		xc_bind,	1},
   {"unbind",		xc_unbind,	1},
   {"getcursor",	xc_getcursor,	1},
   {"getwindow",	xc_getwindow,	1},
   {"zoom",		xc_zoom,	1},
   {"pan",		xc_pan,		1},
   {"popupprompt",	xc_popupprompt, 1},
   {"filepopup",	xc_filepopup,	1},
   {"simplepopup",	xc_simplepopup,	1},
   {"newbutton",	xc_newbutton,	1},
   {"reset",		xc_reset,	1},
   {"page",		xc_page,	1},
   {"load",		xc_load,	1},
#ifdef HAVE_XPM
   {"newtool",		xc_newtool,	1},
#endif
   {"netlist",		xc_netlist,	1},
   {NULL,	NULL}			/* sentinel */
};

/*--------------------------------------------------------------*/
/* Initialize Python interpreter and load all xcircuit methods 	*/
/*--------------------------------------------------------------*/
void init_interpreter()
{
   Py_SetProgramName("XCircuit");
   Py_Initialize();
   PyImport_AddModule("xc");
   xcmod = Py_InitModule("xc", xc_methods);
   PyRun_SimpleString("from xc import *\n");
   PyRun_SimpleString("keydict = {}\n");	/* initialize key/function pairs */
   PyRun_SimpleString("buttondict = {}\n");  /* initialize button/function pairs */
   PyRun_SimpleString("tooldict = {}\n");  /* initialize tool/function pairs */
   sprintf(_STR, "xc_version = %2.1f\n", PROG_VERSION);
   PyRun_SimpleString(_STR);
}

/*--------------------------------------------------------------*/
/* Exit the Python interpreter					*/
/*--------------------------------------------------------------*/

void exit_interpreter()
{
   Py_Exit(0);
}

/*--------------------------------------------------------------*/
/* Replace the functions of the simple rcfile.c	interpreter.	*/
/*--------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Execute a single command from a script or from the command line      */
/*----------------------------------------------------------------------*/

short execcommand(short pflags, char *cmdptr)
{
   flags = pflags;
   PyRun_SimpleString(cmdptr);
   refresh(NULL, NULL, NULL);
   return flags;
}

/*----------------------------------------------------------------------*/
/* Load the default script (like execscript() but don't allow recursive */
/* loading of the startup script)                                       */
/*----------------------------------------------------------------------*/

void defaultscript()
{
   FILE *fd;
   char *tmp_s = getenv((const char *)"XCIRCUIT_SRC_DIR");

   flags = LIBOVERRIDE | LIBLOADED | FONTOVERRIDE;

   if (!tmp_s) tmp_s = SCRIPTS_DIR; 
   sprintf(_STR2, "%s/%s", tmp_s, STARTUP_FILE);

   if ((fd = fopen(_STR2, "r")) == NULL) {
      sprintf(_STR2, "%s/%s", SCRIPTS_DIR, STARTUP_FILE);
      if ((fd = fopen(_STR2, "r")) == NULL) {
         Wprintf("Failed to open startup script \"%s\"\n", STARTUP_FILE);
	 return;
      }
   }
   PyRun_SimpleFile(fd, _STR2);
}

/*----------------------------------------------------------------------*/
/* Execute a script                                                     */
/*----------------------------------------------------------------------*/

void execscript()
{
   FILE *fd;

   flags = 0;

   xc_tilde_expand(_STR2, 249);
   if ((fd = fopen(_STR2, "r")) != NULL) {
      PyRun_SimpleFile(fd, _STR2);
      refresh(NULL, NULL, NULL);
   }
   else {
      Wprintf("Failed to open script file \"%s\"\n", _STR2);
   }  
}

/*----------------------------------------------------------------------*/
/* Execute the .xcircuitrc startup script                               */
/*----------------------------------------------------------------------*/

void loadrcfile()
{
   char *userdir = getenv((const char *)"HOME");
   FILE *fd;
   short i;

   /* Initialize flags */

   flags = 0;

   sprintf(_STR2, "%s", USER_RC_FILE);     /* Name imported from Makefile */
      
   /* try first in current directory, then look in user's home directory */
      
   xc_tilde_expand(_STR2, 249);
   if ((fd = fopen(_STR2, "r")) == NULL) {
      if (userdir != NULL) {
         sprintf(_STR2, "%s/%s", userdir, USER_RC_FILE);
         fd = fopen(_STR2, "r");
      }
   }  
   if (fd != NULL)
      PyRun_SimpleFile(fd, _STR2);

   /* Add the default font if not loaded already */

   if (!(flags & FONTOVERRIDE)) {
      loadfontfile("Helvetica");
      if (areawin->psfont == -1)
         for (i = 0; i < fontcount; i++)
	    if (!strcmp(fonts[i].psname, "Helvetica")) {
	       areawin->psfont = i;
	       break;
	    }
   }

   if (areawin->psfont == -1) areawin->psfont = 0;

   setdefaultfontmarks();
      
   /* arrange the loaded libraries */
      
   if (!(flags & (LIBOVERRIDE | LIBLOADED)))
      defaultscript();
      
   /* Add the default colors */
      
   if (!(flags & COLOROVERRIDE)) {
      addnewcolorentry(xc_alloccolor("Gray40"));
      addnewcolorentry(xc_alloccolor("Gray60"));
      addnewcolorentry(xc_alloccolor("Gray80"));
      addnewcolorentry(xc_alloccolor("Gray90"));
      addnewcolorentry(xc_alloccolor("Red"));
      addnewcolorentry(xc_alloccolor("Blue"));
      addnewcolorentry(xc_alloccolor("Green2"));
      addnewcolorentry(xc_alloccolor("Yellow"));
      addnewcolorentry(xc_alloccolor("Purple"));
      addnewcolorentry(xc_alloccolor("SteelBlue2"));
      addnewcolorentry(xc_alloccolor("Red3"));
      addnewcolorentry(xc_alloccolor("Tan"));
      addnewcolorentry(xc_alloccolor("Brown"));
   }  

   /* These colors must be enabled whether or not colors are overridden, */
   /* because they are needed by the schematic capture system.		 */

   addnewcolorentry(xc_getlayoutcolor(LOCALPINCOLOR));
   addnewcolorentry(xc_getlayoutcolor(GLOBALPINCOLOR));
   addnewcolorentry(xc_getlayoutcolor(INFOLABELCOLOR));
   addnewcolorentry(xc_getlayoutcolor(RATSNESTCOLOR));
   addnewcolorentry(xc_getlayoutcolor(BBOXCOLOR));

   if (!(flags & KEYOVERRIDE))
      default_keybindings();
}

#endif
/* #endif HAVE_PYTHON and !TCL_WRAPPER */
/*--------------------------------------------------------------*/
