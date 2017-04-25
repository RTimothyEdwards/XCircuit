#ifndef __XCWIN32_H__
#define __XCWIN32_H__

#include <windows.h>
#include <commctrl.h>

typedef int Boolean;
typedef Boolean Bool;
#define True 1
#define False 0
typedef void* XtPointer;
typedef short Dimension;
typedef short Position;
typedef HBITMAP Pixmap;
typedef struct __WinWidget *Widget;
typedef Widget *WidgetList;
typedef HWND Window;
typedef int Pixel;
typedef struct {
	int pixel;
	int red, green, blue;
	int flags;
} XColor;
#define DoRed	0x1
#define DoGreen	0x2
#define DoBlue	0x4
typedef struct __GC *GC;
typedef void Display;
typedef void* Colormap;
typedef int XtIntervalId;
typedef HCURSOR Cursor;
typedef void* caddr_t;
typedef void(*XtCallbackProc)();
typedef void(*XtEventHandler)();
typedef void(*XtTimerCallbackProc)();
typedef void* XtAppContext;
typedef struct __XrmValue *XrmValuePtr;
typedef struct {
	short x, y;
} XPoint;
typedef struct {
	short width, height;
	char *data;
	int bits_per_pixel;
	int bytes_per_line;
	HBITMAP handle;
} XImage;
typedef struct {
	int ascent, descent;
	HFONT fid;
} XFontStruct;
typedef struct {
	int type;
	int x, y;
	int button;
	int state;
} XButtonEvent;
typedef struct {
	int type;
	int x, y;
	int keycode;
	int state;
} XKeyEvent;
typedef struct {
	int type;
	int x, y;
} XMotionEvent, XPointerMovedEvent;
typedef void* XCrossingEvent;
typedef struct {
	int type;
	int message_type;
	Display* display;
	int format;
	HWND window;
	union {
		long l[5];
	} data;
} XClientMessageEvent;
typedef union {
	int type;
	XButtonEvent button;
	XKeyEvent key;
	XMotionEvent motion;
	XPointerMovedEvent pointer_moved;
	XCrossingEvent crossing;
	XClientMessageEvent xclient;
} XEvent;
typedef unsigned int KeySym;
typedef struct {
	int type;
	void *data;
} Arg;
typedef struct __CharStruct *XCharStruct;

struct __GC {
	/* colors */
	int foreground, background;
	int function;
	/* fill */
	int fill_style;
	HBITMAP fill_stipple;
	/* line */
	int line_width;
	int line_style;
	int line_cap;
	int line_join;
	int line_dash_len;
	char *line_dash;
	/* font */
	HFONT font;
	/* misc */
	Boolean graphics_exposures;
	HBITMAP clipMask;
};
typedef struct __GC XGCValues;
typedef ATOM Atom;
typedef void XWMHints;

typedef unsigned int Cardinal;
#define XtOffset(p_type,field) \
	        ((Cardinal) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

#define GCForeground		0x01
#define GCBackground		0x02
#define GCFont			0x04
#define GCFunction		0x08
#define GCGraphicsExposures	0x10

#define GXcopy R2_COPYPEN
#define GXxor R2_XORPEN

/* TODO */
#define PointerMotionMask	0x0001
#define ButtonMotionMask	0x0002
#define Button1MotionMask	0x0004
#define Button2MotionMask	0x0008
#define EnterWindowMask		0x0010
#define LeaveWindowMask		0x0020
#define ButtonPressMask		0x0040
#define KeyPressMask		0x0080
#define NoEventMask		0x0000

#define ButtonPress		0x01
#define ButtonRelease		0x02
#define KeyPress		0x03
#define KeyRelease		0x04
#define ClientMessage		0x05
#define MotionNotify		0x06

#define None		0
#define Nonconvex	0
#define CoordModeOrigin	0
#define NoSymbol	0

#define Mod1Mask    0x0001
#define Mod2Mask    0x0002
#define LockMask    0x0004
#define ShiftMask   0x0008
#define ControlMask 0x0010
#define Mod5Mask    0x0080
#define Button1     0x0100
#define Button2     0x0200
#define Button3     0x0800
#define Button4     0x1000
#define Button5     0x2000
#define Button1Mask Button1
#define Button2Mask Button2
#define Button3Mask Button3
#define Button4Mask Button4
#define Button5Mask Button5

#define LineSolid		PS_SOLID
#define LineOnOffDash		PS_DASH
#define CapRound		PS_ENDCAP_ROUND
#define CapButt			PS_ENDCAP_FLAT
#define CapProjecting		PS_ENDCAP_SQUARE
#define JoinBevel		PS_JOIN_BEVEL
#define JoinMiter		PS_JOIN_MITER
#define FillSolid		0
#define FillStippled		1
#define FillOpaqueStippled	2

#define XtNwidth		0
#define XtNheight		1
#define XtNx			2
#define XtNy			3
#define XtNfont			4
#define XtNexpose		5
#define XtNset			6
#define XtNsquare		7
#define XtNlabelLocation	8
#define XtNborderWidth		9
#define XtNselect		10
#define XtNrelease		11
#define XtNlabel		12
#define XtNxResizable		13
#define XtNyResizable		14
#define XtNxRefWidget		15
#define XtNyRefWidget		16
#define XtNyAddHeight		17
#define XtNyAttachBottom	18
#define XtNxAttachRight		19
#define XtNyAddHeight		17
#define XtNyAttachBottom	18
#define XtNxAttachRight		19
#define XtNyAddHeight		17
#define XtNyAttachBottom	18
#define XtNxAttachRight		19
#define XtNxAddWidth		20
#define XtNforeground		21
#define XtNstring		22
#define XtNsetMark		23
#define XtNbackground		24
#define XtNborderColor		25
#define XtNrectColor		26
#define XtNresize		27
#define XtNlabelType		28
#define XtNrectStipple		29
#define XtNattachTo		30
#define XtNsensitive		31
#define XtNxOffset		32
#define XtNyOffset		33
#define XtNlayout		34
#define XtNinsertPosition	35
#define XtNgravity		36
#define XtNscroll		37
#define XtNwrap			38
#define XtNexecute		39
#define XtNkeyUp		40
#define XtNkeyDown		41
#define XtNcolormap		42
#define XtNstrip		43

#define XwLEFT		0
#define XwRECT		1
#define XwIMAGE		2
#define XwIGNORE	3

#define WestGravity	0
#define XwWrapOff	0
#define XwAutoScrollHorizontal	0

#define XwworkSpaceWidgetClass		0x01
#define XwtoggleWidgetClass		0x02
#define XwmenubuttonWidgetClass		0x03
#define XwformWidgetClass		0x04
#define XwmenuButtonWidgetClass		0x05
#define XwbulletinWidgetClass		0x06
#define XwcascadeWidgetClass		0x07
#define XwpopupmgrWidgetClass		0x08
#define XwstaticTextWidgetClass		0x09
#define XwpushButtonWidgetClass		0x0A
#define XwtextEditWidgetClass		0x0B

#define XC_xterm IDC_IBEAM
#define XC_watch IDC_WAIT

#define XtWindow(x) WinWindow(x)
#define XtAppAddTimeOut(app,delay,proc,data) WinAddTimeOut(delay,proc,data)
#define XtRemoveTimeOut(ID) WinRemoveTimeOut(ID)
#define XWarpPointer(dpy,flag,win,x1,y1,x2,y2,x3,y3) WinWarpPointer(win,x3,y3)
/*#define XCheckWindowEvent(dpy,win,mask,evt) WinCheckWindowEvent(win,mask,evt)*/
#define XCheckWindowEvent(dpy,win,mask,evt) False
#define XtDispatchEvent(evt)
#define XtSetArg(arg,t,d) (arg.type = t, arg.data = (void*)(d))
#define XtGetValues(win,args,n) WinGetValues(win,args,n)
#define XtSetValues(win,args,n) WinSetValues(win,args,n)
#define XCreatePixmap(dpy,win,w,h,depth) WinCreatePixmap(win,w,h)
#define XFreePixmap(dpy,w) WinFreePixmap(w)
#define XCreateImage(dpy,visual,depth,flag,offset,data,w,h,pad,bpl) WinCreateImage(w,h)
#define XDestroyImage(img) WinDestroyImage(img)
#define XSetLineAttributes(dpy,gc,w,style,cap,join) WinSetLineAttributes(gc,w,style,cap,join)
#define XCreateGC(dpy,win,mask,gc) WinCreateGC(win,mask,gc)
#define XFreeGC(gc) WinFreeGC(gc)
#define XtParent(w) WinParent(w)
#define XtNameToWidget(parent,name) WinNameToWindow(parent,name)
#define XtCreateManagedWidget(name,cls,parent,args,n) WinCreateWidget(name,cls,parent,args,n,1)
#define XtCreateWidget(name,cls,parent,args,n) WinCreateWidget(name,cls,parent,args,n,0)
#define DefaultRootWindow(dpy) GetDesktopWindow()
#define DefaultColormap(dpy,screen) NULL
#define DefaultScreen(dpy) 0
#define XFreeColormap(dpy,cmap)
#define XtDisplay(x) GetDesktopWindow()
#define XtCreatePopupShell(name,cls,btn,args,n) WinCreatePopup(name,args,n)
#define XtPopup(w,flag) WinPopup(w)
#define XKeysymToString(ks) WinKeyToString(ks)
#define XChangeProperty(dpy,win,prop,type,format,mode,data,len)
#define XCreateBitmapFromData(dpy,win,bits,w,h) WinCreateBitmapFromData(win,bits,w,h)
#define XCreatePixmapCursor(dpy,src,mask,fg,bg,xhot,yhot) WinCreateCursor(src,mask,xhot,yhot)
#define XCreateFontCursor(dpy,type) WinCreateStandardCursor(type)
#define XtOpenApplication(app,name,dummy,n,Argc,Argv,argfb,cls,arglist,narg) WinCreateTopLevel(name,Argc,Argv)
#define XDefineCursor(dpy,w,cur) WinSetCursor(w,cur)
#define XtMalloc malloc
#define BlackPixel(dpy,screen) RGB(0,0,0)
#define WhitePixel(dpy,screen) RGB(255,255,255)
#define XBell(dpy,len) MessageBeep(-1)
#define XSync(dpy,flag) WinDebug("Should sync\n")
#define XFlush(dpy) WinDebug("Should flush\n")
#define XSendEvent(dpy,win,prop,mask,evt) WinDebug("Should send an event\n")
#define DisplayWidth(dpy,screen) WinDesktopWidth()
#define DisplayHeight(dpy,screen) WinDesktopHeight()
#define XGetPixel(img,x,y) WinGetPixel(img,x,y)
#define XPutPixel(img,x,y,c) WinPutPixel(img,x,y,c)
#define XDrawArc(dpy,win,gc,x,y,w,h,a1,a2) WinDrawArc(win,gc,x,y,w,h,a1,a2)
#define XDrawLine(dpy,win,gc,x1,y1,x2,y2) WinDrawLine(win,gc,x1,y1,x2,y2)
#define XDrawLines(dpy,win,gc,pl,n,dummy) WinDrawLines(win,gc,pl,n)
#define XFillPolygon(dpy,win,gc,pl,n,x1,x2) WinFillPolygon(win,gc,pl,n)
#define XClearArea(dpy,win,x,y,w,h,dummy) WinClearArea(win,x,y,w,h)
#define XFillRectangle(dpy,win,gc,x,y,w,h) WinFillRectangle(win,gc,x,y,w,h)
/*#define XSetLineAttributes(dpy,gc,w,s,c,j) WinSetLineAttributes(gc,w,s,c,j)*/
#define XDrawPoint(dpy,win,gc,x,y) WinDrawPoint(win,gc,x,y)
#define XSetForeground(dpy,gc,c) WinSetForeground(gc,c)
#define XSetFunction(dpy,gc,z) WinSetFunction(gc,z)
#define XSetBackground(dpy,gc,c) WinSetBackground(gc,c)
#define XCopyArea(dpy,win1,win2,gc,x,y,w,h,offx,offy) WinCopyArea(win1,win2,gc,x,y,w,h,offx,offy)
#define XTextWidth(font,str,len) WinTextWidth(font,str,len)
#define XtNumber(arr) ((int) (sizeof(arr) / sizeof(arr[0])))
#define XSetFillStyle(dpy,gc,fs) WinSetFillStyle(gc,fs)
#define XSetDashes(dpy,gc,offset,dash,n) WinSetDashes(gc,offset,dash,n)
#define XSetStipple(dpy,gc,stipple) WinSetStipple(gc,stipple)
#define XQueryPointer(dpy,win,rr,cr,rx,ry,x,y,m) WinQueryPointer(win,x,y)
#define XDrawRectangle(dpy,win,gc,x,y,w,h) WinDrawRectangle(win,gc,x,y,w,h)
#define XtManageChild(win) WinManageChild(win)
#define XtUnmanageChild(win) WinUnmanageChild(win)
#define XRecolorCursor(dpy,cur,fg,bg)
#define XDrawString(dpy,win,gc,x,y,str,len) WinDrawString(win,gc,x,y,str,len)
#define XTextExtents(font,str,len,dum1,w,h,dum2) WinTextExtents(font,str,len,w,h)
#define XDisplayString(dpy) "unknown:0.0"
#define XQueryColors(dpy,cmap,cl,n) WinQueryColors(cl,n)
#define XLookupColor(dpy,cmap,cname,exact,color) WinLookupColor(cname,color)
#define XtAppMainLoop(app) WinAppMainLoop()
#define XtIsRealized(w) True
#define XPutImage(dpy,win,gc,img,srcx,srcy,destx,desty,w,h) WinPutImage(win,gc,img,srcx,srcy,destx,desty,w,h)
#define XFreeColorMap(dpy,cmap)
#define XtDestroyWidget(w) DeleteObject(w)
#define XtTranslateCoords(win,x,y,rx,ry) WinTranslateCoords(win,x,y,rx,ry)
#define XStringToKeysym(str) WinStringToKey(str)
#define XInternAtom(dpy,str,flag) AddAtom(str)
#define XSetClipMask(dpy,gc,pix) WinSetClipMask(gc,pix)
#define XSetClipOrigin(dpy,gc,x,y)
#define XtAddEventHandler(win,mask,flag,proc,data) WinAddEventHandler(win,mask,proc,data)
#define XtRemoveEventHandler(win,mask,flag,proc,data) WinRemoveEventHandler(win,mask,proc,data)
#define XtAddCallback(win,mask,proc,data) WinAddCallback(win,mask,proc,data)
#define XtRemoveCallback(win,mask,proc,data) WinRemoveCallback(win,mask,proc,data)
#define XtRemoveAllCallbacks(win,event)
#define XwTextCopyBuffer(win) WinGetText(win)
#define XwTextClearBuffer(win) WinClearText(win)
#define XLookupString(event,str,size,keyptr,flag) WinLookupString(event,keyptr)
#define XwPostPopup(shell,menu,button,dx,dy) WinPostPopup(menu,button,dx,dy)
#define XtOverrideTranslations(win,table,def)

void WinWarpPointer(HWND w, int x, int y);
int WinAddTimeOut(int delay, void(*proc)(), void* data);
void WinRemoveTimeOut(int ID);
int WinCheckWindowEvent(HWND w, int mask, XEvent *event);
void WinGetValues(Widget win, Arg *args, int n);
void WinSetValues(Widget win, Arg *args, int n);
HBITMAP WinCreatePixmap(HWND w, int width, int height);
XImage* WinCreateImage(int width, int height);
void WinSetLineAttributes(GC gc, int width, int style, int cap, int join);
GC WinCreateGC(HWND w, int mask, XGCValues *values);
void WinFreeGC(GC gc);
Widget WinParent(Widget w);
Widget WinNameToWindow(Widget parent, const char *name);
Widget WinCreateWidget(const char *name, int cls, Widget parent, Arg *args, int n, int show);
Widget WinCreatePopup(const char *name, Arg *args, int n);
void WinPopup(Widget w);
char* WinKeyToString(int ks);
HCURSOR WinCreateCursor(HBITMAP src, HBITMAP mask, int xhot, int yhot);
HBITMAP WinCreateBitmapFromData(HWND w, char *data, int width, int height);
HCURSOR WinCreateStandardCursor(char *type);
HWND WinCreateTopLevel(const char *name, int *argc, char **argv);
void WinSetCursor(HWND w, HCURSOR cur);
void WinFreePixmap(Pixmap w);
int WinDesktopWidth(void);
int WinDesktopHeight(void);
int WinGetPixel(XImage *img, int x, int y);
void WinPutPixel(XImage *img, int x, int y, int c);
void WinDrawArc(HWND win, GC gc, int x, int y, int w, int h, int a1, int a2);
void WinDrawLines(HWND win, GC gc, XPoint* pathlist, int number);
void WinFillPolygon(HWND win, GC gc, XPoint* pathlist, int number);
void WinClearArea(HWND win, int x, int y, int w, int h);
void WinFillRectangle(HWND win, GC gc, int x, int y, int w, int h);
void WinSetForeground(GC gc, int rgb);
int WinTextWidth(XFontStruct *f, char *s, int len);
void WinSetFillStyle(GC gc, int style);
void WinSetStipple(GC gc, Pixmap pattern);
void WinSetFunction(GC gc, int op);
void WinDrawLine(HWND win, GC gc, int x1, int y1, int x2, int y2);
void WinDrawPoint(HWND win, GC gc, int x, int y);
void WinSetBackground(GC gc, int rgb);
void WinCopyArea(Pixmap buf, HWND win, GC gc, int x, int y, int w, int h, int offx, int offy);
void WinSetDashes(GC gc, int offset, char *dash, int n);
void WinQueryPointer(HWND w, int *x, int *y);
void WinDrawRectangle(HWND w, GC gc, int x, int y, int width, int height);
void WinDrawString(HWND win, GC gc, int x, int y, const char *str, int len);
void WinTextExtents(XFontStruct *fs, char *str, int len, int *width, int *height);
void WinQueryColors(XColor *cl, int n);
void WinAppMainLoop(void);
void WinPutImage(HWND win, GC gc, XImage *img, int srcx, int srcy, int destx, int desty, int width, int height);
void WinTranslateCoords(Widget win, short x, short y, short *rx, short *ry);
int WinStringToKey(const char *str);
void WinAddEventHandler(Widget win, int event, void(*proc)(), void *data);
void WinRemoveEventHandler(Widget win, int event, void(*proc)(), void *data);
void WinAddCallback(Widget win, int event, void(*proc)(), void *data);
int WinFindCallback(Widget win, int event, void(**proc)(), void **data);
void WinRemoveCallback(Widget win, int event, void(*proc)(), void *data);
void WinRemoveAllCallbacks(Widget win, int event);
char* WinGetText(Widget win);
void WinClearText(Widget win);
int WinNamedColor(const char *cname);
int WinLookupColor(const char *cname, XColor *color);
char* WinColorName(int c);
HWND WinWindow(Widget w);
void WinManageChild(Widget w);
void WinUnmanageChild(Widget w);
void WinLookupString(XKeyEvent *event, KeySym *key);
void WinPostPopup(Widget menu, Widget button, int dx, int dy);
const char* WinBuiltinsDir(void);
Widget WinGetMenu(const char *name);
const char* WinName(Widget w);
int WinIsPopupMenu(Widget w);

#define KPMOD	0x1000
#define VKMOD	0x2000

#define XK_Control_L	(VKMOD|VK_CONTROL)
#define XK_Control_R	(VKMOD|VK_CONTROL)
#define XK_Alt_L	(VKMOD|VK_MENU)
#define XK_Alt_R	(VKMOD|VK_MENU)
#define XK_Shift_L	(VKMOD|VK_SHIFT)
#define XK_Shift_R	(VKMOD|VK_SHIFT)
#define XK_Caps_Lock	(VKMOD|VK_CAPITAL)
#define XK_Num_Lock	(VKMOD|VK_NUMLOCK)

#define XK_a 'a'
#define XK_b 'b'
#define XK_c 'c'
#define XK_d 'd'
#define XK_e 'e'
#define XK_f 'f'
#define XK_g 'g'
#define XK_h 'h'
#define XK_i 'i'
#define XK_j 'j'
#define XK_k 'k'
#define XK_l 'l'
#define XK_m 'm'
#define XK_n 'n'
#define XK_o 'o'
#define XK_p 'p'
#define XK_q 'q'
#define XK_r 'r'
#define XK_s 's'
#define XK_t 't'
#define XK_u 'u'
#define XK_v 'v'
#define XK_w 'w'
#define XK_x 'x'
#define XK_y 'y'
#define XK_z 'z'
#define XK_A 'A'
#define XK_B 'B'
#define XK_C 'C'
#define XK_D 'D'
#define XK_E 'E'
#define XK_F 'F'
#define XK_G 'G'
#define XK_H 'H'
#define XK_I 'I'
#define XK_J 'J'
#define XK_K 'K'
#define XK_L 'L'
#define XK_M 'M'
#define XK_N 'N'
#define XK_O 'O'
#define XK_P 'P'
#define XK_Q 'Q'
#define XK_R 'R'
#define XK_S 'S'
#define XK_T 'T'
#define XK_U 'U'
#define XK_V 'V'
#define XK_W 'W'
#define XK_X 'X'
#define XK_Y 'Y'
#define XK_Z 'Z'
#define XK_Home		(VKMOD|VK_HOME)
#define XK_Delete	(VKMOD|VK_DELETE)
#define XK_Escape	(VKMOD|VK_ESCAPE)
#define XK_F19		(VKMOD|VK_F19)
#define XK_Right	(VKMOD|VK_RIGHT)
#define XK_Left		(VKMOD|VK_LEFT)
#define XK_Down 	(VKMOD|VK_DOWN)
#define XK_Up		(VKMOD|VK_UP)
#define XK_Insert	(VKMOD|VK_INSERT)
#define XK_End		(VKMOD|VK_END)
#define XK_Return	(VKMOD|VK_RETURN)
#define XK_BackSpace	(VKMOD|VK_BACK)
#define XK_Undo		(VKMOD|VK_F6)
#define XK_Redo		(VKMOD|VK_F7)
#define XK_Tab		(VKMOD|VK_TAB)
#define XK_percent	'%'
#define XK_period	'.'
#define XK_bar		'|'
#define XK_greater	'>'
#define XK_less		'<'
#define XK_question	'?'
#define XK_colon	':'
#define XK_underscore	'_'
#define XK_space	' '
#define XK_plus		'+'
#define XK_minus	'-'
#define XK_slash	'/'
#define XK_backslash	'\\'
#define XK_1 '1'
#define XK_2 '2'
#define XK_3 '3'
#define XK_4 '4'
#define XK_5 '5'
#define XK_6 '6'
#define XK_7 '7'
#define XK_8 '8'
#define XK_9 '9'
#define XK_0 '0'
#define XK_KP_1	(KPMOD|XK_1)
#define XK_KP_2	(KPMOD|XK_2)
#define XK_KP_3	(KPMOD|XK_3)
#define XK_KP_4	(KPMOD|XK_4)
#define XK_KP_5	(KPMOD|XK_5)
#define XK_KP_6	(KPMOD|XK_6)
#define XK_KP_7	(KPMOD|XK_7)
#define XK_KP_8	(KPMOD|XK_8)
#define XK_KP_9	(KPMOD|XK_9)
#define XK_KP_0	(KPMOD|XK_0)
#define XK_KP_Enter	(KPMOD|VKMOD|VK_RETURN)
#define XK_KP_Add	(KPMOD|XK_plus)
#define XK_KP_Subtract	(KPMOD|XK_minus)
#define XK_KP_End	(KPMOD|VKMOD|VK_END)
#define XK_KP_Home	(KPMOD|VKMOD|VK_HOME)
#define XK_KP_Up	(KPMOD|VKMOD|VK_UP)
#define XK_KP_Down	(KPMOD|VKMOD|VK_DOWN)
#define XK_KP_Right	(KPMOD|VKMOD|VK_RIGHT)
#define XK_KP_Left	(KPMOD|VKMOD|VK_LEFT)
#define XK_KP_Next	(KPMOD|VKMOD|VK_NEXT)
#define XK_KP_Prior	(KPMOD|VKMOD|VK_PRIOR)
#define XK_KP_Begin	(KPMOD|VKMOD|VK_CLEAR)

struct direct {
	char *d_name;
};
typedef struct {
	HANDLE hnd;
	WIN32_FIND_DATA fd;
	int dirty;
	struct direct d;
} DIR;
DIR* opendir(const char*);
struct direct* readdir(DIR*);
void closedir(DIR*);

#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define va_copy(a,b)	a=b
#define pipe		_pipe
#define strdup		_strdup
#define unlink		_unlink
#define putenv		_putenv
#define fdopen		_fdopen

#define usleep(x) if (x < 1000) WinDebug("Cannot sleep less than 1us\n"); else Sleep(x/1000);

#ifdef USE_WIN32_COM
extern void win32_comscript();
#ifdef USE_WIN32_DOTNET
extern void win32_comdotnet();
#endif
enum win32_events {
	ModifiedChanged,
	ZoomChanged
};
extern void win32_fire_event(int event_type);
#endif

extern void win32_new_window(Widget, void*, void*);

#endif
