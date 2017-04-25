/*----------------------------------------------------------------------*/
/* prototypes.h:							*/
/*     Master list of function prototypes				*/
/*----------------------------------------------------------------------*/

/* from undo.c */

/* Note variable argument list for register_for_undo() */
extern void register_for_undo(u_int, u_char, objinstptr, ...);
extern void undo_finish_series(void);
extern void undo_action(void);
extern void redo_action(void);
extern void flush_undo_stack(void);
extern void flush_redo_stack(void);
extern void truncate_undo_stack(void);
extern void free_undo_record(Undoptr);
extern void free_redo_record(Undoptr);
extern stringpart *get_original_string(labelptr);
extern short *recover_selectlist(Undoptr);
extern void free_selection(uselection *);
extern uselection *remember_selection(objinstptr, short *, int);
extern short *regen_selection(objinstptr, uselection *);
#ifndef TCL_WRAPPER
extern void undo_call(xcWidget, caddr_t, caddr_t);
extern void redo_call(xcWidget, caddr_t, caddr_t);
#endif


/* from tclxcircuit.c */

#ifdef TCL_WRAPPER
extern void xctk_drag(ClientData, XEvent *);
extern void xctk_draglscroll(ClientData, XEvent *);
extern void xctk_dragfilebox(ClientData, XEvent *);
extern void tcl_stdflush(FILE *);
extern void tcl_printf(FILE *, const char *, ...);
extern int XcTagCallback(Tcl_Interp *, int, Tcl_Obj *CONST objv[]);
extern Tcl_Obj *evaluate_raw(objectptr, oparamptr, objinstptr, int *);
extern char *TCL_to_PS(char *);
extern XCWindowData *GUI_init(int, Tcl_Obj *CONST objv[]);
extern void build_app_database(Tk_Window);
extern int XcInternalTagCall(Tcl_Interp *, int, ...);
extern char *translateparamtype(int);
extern char *translatestyle(int);
extern char *translateencoding(int);

#endif

extern char *evaluate_expr(objectptr, oparamptr, objinstptr);

/* from elements.c: */

/* element constructor functions */
extern labelptr new_label(objinstptr, stringpart *, int, int, int, u_char);
extern labelptr new_simple_label(objinstptr, char *, int, int, int);
extern labelptr new_temporary_label(objectptr, char *, int, int);
extern polyptr new_polygon(objinstptr, pointlist *, int);
extern splineptr new_spline(objinstptr, pointlist);
extern arcptr new_arc(objinstptr, int, int, int);
extern objinstptr new_objinst(objinstptr, objinstptr, int, int);

/* element destructor function */
extern void remove_element(objinstptr, genericptr);

/* functions to set default values for element types */
extern void polydefaults(polyptr, int, int, int);
extern void splinedefaults(splineptr, int, int);
extern void arcdefaults(arcptr, int, int);
extern void pathdefaults(pathptr, int, int);
extern void instancedefaults(objinstptr, objectptr, int, int);
extern void labeldefaults(labelptr, u_char, int, int);

extern void converttocurve();
extern void poly_add_point(polyptr, XPoint *);
extern void drawdot(int, int);
extern void copyalleparams(genericptr, genericptr);
extern void copyparams(objinstptr, objinstptr);

extern void textbutton(u_char, int, int);
extern void charreport(labelptr);
extern Boolean labeltext(int, char *);
extern void textreturn(void);
extern void reanchor(short);
extern void findconstrained(polyptr);
extern void reversepoints(XPoint *, short);
extern void reversefpoints(XfPoint *, short);
extern void freeparts(short *, short);
extern void removep(short *, short);
extern void unjoin(void);
extern labelptr findlabelcopy(labelptr, stringpart *);
extern Boolean neartest(XPoint *, XPoint *);
extern void join(void);
extern genericptr getsubpart(pathptr, int *);

/* interactive manipulation of elements */
extern void splinebutton(int, int);
extern void updatepath(pathptr);
extern void trackelement(xcWidget, caddr_t, caddr_t);
extern void arcbutton(int, int);
extern void trackarc(xcWidget, caddr_t, caddr_t);
extern void boxbutton(int, int);
extern void trackbox(xcWidget, caddr_t, caddr_t);
extern void trackwire(xcWidget, caddr_t, caddr_t);
extern void startwire(XPoint *);
extern void setendpoint(short *, short, XPoint **, XPoint *);
extern void wire_op(int, int ,int);

void arc_mode_draw(xcDrawType type, arc *newarc);
void spline_mode_draw(xcDrawType type, spline *newspline);
void poly_mode_draw(xcDrawType type, polygon *newpoly);
void path_mode_draw(xcDrawType type, path *newpoly);
void text_mode_draw(xcDrawType type, label *newlabel);
void selarea_mode_draw(xcDrawType type, void *unused);
void rescale_mode_draw(xcDrawType type, void *unused);
void move_mode_draw(xcDrawType type, void *unused);
void normal_mode_draw(xcDrawType type, void *unused);

/* from events.c: */

extern Boolean recursefind(objectptr, objectptr);
extern void transferselects(void);
extern void select_invalidate_netlist(void);
extern void newmatrix(void);
extern void setpage(Boolean);
extern int changepage(short);
extern void newpage(short);
extern void pushobject(objinstptr);
extern void push_stack(pushlistptr *, objinstptr, char *);
extern void pop_stack(pushlistptr *);
extern void free_stack(pushlistptr *);
extern void popobject(xcWidget, pointertype, caddr_t);
extern void resetbutton(xcWidget, pointertype, caddr_t);
extern void drawhbar(xcWidget, caddr_t, caddr_t);
extern void drawvbar(xcWidget, caddr_t, caddr_t);
extern void panhbar(xcWidget, caddr_t, XButtonEvent *);
extern void endhbar(xcWidget, caddr_t, XButtonEvent *);
extern void panvbar(xcWidget, caddr_t, XButtonEvent *);
extern void endvbar(xcWidget, caddr_t, XButtonEvent *);
extern void zoombox(xcWidget, caddr_t, caddr_t);
extern void zoomin(int, int);
extern void zoomout(int, int);
extern void warppointer(int, int);
extern void panbutton(u_int, int, int, float);
extern void zoominrefresh(int, int);
extern void zoomoutrefresh(int, int);
extern void panrefresh(u_int, int, int, float);
extern void checkwarp(XPoint *);
extern void warparccycle(arcptr, short);
extern int checkcycle(genericptr, short);
extern pointselect *getrefpoint(genericptr, XPoint **);
extern void copyvirtual(void);
extern void nextpathcycle(pathptr, short);
extern void nextpolycycle(polyptr *, short);
extern void nextsplinecycle(splineptr *, short);
extern void nextarccycle(arcptr *, short);
extern void buttonhandler(xcWidget, caddr_t, XButtonEvent *);
extern void keyhandler(xcWidget, caddr_t, XKeyEvent *);
extern Boolean compatible_function(int);
extern int eventdispatch(int, int, int);
extern int functiondispatch(int, short, int, int);
extern void releasehandler(xcWidget, caddr_t, XKeyEvent *);
extern void setsnap(short);
extern void snapelement(void);
extern int ipow10(int);
extern int calcgcf(int, int);
extern void fraccalc(float, char *);
extern void printpos(short, short);
extern void findwirex(XPoint *, XPoint *, XPoint *, XPoint *, float *);
extern void findattach(XPoint *, float *, XPoint *);
extern XPoint *pathclosepoint(pathptr, XPoint *);
extern void placeselects(short, short, XPoint *);
extern void drag(int, int);
extern void xlib_drag(xcWidget, caddr_t, XEvent *);
extern void elemrotate(genericptr *, float, XPoint *);
extern void elementrotate(float, XPoint *);
extern void edit(int, int);
extern void pathedit(genericptr);
extern void xc_lower();
extern void xc_raise();
extern void xc_top(short *, short *);
extern void xc_bottom(short *, short *);
extern void exchange(void);
extern void elhflip(genericptr *, short);
extern void elvflip(genericptr *, short);
extern void elementflip(XPoint *);
extern void elementvflip(XPoint *);
extern short getkeynum(void);
#ifdef TCL_WRAPPER
extern void makepress(ClientData);
#else
extern void makepress(XtPointer, xcIntervalId *);
#endif
extern void reviseselect(short *, int, short *);
extern void deletebutton(int, int);
extern void delete_one_element(objinstptr, genericptr);
extern short *xc_undelete(objinstptr, objectptr, short, short *);
extern objectptr delete_element(objinstptr, short *, int, short);
extern void printname(objectptr);
extern Boolean checkname(objectptr);
extern char *checkvalidname(char *, objectptr);
extern objectptr finddot(void);
extern void movepoints(genericptr *, short, short);
extern void editpoints(genericptr *, short, short);
#ifndef TCL_WRAPPER
extern void xlib_makeobject(xcWidget, caddr_t);
#endif
extern objinstptr domakeobject(int, char *, Boolean);
extern void selectsave(xcWidget, caddr_t, caddr_t);
extern void arceditpush(arcptr);
extern void splineeditpush(splineptr);
extern void polyeditpush(polyptr);
extern void patheditpush(pathptr);
extern pointlist copypoints(pointlist, int);
extern void labelcopy(labelptr, labelptr);
extern void arccopy(arcptr, arcptr);
extern void polycopy(polyptr, polyptr);
extern void splinecopy(splineptr, splineptr);
extern void pathcopy(pathptr, pathptr);
extern void instcopy(objinstptr, objinstptr);
extern void delete_tagged(objinstptr);
extern void createcopies(void);
extern void copydrag(void);
extern void copy_op(int, int, int);
extern Boolean checkmultiple(XButtonEvent *);
extern void continue_op(int, int, int);
extern void finish_op(int, int, int);
extern void path_op(genericptr, int, int, int);
extern void inst_op(genericptr, int, int, int);
extern void resizearea(xcWidget, caddr_t, caddr_t);
void draw_grids(void);
void draw_fixed(void);
extern void drawarea(xcWidget, caddr_t, caddr_t);
extern void standard_element_delete(short);
extern void delete_for_xfer(short, short *, int);
extern void delete_noundo(short);

/* from filelist.c: */

extern int fcompare(const void *, const void *);
extern void dragfilebox(xcWidget, caddr_t, XMotionEvent *);
extern void startfiletrack(xcWidget, caddr_t, XCrossingEvent *);
extern void endfiletrack(xcWidget, caddr_t, XCrossingEvent *);
extern char *getcrashfilename(void);
extern void crashrecover(void);
extern void findcrashfiles(void);
extern void listfiles(xcWidget, popupstruct *, caddr_t);
extern void newfilelist(xcWidget, popupstruct *);
extern void fileselect(xcWidget, popupstruct *, XButtonEvent *);
extern void showlscroll(xcWidget, caddr_t, caddr_t);
extern void draglscroll(xcWidget, popupstruct *, XButtonEvent *);
extern void genfilelist(xcWidget, popupstruct *, Dimension);
extern int lookdirectory(char *, int);

/* from files.c: */

#ifdef ASG
extern void importspice(void);
#endif

extern char *ridnewline(char *);
extern void free_single(genericptr);
extern void reset(objectptr, short);
extern void pagereset(short);
extern void initmem(objectptr);
extern void freelabel(stringpart *);
extern Boolean compare_single(genericptr *, genericptr *);
extern Boolean elemcompare(genericptr *, genericptr *);
extern short objcompare(objectptr, objectptr);
extern float getpsscale(float, short);
extern void dostcount(FILE *, short *, short);
extern short printparams(FILE *, objinstptr, short);
extern void printobjectparams(FILE *, objectptr);
extern void varcheck(FILE *, short, objectptr, short *, genericptr, u_char);
extern void varfcheck(FILE *, float, objectptr, short *, genericptr, u_char);
extern Boolean varpcheck(FILE *, short, objectptr, int, short *, genericptr, u_char);
extern Boolean varpathcheck(FILE *, short, objectptr, int, short *,
		genericptr *, pathptr, u_char);
extern void getfile(xcWidget, pointertype, caddr_t);
extern int filecmp(char *, char *);
extern Boolean nextfilename(void);
extern FILE *libopen(char *, short, char *, int);
extern void loadfontlib(void);
extern void loadglib(Boolean, short, short);
extern void loadulib(void);
extern void loadblib(void);
extern void getlib(xcWidget, caddr_t, caddr_t);
extern void getuserlib(xcWidget, caddr_t, caddr_t);
extern Boolean loadlibrary(short);
extern void startloadfile(int);
extern void normalloadfile(void);
extern void importfile(void);
extern void importgraphic(void);
extern Boolean loadfile(short, int);
extern void readlabel(objectptr, char *, stringpart **);
extern void readparams(objectptr, objinstptr, objectptr, char *);
extern u_char *find_match(u_char *);
extern char *advancetoken(char *);
extern char *varpscan(objectptr, char *, short *, genericptr, int, int, u_char);
extern char *varscan(objectptr, char *, short *, genericptr, u_char);
extern char *varfscan(objectptr, char *, float *, genericptr, u_char);
extern objinstptr addtoinstlist(int, objectptr, Boolean);
extern Boolean objectread(FILE *, objectptr, short, short, short, char *, int, TechPtr);
void importfromlibrary(short, char *, char *);
objectptr *new_library_object(short, char *, objlistptr *, TechPtr);
Boolean library_object_unique(short, objectptr, objlistptr);
void add_object_to_library(short, objectptr);
u_char *find_delimiter(u_char *);
char standard_delimiter_end(char);
void output_graphic_data(FILE *, short *);

extern Boolean CompareTechnology(objectptr, char *);
extern TechPtr LookupTechnology(char *);
extern TechPtr GetObjectTechnology(objectptr);
extern TechPtr AddNewTechnology(char *, char *);
extern void AddObjectTechnology(objectptr);
extern void TechReplaceSave();
extern void TechReplaceRestore();
extern void TechReplaceAll();
extern void TechReplaceNone();

#ifdef TCL_WRAPPER
extern void setfile(char *, int);
extern void savetemp(ClientData);
#else
extern void setfile(xcWidget, xcWidget, caddr_t);
extern void savetemp(XtPointer, xcIntervalId *);
#endif
extern void incr_changes(objectptr);
extern void savelibpopup(xcWidget, char *, caddr_t);
#ifndef TCL_WRAPPER
extern void savelibrary(xcWidget, char *);
#endif
extern void savetechnology(char *, char *);
extern void findfonts(objectptr, short *);
extern void savefile(short);
extern int printRGBvalues(char *, int, const char *);
extern char *nosprint(char *, int *, int *);
extern FILE *fileopen(char *, char *, char *, int);
extern Boolean xc_tilde_expand(char *, int);
extern Boolean xc_variable_expand(char *, int);
extern short writelabel(FILE *, stringpart *, short *);
extern char *writesegment(stringpart *, float *, int *, int *, int *);
extern int writelabelsegs(FILE *, short *, stringpart *);
extern void printobjects(FILE *, objectptr, objectptr **, short *, int);
extern void printrefobjects(FILE *, objectptr, objectptr **, short *);
extern void printpageobject(FILE *, objectptr, short, short);


/* from fontfile.c: */

extern FILE *findfontfile(char *);
extern int loadfontfile(char *);

/* from formats.c: */

extern void loadlgf(int);
extern void loadmat4(caddr_t);

/* from functions.c: */

extern long sqwirelen(XPoint *, XPoint *);
extern float fsqwirelen(XfPoint *, XfPoint *);
extern int wirelength(XPoint *, XPoint *);
extern long finddist(XPoint *,XPoint *, XPoint *);
extern void calcarc(arcptr);
extern void decomposearc(pathptr);
extern void initsplines(void);
extern void computecoeffs(splineptr, float *, float *, float *, float *,
                          float *, float *);
extern void calcspline(splineptr);
extern void findsplinepos(splineptr, float, XPoint *, float *);
extern void ffindsplinepos(splineptr, float, XfPoint *);
extern float findsplinemin(splineptr, XPoint *);
extern short closepoint(polyptr, XPoint *);
extern short closedistance(polyptr, XPoint *);
extern void updateinstparam(objectptr);
extern short checkbounds(void);
extern void window_to_user(short, short, XPoint *);
extern void user_to_window(XPoint, XPoint *);

extern float UTopScale(void);
extern float UTopTransScale(float);
extern float UTopDrawingScale(void);
extern float UTopRotation(void);
extern void UTopOffset(int *, int *);
extern void UTopDrawingOffset(int *, int *);

extern XPoint UGetCursor(void);
extern XPoint UGetCursorPos(void);
extern void u2u_snap(XPoint *);
extern void snap(short, short, XPoint *);
extern void UResetCTM(Matrix *);
extern void InvertCTM(Matrix *);
extern void UCopyCTM(Matrix *, Matrix *);
extern void UMakeWCTM(Matrix *);
extern void UMultCTM(Matrix *, XPoint, float, float);
extern void USlantCTM(Matrix *, float);
extern void UPreScaleCTM(Matrix *);
extern short flipadjust(short);
extern void UPreMultCTM(Matrix *, XPoint, float, float);
extern void UPreMultCTMbyMat(Matrix *, Matrix *);
extern void UTransformbyCTM(Matrix *, XPoint *, XPoint *, short);
extern void UfTransformbyCTM(Matrix *, XfPoint *, XPoint *, short);
extern void UPopCTM(void);
extern void UPushCTM(void);
extern void UTransformPoints(XPoint *, XPoint *, short, XPoint, float,
                             float);
extern void InvTransformPoints(XPoint *, XPoint *, short, XPoint, float,
                             float);
extern void manhattanize(XPoint *, polyptr, short, Boolean);
extern void bboxcalc(short, short *, short *);
extern void calcextents(genericptr *, short *, short *, short *, short *);
extern void objinstbbox(objinstptr, XPoint *, int);
extern void labelbbox(labelptr, XPoint *, objinstptr);
extern void graphicbbox(graphicptr, XPoint *);
extern void calcinstbbox(genericptr *, short *, short *, short *, short *);
extern void calcbboxsingle(genericptr *, objinstptr, short *, short *, short *, short *);
extern Boolean object_in_library(short, objectptr);
extern void calcbboxinst(objinstptr);
extern short find_object(objectptr, objectptr);
extern void updatepagebounds(objectptr);
extern void calcbbox(objinstptr);
extern void calcbboxparam(objectptr, int);
extern void singlebbox(genericptr *);
extern void calcbboxselect(void);
extern void calcbboxvalues(objinstptr, genericptr *);
extern void centerview(objinstptr);
extern void refresh(xcWidget, caddr_t, caddr_t);
extern void zoomview(xcWidget, caddr_t, caddr_t);
extern void UDrawSimpleLine(XPoint *, XPoint *);
extern void UDrawLine(XPoint *, XPoint *);
extern void UDrawCircle(XPoint *, u_char);
extern void UDrawX(labelptr);
extern void UDrawXDown(labelptr);
extern int  toplevelwidth(objinstptr, short *);
extern int  toplevelheight(objinstptr, short *);
extern void extendschembbox(objinstptr, XPoint *, XPoint *);
extern void pinadjust(short, short *, short *, short);
extern void UDrawTextLine(labelptr, short);
extern void UDrawTLine(labelptr);
extern void UDrawXLine(XPoint, XPoint);
extern void UDrawBox(XPoint, XPoint);
extern float UGetRescaleBox(XPoint *corner, XPoint *newpoints);
extern void UDrawRescaleBox(XPoint *);
extern void UDrawBBox(void);
extern void strokepath(XPoint *, short, short, float);
extern void makesplinepath(splineptr, XPoint *);
extern void UDrawSpline(splineptr, float);
extern void UDrawPolygon(polyptr, float);
extern void UDrawArc(arcptr, float);
extern void UDrawPath(pathptr, float);
extern void UDrawObject(objinstptr, short, int, float, pushlistptr *);
extern void TopDoLatex(void);

/* from help.c: */

extern void showhsb(xcWidget, caddr_t, caddr_t);
extern void printhelppix(void);
extern void starthelp(xcWidget, caddr_t, caddr_t);
extern void simplescroll(xcWidget, xcWidget, XPointerMovedEvent *);
extern void exposehelp(xcWidget, caddr_t, caddr_t);
extern void printhelp(xcWidget);

/* from keybindings.c */

extern int firstbinding(xcWidget, int);
extern Boolean ismacro(xcWidget, int);
extern int boundfunction(xcWidget, int, short *);
extern int string_to_func(const char *, short *);
extern int string_to_key(const char *);
extern char *function_binding_to_string(xcWidget, int);
extern char *key_binding_to_string(xcWidget, int);
extern char *compat_key_to_string(xcWidget, int);
extern char *func_to_string(int);
extern char *key_to_string(int);
extern void printeditbindings(void);
extern int add_vbinding(xcWidget, int, int, short);
extern int add_binding(xcWidget, int, int);
extern int add_keybinding(xcWidget, const char *, const char *);
extern void default_keybindings(void);
extern int remove_binding(xcWidget, int, int);
extern void remove_keybinding(xcWidget, const char *, const char *);

#ifndef TCL_WRAPPER
extern void mode_rebinding(int, int);
extern void mode_tempbinding(int, int);
#endif

/* from libraries.c: */

extern short findhelvetica(void);
extern void catreturn(void);
extern int pageposition(short, int, int, int);
extern short pagelinks(int);
extern short *pagetotals(int, short);
extern Boolean is_virtual(objinstptr);
extern int is_page(objectptr);
extern int is_library(objectptr);
extern int NameToLibrary(char *);
extern void tech_set_changes(TechPtr);
extern void tech_mark_changed(TechPtr);
extern int libfindobject(objectptr, int *);
extern int libmoveobject(objectptr, int);
extern void pagecat_op(int, int, int);
extern void pageinstpos(short, short, objinstptr, int, int, int, int);
extern objinstptr newpageinst(objectptr);
extern void computespacing(short, int *, int *, int *, int *);
extern void composepagelib(short);
extern void updatepagelib(short, short);
extern void pagecatmove(int, int);
extern void composelib(short);
extern short finddepend(objinstptr, objectptr **);
extern void cathide(void);
extern void catvirtualcopy(void);
extern void catdelete(void);
extern void catmove(int, int);
extern void copycat(void);
extern void catalog_op(int, int, int);
extern void changecat(void);
extern void startcatalog(xcWidget, pointertype, caddr_t);

/* from menucalls.c: */

extern void setgrid(xcWidget, float *);
extern void measurestr(float, char *);
extern void setwidth(xcWidget, float *);
extern void changetextscale(float);
extern void autoscale(int);
extern float parseunits(char *);
extern Boolean setoutputpagesize(XPoint *);
extern void setkern(xcWidget, stringpart *);
extern void setdscale(xcWidget, XPoint *);
extern void setosize(xcWidget, objinstptr);
extern void setwwidth(xcWidget, void *);
#ifdef TCL_WRAPPER
extern void renamepage(short);
extern void renamelib(short);
extern void setcolormark(int);
extern void setallstylemarks(u_short);
#endif
extern labelptr gettextsize(float **);
extern void stringparam(xcWidget, caddr_t, caddr_t);
extern int setelementstyle(xcWidget, u_short, u_short);
extern void togglegrid(u_short);
extern void togglefontmark(int);
extern void setcolorscheme(Boolean);
extern void getgridtype(xcWidget, pointertype, caddr_t);
extern void newlibrary(xcWidget, caddr_t, caddr_t);
extern int createlibrary(Boolean);
extern void makepagebutton(void);
extern int findemptylib(void);
extern polyptr checkforbbox(objectptr);
#ifdef TCL_WRAPPER
extern void setcolor(xcWidget, int);
extern void setfontmarks(short, short);
#endif
extern void startparam(xcWidget, pointertype, caddr_t);
extern void startunparam(xcWidget, pointertype, caddr_t);
extern void setdefaultfontmarks(void);
extern void setanchorbit(xcWidget, pointertype, caddr_t);
extern void setpinanchorbit(xcWidget, pointertype, caddr_t);
extern void setanchor(xcWidget, pointertype, labelptr, short);
extern void setvanchor(xcWidget, pointertype, caddr_t);
extern void sethanchor(xcWidget, pointertype, caddr_t);
extern void boxedit(xcWidget, pointertype, caddr_t);
extern void locloadfont(xcWidget, char *);
extern short findbestfont(short, short, short, short);
extern void setfontval(xcWidget, pointertype, labelptr);
extern void setfont(xcWidget, pointertype, caddr_t);
extern void setfontstyle(xcWidget, pointertype, labelptr);
extern void fontstyle(xcWidget, pointertype, caddr_t);
extern void setfontencoding(xcWidget, pointertype, labelptr);
extern void fontencoding(xcWidget, pointertype, caddr_t);
extern void addtotext(xcWidget, pointertype, caddr_t);
extern Boolean dospecial(void);

/* from xtfuncs.c: */

extern void makenewfontbutton(void);  /* either here or menucalls.c */
#ifndef TCL_WRAPPER
extern void setfloat(xcWidget, float *);
extern void autoset(xcWidget, xcWidgetList, caddr_t);
extern void autostop(xcWidget, caddr_t, caddr_t);
extern void togglegridstyles(xcWidget);
extern void toggleanchors(xcWidget);
extern void togglefontstyles(xcWidget);
extern void toggleencodings(xcWidget);
extern void getkern(xcWidget, caddr_t, caddr_t);
extern void setcolor(xcWidget, pointertype, caddr_t);
extern void setfill(xcWidget, pointertype, caddr_t);
extern void makebbox(xcWidget, pointertype, caddr_t);
extern void setclosure(xcWidget, pointertype, caddr_t);
extern void setopaque(xcWidget, pointertype, caddr_t);
extern void setline(xcWidget, pointertype, caddr_t);
extern void changetool(xcWidget, pointertype, caddr_t);
extern void exec_or_changetool(xcWidget, pointertype, caddr_t);
extern void rotatetool(xcWidget, pointertype, caddr_t);
extern void pantool(xcWidget, pointertype, caddr_t);
extern void toggleexcl(xcWidget, menuptr, int);
extern void highlightexcl(xcWidget, int, int);
extern void toolcursor(int);
extern void promptparam(xcWidget, caddr_t, caddr_t);
extern void gettsize(xcWidget, caddr_t, caddr_t);
extern void settsize(xcWidget, labelptr);
extern void dotoolbar(xcWidget, caddr_t, caddr_t);
extern void overdrawpixmap(xcWidget);
extern buttonsave *getgeneric(xcWidget, void (*getfunction)(), void *);
extern void getsnapspace(xcWidget, caddr_t, caddr_t);
extern void getgridspace(xcWidget, caddr_t, caddr_t);
extern void setscaley(xcWidget, float *);
extern void setscalex(xcWidget, float *);
extern void setorient(xcWidget, short *);
extern void setpmode(xcWidget, short *);
extern void setpagesize(xcWidget, XPoint *);
extern void getdscale(xcWidget, caddr_t, caddr_t);
extern void getosize(xcWidget, caddr_t, caddr_t);
extern void getwirewidth(xcWidget, caddr_t, caddr_t);
extern void getwwidth(xcWidget, caddr_t, caddr_t);
extern void getfloat(xcWidget, float *, caddr_t);
extern void setfilename(xcWidget, char **);
extern void setpagelabel(xcWidget, char *);
extern void makenewfontbutton(void);
extern void newpagemenu(xcWidget, pointertype, caddr_t);
extern void makenewencodingbutton(char *, char);
extern void toggle(xcWidget, pointertype, Boolean *);
extern void inversecolor(xcWidget, pointertype, caddr_t);
extern void setgridtype(char *);
extern void renamepage(short);
extern void renamelib(short);
extern void setcolormark(int);
extern void setallstylemarks(u_short);
extern void setnewcolor(xcWidget, caddr_t);
extern void addnewcolor(xcWidget, caddr_t, caddr_t);
extern void setfontmarks(short, short);
extern void position_popup(xcWidget, xcWidget);
extern void border_popup(xcWidget, caddr_t, caddr_t);
extern void color_popup(xcWidget, caddr_t, caddr_t);
extern void fill_popup(xcWidget, caddr_t, caddr_t);
extern void param_popup(xcWidget, caddr_t, caddr_t);
extern void addnewfont(xcWidget, caddr_t, caddr_t);
#endif

/* from netlist.c: */

#ifdef TCL_WRAPPER
extern Tcl_Obj *tclglobals(objinstptr);
extern Tcl_Obj *tcltoplevel(objinstptr);
void ratsnest(objinstptr);
#endif

extern void ReferencePosition(objinstptr, XPoint *, XPoint *);
extern int NameToPinLocation(objinstptr, char *, int *, int *);
extern Boolean RemoveFromNetlist(objectptr, genericptr);
extern labelptr NetToLabel(int, objectptr);
extern void NameToPosition(objinstptr, labelptr, XPoint *);
extern XPoint *NetToPosition(int, objectptr);
extern int getsubnet(int, objectptr);
extern void invalidate_netlist(objectptr);
extern void remove_netlist_element(objectptr, genericptr);
extern int updatenets(objinstptr, Boolean);
extern void createnets(objinstptr, Boolean);
extern Boolean nonnetwork(polyptr);
extern int globalmax(void);
extern LabellistPtr geninfolist(objectptr, objinstptr, char *);
extern void gennetlist(objinstptr);
extern void gencalls(objectptr);
extern void search_on_siblings(objinstptr, objinstptr, pushlistptr,
		short, short, short, short);
extern char *GetHierarchy(pushlistptr *, Boolean);
extern Boolean HierNameToObject(objinstptr, char *, pushlistptr *);
extern void resolve_devindex(objectptr, Boolean);
extern void copy_bus(Genericlist *, Genericlist *);
extern Genericlist *is_resolved(genericptr *, pushlistptr, objectptr *);
extern Boolean highlightnet(objectptr, objinstptr, int, u_char);
extern void highlightnetlist(objectptr, objinstptr, u_char);
extern int pushnetwork(pushlistptr, objectptr);
extern Boolean match_buses(Genericlist *, Genericlist *, int);
extern int onsegment(XPoint *, XPoint *, XPoint *);
extern Boolean neardist(long);
extern Boolean nearpoint(XPoint *, XPoint *);
extern int searchconnect(XPoint *, int, objinstptr, int);
extern Genericlist *translateup(Genericlist *, objectptr, objectptr, objinstptr);
extern Genericlist *addpoly(objectptr, polyptr, Genericlist *);
extern long zsign(long, long);
extern Boolean mergenets(objectptr, Genericlist *, Genericlist *);
extern void removecall(objectptr, CalllistPtr);
extern Genericlist *addpin(objectptr, objinstptr, labelptr, Genericlist *);
extern Genericlist *addglobalpin(objectptr, objinstptr, labelptr, Genericlist *);
extern void addcall(objectptr, objectptr, objinstptr);
extern void addport(objectptr, Genericlist *);
extern Boolean addportcall(objectptr, Genericlist *, Genericlist *);
extern void makelocalpins(objectptr, CalllistPtr, char *);
extern int porttonet(objectptr, int);
extern stringpart *nettopin(int, objectptr, char *);
extern Genericlist *pointtonet(objectptr, objinstptr, XPoint *);
extern Genericlist *pintonet(objectptr, objinstptr, labelptr);
extern Genericlist *nametonet(objectptr, objinstptr, char *);
extern Genericlist *new_tmp_pin(objectptr, XPoint *, char *, char *, Genericlist *);
extern Genericlist *make_tmp_pin(objectptr, objinstptr, XPoint *, Genericlist *);
extern void resolve_devnames(objectptr);
extern void resolve_indices(objectptr, Boolean);
extern void clear_indices(objectptr);
extern void unnumber(objectptr);
extern char *parsepininfo(objinstptr, char *, int);
extern char *defaultpininfo(objinstptr, int);
extern char *parseinfo(objectptr, objectptr, CalllistPtr, char *, char *, Boolean,
		Boolean);
extern int writedevice(FILE *, char *, objectptr, CalllistPtr, char *);
extern void writeflat(objectptr, CalllistPtr, char *, FILE *, char *);
extern void writeglobals(objectptr, FILE *);
extern void writehierarchy(objectptr, objinstptr, CalllistPtr, FILE *, char *);
extern void writenet(objectptr, char *, char *);
extern Boolean writepcb(struct Ptab **, objectptr, CalllistPtr, char *, char *);
extern void outputpcb(struct Ptab *, FILE *);
extern void freepcb(struct Ptab *);
extern void freegenlist(Genericlist *);
extern void freepolylist(PolylistPtr *);
extern void freenetlist(objectptr);
extern void freelabellist(LabellistPtr *);
extern void freecalls(CalllistPtr);
extern void freenets(objectptr);
extern void freetemplabels(objectptr);
extern void freeglobals(void);
extern void destroynets(objectptr);
extern int  cleartraversed(objectptr);
extern int  checkvalid(objectptr);
extern void clearlocalpins(objectptr);
extern void append_included(char *);
extern Boolean check_included(char *);
extern void free_included(void);
extern void genprefixlist(objectptr, slistptr *);


/* from ngspice.c: */
extern int exit_spice(void);

/* from parameter.c: */ 

extern char *find_indirect_param(objinstptr, char *);
extern oparamptr match_param(objectptr, char *);
extern oparamptr match_instance_param(objinstptr, char *);
extern oparamptr find_param(objinstptr, char *);
extern int get_num_params(objectptr);
extern void free_all_eparams(genericptr);
extern void free_object_param(objectptr, oparamptr);
extern oparamptr free_instance_param(objinstptr, oparamptr);
extern void free_element_param(genericptr, eparamptr);

extern oparamptr make_new_parameter(char *);
extern eparamptr make_new_eparam(char *);

extern char *getnumericalpkey(u_int);
extern char *makeexprparam(objectptr, char *, char *, int);
extern Boolean makefloatparam(objectptr, char *, float);
extern Boolean makestringparam(objectptr, char *, stringpart *);
extern void std_eparam(genericptr, char *);
extern void indicateparams(genericptr);
extern void setparammarks(genericptr);
extern void makenumericalp(genericptr *, u_int, char *, short);
extern void noparmstrcpy(u_char *, u_char *);
extern void insertparam(void);
extern void makeparam(labelptr, char *);
extern void searchinst(objectptr, objectptr, char *);
extern stringpart *searchparam(stringpart *);
extern void unmakeparam(labelptr, objinstptr, stringpart *);
extern void removenumericalp(genericptr *, u_int);         
extern void unparameterize(int);
extern void parameterize(int, char *, short);
extern genericptr findparam(objectptr, void *, u_char);
extern Boolean paramcross(objectptr, labelptr);
extern oparamptr parampos(objectptr, labelptr, char *, short *, short *);
extern int opsubstitute(objectptr, objinstptr);
extern void exprsub(genericptr);
extern int epsubstitute(genericptr, objectptr, objinstptr, Boolean *);
extern int psubstitute(objinstptr);
extern Boolean has_param(genericptr);
extern oparamptr copyparameter(oparamptr);
extern eparamptr copyeparam(eparamptr, genericptr);

extern void pwriteback(objinstptr);
extern short paramlen(u_char *);
extern int natstrlen(u_char *);
extern int natstrcmp(u_char *, u_char *);
extern void curtail(u_char *);
extern int checklibtop(void);
extern void removeinst(objinstptr);
extern void removeparams(objectptr);
extern void resolveparams(objinstptr);

/* from python.c: */

#ifdef HAVE_PYTHON
extern int python_key_command(int);
extern void init_interpreter(void);
extern void exit_interpreter(void);
#endif

#ifdef HAVE_XPM
extern xcWidget *pytoolbuttons(int *);
#endif

/* from rcfile.c: */

extern short execcommand(short, char *);
#ifdef TCL_WRAPPER
extern int defaultscript(void);
extern int loadrcfile(void);
#else
extern void defaultscript(void);
extern void loadrcfile(void);
#endif
extern void execscript(void);
#ifndef HAVE_PYTHON
extern short readcommand(short, FILE *);
#endif

/* from graphic.c */

extern void count_graphics(objectptr, short *);
extern void UDrawGraphic(graphicptr);
extern Imagedata *addnewimage(char *, int, int);
extern graphicptr new_graphic(objinstptr, char *, int, int);
extern graphicptr gradient_field(objinstptr, int, int, int, int);
extern void invalidate_graphics(objectptr);
extern void freegraphic(graphicptr);
extern short *collect_graphics(short *);
xcImage *xcImageCreate(int width, int height);
void xcImageDestroy(xcImage *img);
int xcImageGetWidth(xcImage *img);
int xcImageGetHeight(xcImage *img);
void xcImagePutPixel(xcImage *img, int x, int y, u_char r, u_char g, u_char b);
void xcImageGetPixel(xcImage *img, int x, int y, u_char *r, u_char *g,
      u_char *b);

/* from flate.c */

#ifdef HAVE_LIBZ
extern u_long large_deflate(u_char *, u_long, u_char *, u_long);
extern u_long large_inflate(u_char *, u_long, u_char **, u_long);
extern unsigned long ps_deflate (unsigned char *, unsigned long,
	unsigned char *, unsigned long);
extern unsigned long ps_inflate (unsigned char *, unsigned long,
	unsigned char **, unsigned long);
#endif

/* from render.c: */

extern void ghostinit(void);
extern void send_client(Atom);
extern void ask_for_next(void);
extern void start_gs(void);
extern void parse_bg(FILE *, FILE *);
extern void bg_get_bbox(void);
extern void backgroundbbox(int);
extern void readbackground(FILE *);
extern void savebackground(FILE *, char *);
extern void register_bg(char *);
extern void loadbackground(void);
extern void send_to_gs(char *);
extern int renderbackground(void);
extern int copybackground(void);
extern int exit_gs(void);
extern int reset_gs(void);
void write_scale_position_and_run_gs(float norm, float xpos, float ypos,
      const char *bgfile);

#ifndef TCL_WRAPPER
extern Boolean render_client(XEvent *);
#endif

/* from schema.c: */

extern objectptr NameToPageObject(char *, objinstptr *, int *);
extern objectptr NameToObject(char *, objinstptr *, Boolean);
extern int checkpagename(objectptr);
extern void callwritenet(xcWidget, pointertype, caddr_t);
extern void startconnect(xcWidget, caddr_t, caddr_t);
extern void connectivity(xcWidget, caddr_t, caddr_t);
extern Boolean setobjecttype(objectptr);
extern void pinconvert(labelptr, pointertype);
extern void dopintype(xcWidget, pointertype, caddr_t);
extern void setsymschem(void);
extern int findpageobj(objectptr);
extern void collectsubschems(int);
extern int findsubschems(int, objectptr, int, short *, Boolean);
extern void copypinlabel(labelptr);
extern int checkschem(objectptr, char *);
extern int checksym(objectptr, char *);
extern int changeotherpins(labelptr, stringpart *);
extern void swapschem(int, int, char *);
extern void dobeforeswap(xcWidget, caddr_t, caddr_t);
extern void schemdisassoc(void);
extern void startschemassoc(xcWidget, pointertype, caddr_t);
extern Boolean schemassoc(objectptr, objectptr);
#ifndef TCL_WRAPPER
extern void xlib_swapschem(xcWidget, pointertype, caddr_t);
#endif

/* from selection.c: */

extern void enable_selects(objectptr, short *, int);
extern void disable_selects(objectptr, short *, int);
extern void selectfilter(xcWidget, pointertype, caddr_t);
extern Boolean checkselect(short);
extern Boolean checkselect_draw(short, Boolean);
extern void geneasydraw(short, int, objectptr, objinstptr);
extern void gendrawselected(short *, objectptr, objinstptr);
extern selection *genselectelement(short, u_char, objectptr, objinstptr);
extern short *allocselect(void);
extern void setoptionmenu(void);
extern int test_insideness(int, int, XPoint *);
extern Boolean pathselect(genericptr *, short, float);
extern Boolean areaelement(genericptr *, XPoint *, Boolean, short);
extern Boolean selectarea(objectptr, XPoint *, short);
extern void startdesel(xcWidget, caddr_t, caddr_t);
extern void deselect(xcWidget, caddr_t, caddr_t);
extern void draw_normal_selected(objectptr, objinstptr);
extern void freeselects(void);
extern void draw_all_selected(void);
extern void clearselects_noundo(void);
extern void clearselects(void);
extern void unselect_all(void);
extern void select_connected_pins();
extern void reset_cycles();
extern selection *recurselect(short, u_char, pushlistptr *);
extern short *recurse_select_element(short, u_char);
extern void startselect(void);
extern void trackselarea(void);
extern void trackrescale(void);
extern Boolean compareselection(selection *, selection *);
extern pointselect *addcycle(genericptr *, short, u_char);
extern void addanticycle(pathptr, splineptr, short);
extern void copycycles(pointselect **, pointselect **);
extern void advancecycle(genericptr *, short);
extern void removecycle(genericptr *);
extern void removeothercycles(pathptr, genericptr);
extern Boolean checkforcycles(short *, int);
extern void makerefcycle(pointselect *, short);

/* from text.c: */

extern Boolean hasparameter(labelptr);
extern void joinlabels(void);
extern void drawparamlabels(labelptr, short);
extern stringpart *nextstringpart(stringpart *, objinstptr);
extern stringpart *nextstringpartrecompute(stringpart *, objinstptr);
extern stringpart *makesegment(stringpart **, stringpart *);
extern stringpart *splitstring(int, stringpart **, objinstptr);
extern stringpart *mergestring(stringpart *);
extern stringpart *linkstring(objinstptr, stringpart *, Boolean);
extern int findcurfont(int, stringpart *, objinstptr);
extern stringpart *findtextinstring(char *, int *, stringpart *, objinstptr);
extern stringpart *findstringpart(int, int *, stringpart *, objinstptr);
extern void charprint(char *, stringpart *, int);
extern void charprinttex(char *, stringpart *, int);
extern char *stringprint(stringpart *, objinstptr);
extern char *textprint(stringpart *, objinstptr);
extern char *textprinttex(stringpart *, objinstptr);
extern char *textprintsubnet(stringpart *, objinstptr, int);
extern char *textprintnet(char *, char *, Genericlist *);
extern int textcomp(stringpart *, char *, objinstptr);
extern int textncomp(stringpart *, char *, objinstptr);
extern int stringcomp(stringpart *, stringpart *);
extern Boolean issymbolfont(int);
extern Boolean issansfont(int);
extern Boolean isisolatin1(int);
extern int stringcomprelaxed(stringpart *, stringpart *, objinstptr);
extern int stringparts(stringpart *);
extern int stringlength(stringpart *, Boolean, objinstptr);
extern stringpart *stringcopy(stringpart *);
extern stringpart *stringcopyall(stringpart *, objinstptr);
extern stringpart *stringcopyback(stringpart *, objinstptr);
extern stringpart *deletestring(stringpart *, stringpart **, objinstptr);
extern Genericlist *break_up_bus(labelptr, objinstptr, Genericlist *);
extern int sub_bus_idx(labelptr, objinstptr);
extern Boolean pin_is_bus(labelptr, objinstptr);
extern int find_cardinal(int, labelptr, objinstptr);
extern int find_ordinal(int, labelptr, objinstptr);

void UDrawCharString(u_char *text, int start, int end, XfPoint *offset,
     short styles, short ffont, int groupheight, int passcolor, float tmpscale);
extern void UDrawString(labelptr, int, objinstptr);
extern void UDrawStringNoX(labelptr, int, objinstptr);
extern void CheckMarginStop(labelptr, objinstptr, Boolean);
extern TextExtents ULength(labelptr, objinstptr, TextLinesInfo *);
extern void undrawtext(labelptr);
extern void redrawtext(labelptr);
extern void composefontlib(short);
extern void fontcat_op(int, int, int);

/* from xcircuit.c: */

extern void Wprintf(char *, ...);
extern void W1printf(char *, ...);
extern void W2printf(char *, ...);
extern void W3printf(char *, ...);

extern XCWindowData *create_new_window(void);
extern void pre_initialize(void);
extern void post_initialize(void);
extern void delete_window(XCWindowDataPtr);
extern void printeventmode(void);
extern void popupprompt(xcWidget, char *, char *, void (*function)(),
                        buttonsave *, const char *);
extern void getproptext(xcWidget, propstruct *, caddr_t);
extern int rgb_alloccolor(int, int, int);
extern void addtocolorlist(xcWidget, int);
extern int addnewcolorentry(int);
extern int xc_getlayoutcolor(int);
void xc_get_color_rgb(unsigned long cidx, unsigned short *red, 
      unsigned short *green, unsigned short *blue);
extern int query_named_color(char *);
extern caddr_t CvtStringToPixel(XrmValuePtr, int *, XrmValuePtr, XrmValuePtr);
extern void outputpopup(xcWidget, caddr_t, caddr_t);
extern void docommand(void);
extern int  installowncmap(void);  /* sometimes from xtgui.c */
extern void destroypopup(xcWidget, popupstruct *, caddr_t);
extern int xc_alloccolor(char *);
extern void dointr(int);
extern void DoNothing(xcWidget, caddr_t, caddr_t);
extern u_short countchanges(char **);
extern u_short getchanges(objectptr);
extern int quitcheck(xcWidget, caddr_t, caddr_t);
extern void quit(xcWidget, caddr_t);
extern void resizetoolbar(void);
extern void writescalevalues(char *, char *, char *);
#ifdef TCL_WRAPPER
extern Tcl_Obj *Tcl_NewHandleObj(void *);
extern int Tcl_GetHandleFromObj(Tcl_Interp *, Tcl_Obj *, void **);
#else
extern void updatetext(xcWidget, xcWidgetList, caddr_t);
extern void delwin(xcWidget, popupstruct *, XClientMessageEvent *);
#endif


extern void makecursors(void);

/* from cairo.c */

void xc_cairo_set_matrix(const Matrix *xcm);
void xc_cairo_set_color(int coloridx);
void xc_cairo_set_fontinfo(size_t fontidx);
