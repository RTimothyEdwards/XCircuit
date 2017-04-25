/*-------------------------------------------------------------------------*/
/* menus.h								   */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* Hierarchical menus must be listed in bottom-up order			   */
/*-------------------------------------------------------------------------*/
/* Note:  underscore (_) before name denotes a color to paint the button.  */
/*	  colon (:) before name denotes a stipple, defined by the data	   */
/*		    passed to setfill().				   */
/*-------------------------------------------------------------------------*/

#ifdef _MENUDEP

#define submenu(a)      a, (sizeof (a) / sizeof(menustruct)), NULL, NULL
#define action(b,c)     NULL, 0, NULL, NULL
#define setaction(b,c)  NULL, 1, NULL, NULL
#define toolaction(b,c) NULL, NULL
#define noaction        NULL, 0, NULL, NULL
#define offset(a)	0

#else                   /* real definitions. . .*/

#define submenu(a)	a, (sizeof (a) / sizeof(menustruct)), \
			(XtCallbackProc)DoNothing, NULL
#define action(b,c)	NULL, 0, (XtCallbackProc)b, c
#define setaction(b,c)	NULL, 1, (XtCallbackProc)b, c
#define toolaction(b,c) (XtCallbackProc)b, c
#define noaction	NULL, 0, (XtCallbackProc)DoNothing, NULL
#define offset(a)	Number(XtOffset(XCWindowDataPtr, a))

#endif

/* Inclusions for pixmap icons */

#include "tool_bar.h"

/* Things commented out here are reminders of future implementations. */

menustruct Fonts[] = {
	{"Add New Font", action(addnewfont, NULL)},
	{" ", noaction},
};

/* Note the unorthodox passing of integer constants through type pointer */

menustruct FontStyles[] = {
	{"Normal", setaction(fontstyle, Number(0))},
	{"Bold", action(fontstyle, Number(1))},
	{"Italic", action(fontstyle, Number(2))},
	{"BoldItalic", action(fontstyle, Number(3))},
	{" ", noaction},
	{"Subscript", action(addtotext, Number(SUBSCRIPT))},
	{"Superscript", action(addtotext, Number(SUPERSCRIPT))},
	{"Normalscript", action(addtotext, Number(NORMALSCRIPT))},
	{" ", noaction},
	{"Underline", action(addtotext, Number(UNDERLINE))},
	{"Overline", action(addtotext, Number(OVERLINE))},
	{"No line", action(addtotext, Number(NOLINE))},
};

menustruct FontEncodings[] = {
	{"Standard", action(fontencoding, Number(0))},
	{"ISO-Latin1", setaction(fontencoding, Number(2))},
};

menustruct TextSpacing[] = {
	{"Tab stop", action(addtotext, Number(TABSTOP))},
	{"Tab forward", action(addtotext, Number(TABFORWARD))},
	{"Tab backward", action(addtotext, Number(TABBACKWARD))},
	{"Carriage Return", action(addtotext, Number(RETURN))},
	{"1/2 space", action(addtotext, Number(HALFSPACE))},
	{"1/4 space", action(addtotext, Number(QTRSPACE))},
	{"Kern", action(getkern, NULL)},
	{"Character", action(addtotext, Number(SPECIAL))},
};

menustruct Anchors[] = {
	{"Left Anchored", setaction(sethanchor, Number(NORMAL))},
	{"Center Anchored", action(sethanchor, Number(NOTLEFT))},
	{"Right Anchored", action(sethanchor, Number(NOTLEFT | RIGHT))},
	{" ", noaction},
	{"Top Anchored", action(setvanchor, Number(NOTBOTTOM | TOP))},
	{"Middle Anchored", action(setvanchor, Number(NOTBOTTOM))},
	{"Bottom Anchored", setaction(setvanchor, Number(NORMAL))},
	{" ", noaction},
	{"Flip Invariant", setaction(setanchorbit, Number(FLIPINV))},
};
	
menustruct BoxEditStyles[] = {
	{"Manhattan Box Edit", setaction(boxedit, Number(MANHATTAN))},
	{"Rhomboid X", action(boxedit, Number(RHOMBOIDX))},
	{"Rhomboid Y", action(boxedit, Number(RHOMBOIDY))},
	{"Rhomboid A", action(boxedit, Number(RHOMBOIDA))},
	{"Normal", action(boxedit, Number(NORMAL))},
};

menustruct GridStyles[] = {
	{"Decimal Inches", action(getgridtype, Number(DEC_INCH))},
	{"Fractional Inches", setaction(getgridtype, Number(FRAC_INCH))},
	{"Centimeters", action(getgridtype, Number(CM))},
	{" ", noaction},
	{"Drawing Scale", action(getdscale, NULL)},
};

menustruct Libraries[] = {
	{"Add New Library", action(newlibrary, NULL)},
	{" ", noaction},
	{"Library 1", action(startcatalog, Number(LIBRARY))},
	{"Library 2", action(startcatalog, Number(LIBRARY + 1))},
};

menustruct Pages[] = {
	{"Add New Page", action(newpagemenu, Number(255))},
	{" ", noaction},
	{"Page 1", action(newpagemenu, Number(0))},
	{"Page 2", action(newpagemenu, Number(1))},
	{"Page 3", action(newpagemenu, Number(2))},
	{"Page 4", action(newpagemenu, Number(3))},
	{"Page 5", action(newpagemenu, Number(4))},
	{"Page 6", action(newpagemenu, Number(5))},
	{"Page 7", action(newpagemenu, Number(6))},
	{"Page 8", action(newpagemenu, Number(7))},
	{"Page 9", action(newpagemenu, Number(8))},
	{"Page 10", action(newpagemenu, Number(9))}
};

menustruct BorderStyles[] = {
	{"Linewidth", action(getwwidth, NULL)},
	{" ", noaction},
	{"Solid", setaction(setline, Number(NORMAL))},
	{"Dashed", action(setline, Number(DASHED))},
	{"Dotted", action(setline, Number(DOTTED))},
	{"Unbordered", action(setline, Number(NOBORDER))},
	{" ", noaction},
	{"Closed", action(setclosure, Number(UNCLOSED))}, 
	{"Bounding Box", action(makebbox, Number(BBOX))},
};

menustruct Colors[] = {
	{"Add New Color", action(addnewcolor, NULL)},
	{" ", noaction},
	{"Inherit Color", setaction(setcolor, Number(1))},
	{"_Black", action(setcolor, NULL)},
	{"_White", action(setcolor, NULL)},
};

menustruct Parameterize[] = {
	{"Substring", action(promptparam, NULL)},
	{"Numeric", action(startparam, Number(P_NUMERIC))},
	{"Style", action(startparam, Number(P_STYLE))},
	{"Anchoring", action(startparam, Number(P_ANCHOR))},
	{"Start Angle", action(startparam, Number(P_ANGLE1))},
	{"End Angle", action(startparam, Number(P_ANGLE2))},
	{"Radius", action(startparam, Number(P_RADIUS))},
	{"Minor Axis", action(startparam, Number(P_MINOR_AXIS))},
	{"Rotation", action(startparam, Number(P_ROTATION))},
	{"Scale", action(startparam, Number(P_SCALE))},
	{"Linewidth", action(startparam, Number(P_LINEWIDTH))},
	{"Color", action(startparam, Number(P_COLOR))},
	{"Position", action(startparam, Number(P_POSITION))},
};

menustruct Stipples[] = {
	{":Black",  action(setfill, Number(OPAQUE | FILLED | FILLSOLID))},
	{":Gray12", action(setfill, Number(OPAQUE | FILLED | STIP2 | STIP1))},
	{":Gray25", action(setfill, Number(OPAQUE | FILLED | STIP2 | STIP0))},
	{":Gray37", action(setfill, Number(OPAQUE | FILLED | STIP2))},
	{":Gray50", action(setfill, Number(OPAQUE | FILLED | STIP0 | STIP1))},
	{":Gray62", action(setfill, Number(OPAQUE | FILLED | STIP1))},
	{":Gray75", action(setfill, Number(OPAQUE | FILLED | STIP0))},
	{":Gray87", action(setfill, Number(OPAQUE | FILLED))},
	{":White", setaction(setfill, Number(FILLSOLID))},
	{" ", noaction},
	{"Opaque", action(setopaque, Number(OPAQUE))},
	{"Transparent", setaction(setopaque, Number(NORMAL))},
};

menustruct TextMenu[] = {
	{"Text Size", action(gettsize, NULL)},
	{"Font", submenu(Fonts)},
	{"Style", submenu(FontStyles)},
	{"Encoding", submenu(FontEncodings)},
	{"Insert", submenu(TextSpacing)},             
	{"Anchoring (keypad)", submenu(Anchors)},
	{"Parameterize", action(promptparam, NULL)},
	{"Unparameterize", action(startunparam, Number(P_SUBSTRING))},
	{" ", noaction},
	{"Set LaTeX Mode", action(setanchorbit, Number(LATEXLABEL))},
	{" ", noaction},
	{"Make Label (t)", action(changetool, Number(XCF_Text))},
};

menustruct PolyMenu[] = {
	{"Border", submenu(BorderStyles)},
	{"Fill", submenu(Stipples)},
	{"Color", submenu(Colors)},
	{" ", noaction},
	{"Object size", action(getosize, NULL)},
	{"Parameters", submenu(Parameterize)},
	{"Center Object", setaction(toggle, offset(center))},
	{"Manhattan Draw", action(toggle, offset(manhatn))},
	{"Polygon Edit", submenu(BoxEditStyles)},
};

menustruct FilterMenu[] = {
	{"Labels", setaction(selectfilter, Number(LABEL))},
	{"Objects", setaction(selectfilter, Number(OBJINST))},
	{"Polygons", setaction(selectfilter, Number(POLYGON))},
	{"Arcs", setaction(selectfilter, Number(ARC))},
	{"Splines", setaction(selectfilter, Number(SPLINE))},
	{"Paths", setaction(selectfilter, Number(PATH))},
};

menustruct RotateMenu[] = {
	{"Flip Horizontal (f)", action(exec_or_changetool, Number(XCF_Flip_X))},
	{"Flip Vertical (F)", action(exec_or_changetool, Number(XCF_Flip_Y))},
	{" ", noaction},
	{"Rotate CW 90", action(rotatetool, Number(90))},
	{"Rotate CW 45", action(rotatetool, Number(45))},
	{"Rotate CW 30", action(rotatetool, Number(30))},
	{"Rotate CW 15 (r)", action(rotatetool, Number(15))},
	{"Rotate CW 5 (o)", action(rotatetool, Number(3))},
	{"Rotate CW 1", action(rotatetool, Number(1))},
	{" ", noaction},
	{"Rotate CCW 90", action(rotatetool, Number(-90))},
	{"Rotate CCW 45", action(rotatetool, Number(-45))},
	{"Rotate CCW 30", action(rotatetool, Number(-30))}, 
	{"Rotate CCW 15 (R)", action(rotatetool, Number(-15))},
	{"Rotate CCW 5 (O)", action(rotatetool, Number(-5))},
	{"Rotate CCW 1", action(rotatetool, Number(-1))},
};

menustruct LineMenu[] = {
	{"Global Linewidth", action(getwirewidth, NULL)},
	{"Wire Linewidth", action(getwwidth, NULL)},
};

menustruct GridMenu[] = {
	{"Grid", setaction(toggle, offset(gridon))},
	{"Axes", setaction(toggle, offset(axeson))},
	{"Grid spacing", action(getgridspace, NULL)},
	{"Grid type/display", submenu(GridStyles)},
};

menustruct SnapMenu[] = {
	{"Snap-to", setaction(toggle, offset(snapto))},
        {"Snap spacing", action(getsnapspace, NULL)},
};

menustruct PinConvert[] = {
	{"Normal label", action(dopintype, Number(NORMAL))},
	{"Local Pin", action(dopintype, Number(LOCAL))},
	{"Global Pin", action(dopintype, Number(GLOBAL))},
	{"Info label", action(dopintype, Number(INFO))},
};

menustruct SchemaMenu[] = {
	{"Make Pin (T)", action(changetool, Number(XCF_Pin_Label))},
	{"Make Info Pin (I)", action(changetool, Number(XCF_Info_Label))},
	{"Make Global Pin (G)", action(changetool, Number(XCF_Pin_Global))},
	{"Convert Label to...", submenu(PinConvert)},
	{"Pin Visibility", action(setpinanchorbit, Number(PINVISIBLE))},
/*	{"Make Object Non-Schematic", action(maketrivial, NULL)}, */
	{"Make Matching Symbol", action(dobeforeswap, NULL)},
	{"Associate with Symbol", action(startschemassoc, Number(1))},
	{"Highlight Connectivity", action(startconnect, NULL)},
	{"Auto-number Components", action(callwritenet, Number(4))},
	{" ", noaction},
	{"Write spice", action(callwritenet, Number(0))},
	{"Write flattened spice", action(callwritenet, Number(3))},
	{"Write sim", action(callwritenet, Number(1))},
	{"Write pcb", action(callwritenet, Number(2))},
};

menustruct WindowMenu[] = {
	{"Zoom In (Z)", action(zoominrefresh, NULL)},
	{"Zoom Out (z)", action(zoomoutrefresh, NULL)},
	{"Full View (v)", action(zoomview, NULL)},
	{"Pan (p)", action(pantool, Number(6))},
	{"Refresh ( )", action(refresh, NULL)},
	{" ", noaction},
	{"Library Directory (L)", action(startcatalog, Number(LIBLIB))},
	{"Goto Library", submenu(Libraries)},
	{" ", noaction},
	{"Page Directory (P)", action(startcatalog, Number(PAGELIB))},
	{"Goto Page", submenu(Pages)}
};

menustruct FileMenu[] = {
#ifdef XC_WIN32
	{"New window", action(win32_new_window, NULL)},
	{" ", noaction},
#endif
	{"Read Xcircuit File", action(getfile, Number(NORMAL))},
	{"Import Xcircuit PS", action(getfile, Number(IMPORT))},
#if !defined(HAVE_CAIRO) || defined(HAVE_GS)
	{"Import background PS", action(getfile, Number(PSBKGROUND))},
#endif
#ifdef HAVE_CAIRO
	{"Import graphic image", action(getfile, Number(IMPORTGRAPHIC))},
#endif
#ifdef ASG
	{"Import SPICE deck", action(getfile, Number(IMPORTSPICE))},
#endif
	{"Execute script", action(getfile, Number(SCRIPT))},
	{"Write Xcircuit PS (W)", action(outputpopup, NULL)},
	{" ", noaction},
	{"Add To Library", action(getlib, NULL)},
	{"Load New Library", action(getuserlib, NULL)},
	{"Save Library", action(savelibpopup, NULL)},
	{" ", noaction},
        {"Clear Page", action(resetbutton, Number(0))},
	{" ", noaction},
	{"Quit", action(quitcheck, NULL)},
};

menustruct OptionMenu[] = {
	{"Alt Colors", action(inversecolor, offset(invert))},
	{"Show Bounding box", action(toggle, offset(bboxon))},
	{"Edit in place", setaction(toggle, offset(editinplace))},
	{"Show Pin Positions", action(toggle, offset(pinpointon))},
#ifdef HAVE_XPM
	{"Disable Toolbar", action(dotoolbar, NULL)},
#endif
	{"Grid", submenu(GridMenu)},
	{"Snap-to", submenu(SnapMenu)},
	{"Linewidth", submenu(LineMenu)},
	{"Elements", submenu(PolyMenu)},
	{" ", noaction},
	{"Help!", action(starthelp, NULL)},
};

menustruct EditMenu[] = {
	{"Undo (u)", action(undo_call, NULL)},
	{"Redo (U)", action(redo_call, NULL)},
	{" ", noaction},
	{"Delete (d)", action(exec_or_changetool, Number(XCF_Delete))},
	{"Copy (c)", action(exec_or_changetool, Number(XCF_Copy))},
	{"Edit (e)", action(exec_or_changetool, Number(XCF_Edit))},
	{"Rotate/Flip", submenu(RotateMenu)},
	{"Deselect (x)", action(startdesel, NULL)},
	{"Select filter", submenu(FilterMenu)},
	{"Push selected (>)", action(exec_or_changetool, Number(XCF_Push))},
	{"Pop hierarchy (<)", action(popobject, Number(0))},
	{" ", noaction},
	{"Make User Object (m)", action(selectsave, NULL)},
	{"Make Arc (a)", action(changetool, Number(XCF_Arc))},
	{"Make Box (b)", action(changetool, Number(XCF_Box))},
	{"Make Spline (s)", action(changetool, Number(XCF_Spline))},
	{"Join (j)", action(join, NULL)},
};

menustruct TopButtons[] = {
	{"File", submenu(FileMenu)},
	{"Edit", submenu(EditMenu)},
	{"Text", submenu(TextMenu)},
	{"Options", submenu(OptionMenu)},
	{"Window", submenu(WindowMenu)},
	{"Netlist", submenu(SchemaMenu)},
};

short maxbuttons = sizeof(TopButtons) / sizeof(menustruct);

#ifdef HAVE_XPM
/* Toolbar buttons */
toolbarstruct ToolBar[] = {
	{"Pan",	   pn_xpm,  toolaction(pantool, Number(6)),
		"pan mode"},
	{"Wire",   w_xpm,  toolaction(changetool, Number(XCF_Wire)),
		"draw wire"},
	{"Box",    b_xpm,  toolaction(changetool, Number(XCF_Box)),
		"draw box"},
	{"Arc",    a_xpm,  toolaction(changetool, Number(XCF_Arc)),
		"draw arc"},
	{"Spline", s_xpm,  toolaction(changetool, Number(XCF_Spline)),
		"draw spline"},
	{"Text",   t_xpm,  toolaction(changetool, Number(XCF_Text)),
		"enter text"},
	{"Move",   mv_xpm, toolaction(exec_or_changetool, Number(XCF_Move)),
		"move element"},
	{"Copy",   cp_xpm, toolaction(exec_or_changetool, Number(XCF_Copy)),
		"copy element"},
	{"Edit",   e_xpm,  toolaction(exec_or_changetool, Number(XCF_Edit)),
		"edit element"},
	{"Delete", d2_xpm, toolaction(exec_or_changetool, Number(XCF_Delete)),
		"delete element"},
	{"RotP",   cw_xpm, toolaction(rotatetool, Number(15)),
		"rotate 15 degrees clockwise"},
	{"RotN",  ccw_xpm, toolaction(rotatetool, Number(-15)),
		"rotate 15 degrees counterclockwise"},
	{"HFlip",  fx_xpm, toolaction(exec_or_changetool, Number(XCF_Flip_X)),
		"flip horizontal"},
	{"VFlip",  fy_xpm, toolaction(exec_or_changetool, Number(XCF_Flip_Y)),
		"flip vertical"},
	{"Push",  pu2_xpm, toolaction(exec_or_changetool, Number(XCF_Push)),
		"push (edit object)"},
	{"Pop",   po2_xpm, toolaction(popobject, Number(0)),
		"pop (return from object edit)"},
	{"Make",   mk_xpm, toolaction(selectsave, NULL),
		"make an object from selected elements"},
	{"Join",   pz_xpm, toolaction(join, NULL),
		"join elements into polygon or path"},
	{"Unjoin", uj_xpm, toolaction(unjoin, NULL),
		"separate path into elements"},
	{"Colors", co_xpm, toolaction(color_popup, NULL),
		"set color"},
	{"Border", bd_xpm, toolaction(border_popup, NULL),
		"set border and line properties"},
	{"Fills",  fi_xpm, toolaction(fill_popup, NULL),
		"set fill properties"},
	{"Parameters", pm_xpm,  toolaction(param_popup, NULL),
		"parameterize properties"},
	{"Library",li_xpm, toolaction(changecat, NULL),
		"go to next library"},
	{"Libdir", yp_xpm, toolaction(startcatalog, Number(LIBLIB)),
		"go to library directory"},
	{"Pagedir",pl_xpm, toolaction(startcatalog, Number(PAGELIB)),
		"go to page directory"},
	{"ZoomI",  z4_xpm, toolaction(zoominrefresh, NULL),
		"zoom in"},
	{"ZoomO",  z5_xpm, toolaction(zoomoutrefresh, NULL),
		"zoom out"},
	{"Help",   i_xpm,  toolaction(starthelp, NULL),
		"pop up help window"},
};

short toolbuttons = sizeof(ToolBar) / sizeof(toolbarstruct);
#endif

#undef submenu
#undef action
#undef setaction
#undef toolaction
#undef noaction
#undef offset

/*-------------------------------------------------------------------------*/
