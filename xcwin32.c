#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#include "xcwin32.h"
#include "xcircuit.h"
#include "colordefs.h"
#include "resource.h"
#include "menudep.h"
#include "prototypes.h"
#include "tool_bar.h"

#include "xcwin32-colors.h"

#ifndef MK_XBUTTON1
#define MK_XBUTTON1	32
#define MK_XBUTTON2	64
#endif

#if 0
#define W32DEBUG(x) printf##x
#else
#define W32DEBUG(x)
#endif

extern XCWindowData *areawin;
extern Globaldata xobjs;
extern char _STR2[250];
extern Pixmap   STIPPLE[STIPPLES];  /* Polygon fill-style stipple patterns */
static char STIPDATA[STIPPLES][4] = {
   "\000\004\000\001",
   "\000\005\000\012",
   "\001\012\005\010",
   "\005\012\005\012",
   "\016\005\012\007",
   "\017\012\017\005",
   "\017\012\017\016",
   "\000\000\000\000"
};
extern xcWidget top, message2, message3;
extern ApplicationData appdata;
extern int number_colors;
extern colorindex *colorlist;
extern menustruct TopButtons[];
extern short maxbuttons;
extern xcWidget menuwidgets[MaxMenuWidgets];
extern Display *dpy;
extern toolbarstruct ToolBar[];
extern short toolbuttons;
extern Pixmap dbuf, helppix;
extern Dimension helpwidth, helpheight;
extern int helptop;
extern GC hgc;
#define MAXPROPS 7
extern propstruct okstruct[MAXPROPS], fpokstruct;

static HWND corner = NULL, statusBar = NULL;
static Widget toolBar = NULL;
static int statusBarWidth[2] = { 100, -1 };

HWND topwin = NULL;

/*
LRESULT CALLBACK XcStaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK XcEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK XcButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK XcToggleProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK XcPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
*/

INT_PTR CALLBACK OutputDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void updatename(xcWidget button, xcWidgetList callstruct, caddr_t calldata);
void linkset(xcWidget button, propstruct *callstruct, caddr_t calldata);
extern int COM_initialize();
extern int COM_terminate();

#define WIN_WIDGET	0
#define WIN_MENUITEM	1
#define WIN_MENU	2
#define WIN_STATUS	3
#define WIN_TOOLITEM	4

#define IS_WIDGET(w) (w->type == WIN_WIDGET)
#define IS_MENU(w) (w->type == WIN_MENU)
#define IS_MENUITEM(w) (w->type == WIN_MENUITEM)
#define IS_STATUS(w) (w->type == WIN_STATUS)
#define IS_TOOLITEM(w) (w->type == WIN_TOOLITEM)

#define TOWIDGET(w) ((Widget)w)
#define TOMENU(w) ((Menu)w)
#define TOMENUITEM(w) ((MenuItem)w)
#define TOSTATUS(w) ((StatusItem)w)
#define TOTOOLITEM(w) ((ToolItem)w)

#define STATUSBAR_ID	10000
#define TOOLBAR_ID	10001

typedef struct __WinCallback {
	int action;
	void(*proc)();
	void* data;
	struct __WinCallback *next;
} WinCallback;
typedef WinCallback WinEventHandler;

struct __WinWidget {
	int type;
	HWND handle;
	char *name;
	WinCallback *callbacks;
	WinEventHandler *handlers;
	int handler_mask;
	HBITMAP buffer;
	HDC bufhdc;
	WNDPROC wndproc;
};
#define HAS_HANDLER(win,themask) (win->handler_mask & themask)
#define WM_BITBLT (WM_USER+1)

struct __WinMenu {
	int type;
	HMENU handle;
};

struct __WinMenuItem {
	int type;
	struct __WinMenu *parentMenu;
	struct __WinMenu *popup;
	int ID;
	int position;
	char *name;
	WinCallback *callbacks;
	int menudata;
	int toolbar_idx;
};

struct __WinStatusItem {
	int type;
	HWND handle;
	int position;
};

struct __WinToolItem {
	int type;
	struct __WinWidget *toolbar;
	int ID;
	char *name;
	WinCallback *callbacks;
};

typedef struct __WinMenu* Menu;
typedef struct __WinMenuItem* MenuItem;
typedef struct __WinStatusItem* StatusItem;
typedef struct __WinToolItem* ToolItem;

void execute_callback(int action, Widget w, void *calldata)
{
	WinCallback *cb;

	switch (w->type) {
		case WIN_WIDGET:
			cb = w->callbacks;
			break;
		case WIN_MENUITEM:
			cb = ((MenuItem)w)->callbacks;
			break;
		case WIN_TOOLITEM:
			cb = ((ToolItem)w)->callbacks;
			break;
		default:
			return;
	}
	while (cb != NULL) {
		if (cb->action == action) {
			(*cb->proc)(w, cb->data, calldata);
		}
		cb = cb->next;
	}
}

void execute_handler(int mask, Widget w, void *calldata)
{
	WinCallback *h = w->handlers;
	if (!HAS_HANDLER(w,mask))
		return;
	while (h != NULL) {
		if (h->action & mask)
			(*h->proc)(w, h->data, calldata);
		h = h->next;
	}
}

void update_event_mask(Widget win)
{
	WinEventHandler *h = win->handlers;
	win->handler_mask = 0;
	for (; h!= NULL; h=h->next)
		win->handler_mask |= h->action;
}

Widget get_widget(HWND win)
{
	/*
	int nbytes = GetClassLong(win, GCL_CBWNDEXTRA);
	if (nbytes >= sizeof(Widget))
		return (Widget)GetWindowLong(win, nbytes-sizeof(Widget));
	else
		return NULL;
		*/
	return (Widget)GetWindowLong(win, GWL_USERDATA);
}

static void get_value(Widget w, Arg *a)
{
	RECT rect;

	switch (a->type) {
		case XtNwidth:
			if (w->type == WIN_STATUS) {
				StatusItem item = (StatusItem)w;
				if (item->position < 1)
					*(int*)(a->data) = statusBarWidth[item->position];
				else {
					GetClientRect(w->handle, &rect);
					*(int*)(a->data) = rect.right - rect.left - statusBarWidth[0];
				}
			} else {
				if (GetClientRect(w->handle, &rect))
					*(int*)(a->data) = rect.right - rect.left;
			}
			break;
		case XtNheight:
			if (GetClientRect(w->handle, &rect))
				*(int*)(a->data) = rect.bottom - rect.top;
			break;
		case XtNrectColor:
		case XtNrectStipple:
			if (w->type == WIN_MENUITEM) {
				*(int*)(a->data) = ((MenuItem)w)->menudata;
				break;
			}
			goto GETDEFAULT;
GETDEFAULT:
		default:
			W32DEBUG(("Unsupported get value: %d\n", a->type));
			break;
	}
}

static void set_value(Widget w, Arg *a)
{
	switch (a->type) {
		case XtNstring:
			switch (w->type) {
				case WIN_STATUS:
					SendMessage(TOSTATUS(w)->handle, SB_SETTEXT, TOSTATUS(w)->position, (LPARAM)(char*)a->data);
					break;
				case WIN_WIDGET:
					SetWindowText(w->handle, (char*)a->data);
					break;
				default:
					goto SETDEFAULT;
			}
			break;
		case XtNsetMark:
			switch (w->type) {
				case WIN_MENUITEM:
					CheckMenuItem(TOMENUITEM(w)->parentMenu->handle, TOMENUITEM(w)->position,
							MF_BYPOSITION|(a->data == NULL ? MF_UNCHECKED : MF_CHECKED));
					break;
				default:
					goto SETDEFAULT;
			}
			break;
		case XtNlabel:
			switch (w->type) {
				case WIN_MENUITEM:
					{
						MENUITEMINFO mi_info;
						ZeroMemory(&mi_info, sizeof(mi_info));
						mi_info.cbSize = sizeof(mi_info);
						mi_info.fMask = MIIM_STRING;
						mi_info.dwTypeData = a->data;
						SetMenuItemInfo(TOMENUITEM(w)->parentMenu->handle, TOMENUITEM(w)->position, MF_BYPOSITION, &mi_info);
						break;
					}
				case WIN_WIDGET:
					SetWindowText(w->handle, (char*)a->data);
					break;
				default:
					goto SETDEFAULT;
			}
			break;
		case XtNfont:
			switch (w->type) {
				case WIN_WIDGET:
					SendMessage(w->handle, WM_SETFONT, (WPARAM)((XFontStruct*)a->data)->fid, MAKELONG(TRUE, 0));
					break;
				default:
					goto SETDEFAULT;
			}
			break;
		case XtNset:
			switch (w->type) {
				case WIN_WIDGET:
					SendMessage(w->handle, BM_SETCHECK, ((int)a->data == True ? BST_CHECKED : BST_UNCHECKED), 0);
					break;
				default:
					goto SETDEFAULT;
			}
			break;
		case XtNborderColor:
			switch (w->type) {
				case WIN_TOOLITEM:
					if ((int)a->data == RATSNESTCOLOR) {
						W32DEBUG(("Check button: %s\n", ((ToolItem)w)->name));
						SendMessage(TOTOOLITEM(w)->toolbar->handle, TB_CHECKBUTTON, TOTOOLITEM(w)->ID, MAKELONG(TRUE, 0));
					}
					else {
						W32DEBUG(("Uncheck button: %s\n", ((ToolItem)w)->name));
						SendMessage(TOTOOLITEM(w)->toolbar->handle, TB_CHECKBUTTON, TOTOOLITEM(w)->ID, MAKELONG(FALSE, 0));
					}
					break;
				default:
					goto SETDEFAULT;
			}
			break;
SETDEFAULT:
		default:
			W32DEBUG(("Unsupported set value: %d\n", a->type));
			/*exit(-1);*/
			break;
	}
}

void WinGetValues(Widget w, Arg *args, int n)
{
	int i;

	if (w != NULL)
		for (i=0; i<n; i++)
			get_value(w, args+i);
}

void WinSetValues(Widget w, Arg *args, int n)
{
	int i;

	if (w != NULL)
		for (i=0; i<n; i++)
			set_value(w, args+i);
}

HBITMAP WinCreatePixmap(HWND w, int width, int height)
{
	HDC hdc;
	HBITMAP hBitmap;

	hdc = GetDC(w);
	hBitmap = CreateCompatibleBitmap(hdc, width, height);
	ReleaseDC(w, hdc);
	return hBitmap;
}

Widget WinParent(Widget child)
{
	if (child)
		switch (child->type) {
			case WIN_WIDGET:
				return (Widget)get_widget(GetParent(child->handle));
			case WIN_MENUITEM:
				return (Widget)((MenuItem)child)->parentMenu;
			case WIN_TOOLITEM:
				return (Widget)((ToolItem)child)->toolbar;
		}
	return NULL;
}

void WinFreePixmap(Pixmap pix)
{
	DeleteObject(pix);
}

void WinDestroyImage(XImage *img)
{
	if (img->handle != NULL)
		DeleteObject(img->handle);
	free(img->data);
	free(img);
}

XImage* WinCreateImage(int width, int height)
{
	HWND h = GetDesktopWindow();
	HDC hdc = GetDC(h);
	XImage *img = (XImage*)malloc(sizeof(XImage));

	img->bits_per_pixel = GetDeviceCaps(hdc, BITSPIXEL);
	img->bytes_per_line = width * img->bits_per_pixel / 8;
	img->width = width;
	img->height = height;
	img->data = NULL;
	ReleaseDC(h, hdc);

	return img;
}

int WinGetPixel(XImage *img, int x, int y)
{
	unsigned char *src;
	if (img->bits_per_pixel != 32) {
		W32DEBUG(("WinGetPixel only supported on 32bpp\n"));
		return -1;
	}
	src = img->data + (y * img->bytes_per_line) + (x * img->bits_per_pixel) / 8;
	return RGB(src[0], src[1], src[2]);
}

void WinPutPixel(XImage *img, int x, int y, Pixel pix)
{
	unsigned char *src;
	if (img->bits_per_pixel != 32) {
		W32DEBUG(("WinPutPixel only supported on 32bpp\n"));
		return;
	}
	src = img->data + (y * img->bytes_per_line) + (x * img->bits_per_pixel) / 8;
	src[0] = GetRValue(pix);
	src[1] = GetGValue(pix);
	src[2] = GetBValue(pix);
}

DIR* opendir(const char *name)
{
	DIR *d = (DIR*)malloc(sizeof(DIR));
	static char buffer[MAX_PATH];

	strncpy(buffer, name, MAX_PATH);
	strncat(buffer, "\\*", MAX_PATH);
	d->hnd = FindFirstFile(buffer, &(d->fd));
	if (d->hnd == INVALID_HANDLE_VALUE)
		return NULL;
	d->dirty = 1;
	return d;
}

void closedir(DIR *d)
{
	free(d);
}

struct direct* readdir(DIR *d)
{
	if (!d->dirty)
	{
		if (!FindNextFile(d->hnd, &(d->fd)))
			return NULL;
	}
	d->d.d_name = d->fd.cFileName;
	d->dirty = 0;
	return &(d->d);
}

int WinDesktopWidth(void)
{
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);
	return r.right-r.left;
}

int WinDesktopHeight(void)
{
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);
	return r.bottom-r.top;
}

void WinSetCursor(HWND win, HCURSOR cursor)
{
	SetClassLong(win, GCL_HCURSOR, (LONG)cursor);
}

HDC cached_hdc = NULL;
HWND cached_win = NULL;

static HPEN create_pen(GC gc)
{
	HPEN hPen = NULL;

	if (gc->line_style != LineSolid && gc->line_dash_len > 0 && gc->line_dash != NULL) {
		DWORD *dashes = NULL;
		DWORD style = 0;
		LOGBRUSH logBrush;
		int i;

		logBrush.lbStyle = BS_SOLID;
		logBrush.lbColor = gc->foreground;
		logBrush.lbHatch = 0;

		style |= PS_GEOMETRIC | PS_USERSTYLE | gc->line_cap | gc->line_join;
		dashes = (DWORD*)malloc(sizeof(DWORD)*gc->line_dash_len);
		for (i=0; i<gc->line_dash_len; i++)
			dashes[i] = gc->line_dash[i];
		hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE | gc->line_cap | gc->line_join,
				gc->line_width, &logBrush, gc->line_dash_len, dashes);
		free(dashes);
	} else {
		hPen = CreatePen(PS_SOLID, gc->line_width, gc->foreground);
	}

	return hPen;
}

static HBRUSH create_brush(GC gc)
{
	LOGBRUSH logBrush;
	logBrush.lbStyle = (gc->fill_style == FillSolid ? BS_SOLID : BS_PATTERN);
	logBrush.lbColor = gc->foreground;
	logBrush.lbHatch = (LONG)(gc->fill_style != FillSolid ? gc->fill_stipple : NULL);
	return CreateBrushIndirect(&logBrush);
}

static HDC get_hdc(HWND win, GC gc)
{
	HDC hdc;
	if (IsWindow(win)) {
		Widget winwidget = (Widget)get_widget(win);
		if (winwidget == NULL || winwidget->buffer == NULL)
			hdc = GetDC(win);
		else
			hdc = winwidget->bufhdc;
	} else if (win == cached_win && cached_hdc != NULL)  {
		hdc = cached_hdc;
	} else {
		hdc = CreateCompatibleDC(NULL);
		SelectObject(hdc, win);
		if (cached_hdc != NULL)
			DeleteDC(cached_hdc);
		cached_hdc = hdc;
		cached_win = win;
	}
	if (gc != NULL) {
		SetROP2(hdc, gc->function);
		SetBkColor(hdc, gc->background);
		SetTextColor(hdc, gc->foreground);
		SetBkMode(hdc, TRANSPARENT);
		SelectObject(hdc, create_pen(gc));
		SelectObject(hdc, create_brush(gc));
		SelectObject(hdc, gc->font);
	}
	return hdc;
}

static release_hdc(HWND win, HDC hdc)
{
	DeleteObject(SelectObject(hdc, GetStockObject(BLACK_PEN)));
	DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
	if (IsWindow(win)) {
		Widget winwidget = (Widget)get_widget(win);
		if (winwidget == NULL || winwidget->buffer == NULL)
			ReleaseDC(win, hdc);
		else
			PostMessage(win, WM_BITBLT, 0, 0);
	}
	else {
		//DeleteDC(hdc);
	}
}

void WinDrawLine(HWND win, GC gc, int x1, int y1, int x2, int y2)
{
	HDC hdc = get_hdc(win, gc);
	MoveToEx(hdc, x1, y1, NULL);
	LineTo(hdc, x2, y2);
	release_hdc(win, hdc);
}

void WinFillPolygon(HWND win, GC gc, XPoint* pathlist, int number)
{
	LPPOINT pts = (LPPOINT)malloc(sizeof(POINT)*number);
	HDC hdc = get_hdc(win, gc);
	RECT rect = { 0x0000ffff, 0x0000ffff, -0x0000ffff, -0x0000ffff };
	int i;

	DeleteObject(SelectObject(hdc, CreatePen(PS_NULL, 0, 0)));

	for (i=0; i<number; i++) {
		pts[i].x = pathlist[i].x;
		pts[i].y = pathlist[i].y;
		rect.left = min(rect.left, pts[i].x);
		rect.top = min(rect.top, pts[i].y);
		rect.right = max(rect.right, pts[i].x);
		rect.bottom = max(rect.bottom, pts[i].y);
	}
	if (gc->fill_style == FillStippled) {
		int width = rect.right-rect.left, height = rect.bottom-rect.top;
		HDC tmphdc = CreateCompatibleDC(hdc);
		HBITMAP tmpbmp = CreateCompatibleBitmap(hdc, width, height);
		HBRUSH tmpbrush = CreateSolidBrush(gc->foreground);

		SelectObject(tmphdc, tmpbmp);
		SelectObject(tmphdc, tmpbrush);
		SetROP2(tmphdc, gc->function);
		BitBlt(tmphdc, 0, 0, width, height, hdc, rect.left, rect.top, SRCCOPY);
		for (i=0; i<number; i++) {
			pts[i].x -= rect.left;
			pts[i].y -= rect.top;
		}
		Polygon(tmphdc, pts, number);
		BitBlt(hdc, rect.left, rect.top, width, height, tmphdc, 0, 0, 0x00AC0744);

		DeleteDC(tmphdc);
		DeleteObject(tmpbmp);
		DeleteObject(tmpbrush);
	} else {
		Polygon(hdc, pts, number);
	}

	release_hdc(win, hdc);
	free(pts);
}

void WinDrawArc(HWND win, GC gc, int x, int y, int width, int height, int angle, int span)
{
	HDC hdc = get_hdc(win, gc);
	if (span != (360*64)) {
		W32DEBUG(("Unimplemented\n"));
	} else
		Ellipse(hdc, x, y, width, height);
	release_hdc(win, hdc);
}

void WinClearArea(HWND win, int x, int y, int w, int h)
{
	HDC hdc = get_hdc(win, NULL);
	RECT r;
	r.left = x;
	r.top = y;
	r.right = x+w;
	r.bottom = y+h;
	FillRect(hdc, &r, (HBRUSH)(COLOR_WINDOW+1));
	release_hdc(win, hdc);
}

void WinFillRectangle(HWND win, GC gc, int x, int y, int w, int h)
{
	HDC hdc = get_hdc(win, gc);
	RECT r;
	r.left = x;
	r.top = y;
	r.right = x+w;
	r.bottom = y+h;
	FillRect(hdc, &r, (HBRUSH)GetCurrentObject(hdc, OBJ_BRUSH));
	release_hdc(win, hdc);
}

void WinSetFunction(GC gc, int op)
{
	gc->function = op;
}

void WinSetForeground(GC gc, int color)
{
	gc->foreground = color;
}

void WinSetBackground(GC gc, int rgb)
{
	gc->background = rgb;
}

void WinCopyArea(Pixmap pix, HWND win, GC gc, int x, int y, int width, int height, int offx, int offy)
{
	HDC pixhdc = get_hdc((HWND)pix, gc), hdc = get_hdc(win, gc);
	BitBlt(hdc, offx, offy, width, height, pixhdc, x, y, SRCCOPY);
	release_hdc((HWND)pix, pixhdc);
	release_hdc(win, hdc);
}

void WinDrawPoint(HWND win, GC gc, int x, int y)
{
	HDC hdc = get_hdc(win, gc);
	SetPixelV(hdc, x, y, gc->foreground);
	release_hdc(win, hdc);
}

void WinSetLineAttributes(GC gc, int width, int style, int capstyle, int joinstyle)
{
	gc->line_style = style;
	gc->line_width = width;
	gc->line_cap = capstyle;
	gc->line_join = joinstyle;
}

void WinWarpPointer(HWND w, int x, int y)
{
	POINT pt;

	pt.x = x;
	pt.y = y;
	ClientToScreen(w, &pt);
	SetCursorPos(pt.x, pt.y);
}

void WinDrawLines(HWND win, GC gc, XPoint* pathlist, int number)
{
	LPPOINT pts = (LPPOINT)malloc(sizeof(POINT)*number);
	HDC hdc = get_hdc(win, gc);
	int i;

	for (i=0; i<number; i++) {
		pts[i].x = pathlist[i].x;
		pts[i].y = pathlist[i].y;
	}
	Polyline(hdc, pts, number);
	release_hdc(win, hdc);
	free(pts);
}

void WinQueryPointer(HWND win, int *x, int *y)
{
	POINT pt;

	if (GetCursorPos(&pt)) {
		ScreenToClient(win, &pt);
		*x = pt.x;
		*y = pt.y;
	}
}

void WinDrawRectangle(HWND win, GC gc, int x, int y, int width, int height)
{
	HDC hdc = get_hdc(win, gc);
	Rectangle(hdc, x, y, x+width, y+height);
	release_hdc(win, hdc);
}

GC WinCreateGC(HWND win, int mask, XGCValues *values)
{
	GC gc = (GC)malloc(sizeof(struct __GC));

	gc->foreground = RGB(0, 0, 0);
	gc->background = RGB(255, 255, 255);
	gc->function = R2_COPYPEN;
	gc->font = NULL;
	gc->graphics_exposures = False;
	gc->fill_style = BS_SOLID;
	gc->fill_stipple = NULL;
	gc->line_width = 0;
	gc->line_style = PS_SOLID;
	gc->line_cap = PS_ENDCAP_ROUND;
	gc->line_join = PS_JOIN_BEVEL;
	gc->line_dash_len = 0;
	gc->line_dash = NULL;
	gc->clipMask = NULL;

	if (mask & GCForeground)
		gc->foreground = values->foreground;
	if (mask & GCBackground)
		gc->background = values->background;
	if (mask & GCFont)
		gc->font = values->font;
	if (mask & GCFunction)
		gc->function = values->function;
	if (mask & GCGraphicsExposures)
		gc->graphics_exposures = values->graphics_exposures;

	return gc;
}

void WinFreeGC(GC gc)
{
	free(gc);
}

void WinSetDashes(GC gc, int offset, char *dash, int n)
{
	if (gc->line_dash) {
		free(gc->line_dash);
		gc->line_dash = NULL;
		gc->line_dash_len = 0;
	}
	if (n > 0 && dash != NULL) {
		gc->line_dash = (char*)malloc(sizeof(char)*n);
		memcpy(gc->line_dash, dash+offset, n);
		gc->line_dash_len = n;
	}
}

void WinSetFillStyle(GC gc, int style)
{
	gc->fill_style = style;
}

void WinSetStipple(GC gc, Pixmap pattern)
{
	gc->fill_stipple = pattern;
}

HCURSOR WinCreateStandardCursor(char *cur)
{
	return LoadCursor(NULL, cur);
}

void WinDrawString(HWND win, GC gc, int x, int y, const char *str, int len)
{
	HDC hdc = get_hdc(win, gc);
	SIZE sz;
	RECT r;

	/*
	GetTextExtentPoint32(hdc, str, len, &sz);
	r.left = x;
	r.right = x+sz.cx;
	r.top = y-sz.cy;
	r.bottom = y;
	TextOut(hdc, r.left, r.top, str, len);
	*/
	SetTextAlign(hdc, TA_LEFT|TA_BASELINE);
	TextOut(hdc, x, y, str, len);
	release_hdc(win, hdc);
}

int WinTextWidth(XFontStruct *fn, char *s, int len)
{
	HWND w = GetDesktopWindow();
	HDC hdc = GetDC(w);
	SIZE sz;

	SelectObject(hdc, fn->fid);
	GetTextExtentPoint32(hdc, s, len, &sz);
	ReleaseDC(w, hdc);

	return sz.cx;
}

void WinTextExtents(XFontStruct *fn, char *str, int len, int *width, int *height)
{
	HWND w = GetDesktopWindow();
	HDC hdc = GetDC(w);
	SIZE sz;

	SelectObject(hdc, fn->fid);
	GetTextExtentPoint32(hdc, str, len, &sz);
	ReleaseDC(w, hdc);

	*width = sz.cy;
	*height = 0;
}

#define DUPBITS(x) (((x)<<8)|(x))

void WinQueryColors(XColor *colors, int n)
{
	int i;

	for (i=0; i<n; i++) {
		colors[i].red = DUPBITS(GetRValue(colors[i].pixel));
		colors[i].green = DUPBITS(GetGValue(colors[i].pixel));
		colors[i].blue = DUPBITS(GetBValue(colors[i].pixel));
	}
}

void WinAppMainLoop(void)
{
	MSG msg;
	BOOL bRet;

	DrawMenuBar(top->handle);
	ShowWindow(top->handle, SW_SHOWNORMAL);
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) {
			W32DEBUG(("Loop error\n"));
			break;
		} else {
			if (GetAncestor(msg.hwnd, GA_ROOT) != topwin) {
				TranslateMessage(&msg);
			}
			DispatchMessage(&msg);
		}
	}

#ifdef USE_WIN32_COM
	COM_terminate();
#endif
}

void WinPutImage(HWND win, GC gc, XImage *img, int srcx, int srcy, int destx, int desty, int width, int height)
{
	HDC pixhdc, hdc;

	if (img->handle == NULL) {
		W32DEBUG(("Invalid image handle\n"));
		return;
	}

	pixhdc = get_hdc((HWND)img->handle, gc);
	hdc = get_hdc(win, gc);
	if (gc->clipMask == NULL)
		BitBlt(hdc, destx, desty, width, height, pixhdc, srcx, srcy, SRCCOPY);
	else
		MaskBlt(hdc, destx, desty, width, height, pixhdc, srcx, srcy, gc->clipMask, 0, 0, SRCCOPY);
	release_hdc((HWND)img->handle, pixhdc);
	release_hdc(win, hdc);
}

void WinSetClipMask(GC gc, Pixmap pix)
{
	gc->clipMask = pix;
}

typedef struct __win32_timer { int ID; void(*proc)(); void *data; struct __win32_timer *next; } win32_timer;
static win32_timer *timers = NULL;

VOID CALLBACK WinTimerProc(HWND hwnd, UINT umsg, UINT id, DWORD dwtime)
{
	win32_timer *t = timers, *prev = NULL;

	for (; t!=NULL && t->ID != id; prev=t, t=t->next);
	if (t == NULL) {
		W32DEBUG(("Timer %d not found\n", id));
		return;
	}
	if (prev == NULL)
		timers = t->next;
	else
		prev->next = t->next;
	KillTimer(NULL, t->ID);
	(*t->proc)(t->data, &(t->ID));
	free(t);
}

int WinAddTimeOut(int delay, void(*proc)(), void *data)
{
	win32_timer* t = (win32_timer*)malloc(sizeof(win32_timer));

	t->proc = proc;
	t->data = data;
	t->ID = SetTimer(NULL, 0, delay, WinTimerProc);

	if (t->ID == 0) {
		free(t);
		return -1;
	} else {
		t->next = timers;
		timers = t;
		return t->ID;
	}
}

void WinRemoveTimeOut(int id)
{
	win32_timer *t = timers, *prev = NULL;

	for (; t!=NULL && t->ID != id; prev=t, t=t->next);
	if (t == NULL) {
		return;
	}
	if (prev == NULL)
		timers = t->next;
	else
		prev->next = t->next;
	KillTimer(NULL, t->ID);
	free(t);
}

void WinTranslateCoords(Widget win, short x, short y, short *rx, short *ry)
{
	POINT pt;

	pt.x = x;
	pt.y = y;
	ClientToScreen(win->handle, &pt);
	*rx = pt.x;
	*ry = pt.y;
}

typedef struct {
	char *request;
	char *current;
} InputDlgData;

BOOL CALLBACK InputDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			{
				RECT r1, r2;
				int w1, w2, h1, h2;
				
				SetDlgItemText(hwnd, IDC_EDIT1, ((InputDlgData*)lParam)->current);
				SetDlgItemText(hwnd, IDC_INFOTEXT, ((InputDlgData*)lParam)->request);
				GetWindowRect(GetParent(hwnd), &r1);
				GetWindowRect(hwnd, &r2);
				w1 = r1.right-r1.left;
				w2 = r2.right-r2.left;
				h1 = r1.bottom-r1.top;
				h2 = r2.bottom-r2.top;
				SetWindowPos(hwnd, HWND_TOP, r1.left+(w1-w2)/2, r1.top+(h1-h2)/2, 0, 0, SWP_NOSIZE);
				SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
				SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
				return FALSE;
			}
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
				switch (LOWORD(wParam)) {
					case IDOK:
						GetDlgItemText(hwnd, IDC_EDIT1, _STR2, sizeof(_STR2));
						EndDialog(hwnd, 1);
						return TRUE;
					case IDCANCEL:
						EndDialog(hwnd, 0);
						return TRUE;
				}
			break;
	}
	return FALSE;
}

#define HELP_MARGIN 10

BOOL CALLBACK HelpDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			{
				RECT br, r, cr, pr, pcr;
				int sbwidth;

				GetWindowRect(hwnd, &r);
				GetClientRect(hwnd, &cr);
				GetClientRect(GetDlgItem(hwnd, IDOK), &br);
				GetWindowRect(GetDlgItem(hwnd, IDC_HELPPIX), &pr);
				GetClientRect(GetDlgItem(hwnd, IDC_HELPPIX), &pcr);
				sbwidth = (pr.right-pr.left)-(pcr.right-pcr.left);
				SetWindowPos(hwnd, HWND_TOP, 0, 0, sbwidth+helpwidth+2*HELP_MARGIN+(r.right-r.left)-(cr.right-cr.left),
						r.bottom-r.top, SWP_NOMOVE);
				GetClientRect(hwnd, &cr);
				SetWindowPos(GetDlgItem(hwnd, IDC_HELPPIX), HWND_TOP, HELP_MARGIN, HELP_MARGIN, helpwidth+sbwidth,
						(cr.bottom-cr.top)-(br.bottom-br.top)-3*HELP_MARGIN, 0);
				SetWindowPos(GetDlgItem(hwnd, IDOK), HWND_TOP,
						HELP_MARGIN+((cr.right-cr.left)-(br.right-br.left))/2, cr.bottom-HELP_MARGIN-(br.bottom-br.top),
						br.right-br.left, br.bottom-br.top, 0);
				SetFocus(GetDlgItem(hwnd, IDC_HELPPIX));
			}
			return FALSE;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
				switch (LOWORD(wParam)) {
					case IDCANCEL:
					case IDOK:
						EndDialog(hwnd, 1);
						return TRUE;
				}
			break;
		default:
			break;
	}
	return FALSE;
}

void popupprompt(Widget button, char *request, char *current, void (*function)(), buttonsave *datastruct, const char *filter)
{
	if (current != NULL && filter == NULL) {
		InputDlgData data;

		data.request = request;
		data.current = current;
		if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INPUTDIALOG), areawin->window, InputDlgProc, (LPARAM)&data))
			(*function)(button, (datastruct != NULL ? datastruct->dataptr : NULL));
	} else if (filter != NULL) {
		OPENFILENAME ofn;
		char filename[1024] = {0};
		char filterspec[1024] = {0};

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = areawin->window;
		if (*filter) {
			_snprintf(filterspec, 1024, "Enable Filter (*%s)%c*%s%cDisable Filter (*.*)%c*.*%c%c%c",
				       	filter, 0, filter, 0, 0, 0, 0, 0);
			ofn.lpstrFilter = filterspec;
		} else
			ofn.lpstrFilter = "All Files (*.*)\000*.*\000\000\000";
		ofn.lpstrFile = filename;
		ofn.nMaxFile = 1024;
		if (GetOpenFileName(&ofn)) {
			char *c;
			strcpy(_STR2, ofn.lpstrFile);
			for (c=_STR2; *c; c++)
				if (*c == '\\') *c = '/';
			(*function)(button, datastruct->dataptr);
		}
	} else {
		if (MessageBox(areawin->window, request, "XCircuit", MB_YESNO|MB_ICONQUESTION) == IDYES)
			(*function)(button, datastruct->dataptr);
	}

	if (datastruct != NULL) free(datastruct);
}

static void get_vkey_name(int key, char *str, int len)
{
	UINT scan = MapVirtualKey(key, 0);
	scan = (scan&0xff)<<16;
	switch (key) {
		case VK_HOME:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_END:
		case VK_DELETE:
		case VK_INSERT:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			scan |= (1<<24);
			break;
		case VK_CLEAR:
			memcpy(str, "CLEAR", 5);
			return;
		default:
			break;
	}
	GetKeyNameText(scan, str, len);
}

char *WinKeyToString(int key)
{
	static char buffer[1024];

	memset(buffer, 0, 1024);
	if (key >= XK_a && key <= XK_z)
		buffer[0] = (char)key;
	else if (key >= XK_A && key <= XK_Z)
		buffer[0] = (char)key;
	else {
		int offset = 0;

		if (key & KPMOD) {
			strcpy(buffer, "Keypad_");
			offset = 7;
		}
		if (key & VKMOD)
			get_vkey_name(key&0x00ff, buffer+offset, 1024-offset);
		else
			buffer[offset] = key&0x00ff;
	}

	return buffer;
}

int WinStringToKey(const char *str)
{
	int key;
	char buf[256];

	for (key=1; key<256; key++) {
		memset(buf, 0, 256);
		get_vkey_name(key, buf, 256);
		if (strcmp(str, buf) == 0)
			return key;
	}
	return 0;
}

static Widget WndFound;

BOOL CALLBACK WndLookupProc(HWND hwnd, LPARAM lParam)
{
	Widget w = (Widget)get_widget(hwnd);

	if (w != NULL && strcmp((char*)lParam, w->name) == 0) {
		WndFound = w;
		W32DEBUG(("widget name: '%s'\n", w->name));
		return FALSE;
	}
	W32DEBUG(("widget name: '%s' (%p)\n", (w ? w->name : (char*)w)));
	return TRUE;
}

const char* WinName(Widget w)
{
	switch (w->type) {
		case WIN_WIDGET:
			return w->name;
		case WIN_MENUITEM:
			return TOMENUITEM(w)->name;
		case WIN_TOOLITEM:
			return TOTOOLITEM(w)->name;
		default:
			return NULL;
	}
}

Widget WinNameToWindow(Widget parent, const char *name)
{
	W32DEBUG(("Looking for '%s' in %p\n", name, parent));
	WndFound = NULL;
	if (parent != NULL)
		switch (parent->type) {
			case WIN_WIDGET:
				if (parent == toolBar) {
					TBBUTTON tb;
					int nbuttons = SendMessage(toolBar->handle, TB_BUTTONCOUNT, 0, 0), i;
					for (i=0; i<nbuttons; i++) {
						ZeroMemory(&tb, sizeof(tb));
						SendMessage(toolBar->handle, TB_GETBUTTON, i, (LPARAM)&tb);
						if (tb.dwData != 0) {
							ToolItem item = (ToolItem)tb.dwData;
							if (item->name && strcmp(item->name, name) == 0) {
								WndFound = (Widget)item;
								break;
							}
						}
					}
				} else
					EnumChildWindows(parent->handle, WndLookupProc, (LPARAM)name);
				break;
			case WIN_MENUITEM:
				if (TOMENUITEM(parent)->popup != NULL)
					parent = (Widget)TOMENUITEM(parent)->popup;
				else
					break;
			case WIN_MENU:
				{
					int i, n = GetMenuItemCount((HMENU)parent->handle);
					MENUITEMINFO mi_info;

					ZeroMemory(&mi_info, sizeof(mi_info));
					mi_info.cbSize = sizeof(mi_info);
					mi_info.fMask = MIIM_DATA;
					for (i=0; i<n; i++) {
						mi_info.dwItemData = (LONG)NULL;
						if (GetMenuItemInfo((HMENU)parent->handle, i, MF_BYPOSITION, &mi_info)
						    && mi_info.dwItemData && ((MenuItem)mi_info.dwItemData)->name
						    && strcmp(((MenuItem)mi_info.dwItemData)->name, name) == 0) {
							WndFound = (Widget)mi_info.dwItemData;
							break;
						}
					}
				}
				break;
		}
	return WndFound;
}

void outputpopup(Widget button, caddr_t clientdata, caddr_t calldata)
{
	if (is_page(topobject) == -1) {
		Wprintf("Can only save a top-level page!");
		return;
	}
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_OUTPUTDLG), top->handle, OutputDlgProc);
}

typedef struct {
	char *bits;
	int width, height;
} cursor_bits;

static u_char reverse_byte(u_char c)
{
	u_char rc = 0;
	int i;

	for (i=(sizeof(char)*8-1); i>=0; i--, c>>=1)
		rc |= (c&0x01) << i;
	return rc;
}

static void compute_cursor_src_mask(u_char *src, u_char *mask)
{
	u_char pixsrc = *src, pixmask = *mask;
	*src = ~(reverse_byte(pixmask));
	*mask = reverse_byte((u_char)((~pixsrc&0xff)&pixmask));
}

static HBITMAP create_stipple(char *stipdata)
{
	HBITMAP hBitmap;
	BYTE data[8];
	int i;

	for (i=0; i<4; i++) {
		data[2*i] = 0xff & ~reverse_byte(stipdata[i]);
	}
	hBitmap = CreateBitmap(4, 4, 1, 1, data);
	return hBitmap;
}

HBITMAP WinCreateBitmapFromData(HWND w, char *data, int width, int height)
{
	if (width == 4 && height == 4) {
		return create_stipple(data);
	} else {
		cursor_bits *cbits = (cursor_bits*)malloc(sizeof(cursor_bits));
		cbits->bits = data;
		cbits->width = width;
		cbits->height = height;
		return (HBITMAP)cbits;
	}
}

HCURSOR WinCreateCursor(HBITMAP _src, HBITMAP _mask, int xhot, int yhot)
{
	HCURSOR hCursor = NULL;
	u_char *src = ((cursor_bits*)_src)->bits, *mask = ((cursor_bits*)_mask)->bits;
	u_char *new_src, *new_mask;
	int width = ((cursor_bits*)_src)->width, height = ((cursor_bits*)_src)->height;
	int nb = (width-1)/(8*sizeof(char))+1;
	int nb2 = (GetSystemMetrics(SM_CXCURSOR)-1)/(8*sizeof(char))+1, height2 = GetSystemMetrics(SM_CYCURSOR);
	int i, j, idx1 = 0, idx2 = 0;

	new_src = (u_char*)malloc(sizeof(char)*nb2*height2);
	new_mask = (u_char*)malloc(sizeof(char)*nb2*height2);

	for (j=0; j<height; j++) {
		for (i=0; i<nb; i++, idx1++, idx2++) {
			new_src[idx2] = src[idx1];
			new_mask[idx2] = mask[idx1];
			compute_cursor_src_mask(&new_src[idx2], &new_mask[idx2]);
		}
		for (i=0; i<(nb2-nb); i++, idx2++) {
			new_src[idx2] = 0xff;
			new_mask[idx2] = 0x00;
		}
	}
	for (j=0; j<(height2-height); j++) {
		for (i=0; i<nb2; i++, idx2++) {
			new_src[idx2] = 0xff;
			new_mask[idx2] = 0x00;
		}
	}

	hCursor = CreateCursor(GetModuleHandle(NULL), xhot, yhot, nb2*8, height2, new_src, new_mask);

	free(new_src);
	free(new_mask);
	free(_src);
	free(_mask);

	return hCursor;
}

static Widget create_widget(HWND handle, char *name)
{
	Widget w = (Widget)malloc(sizeof(struct __WinWidget));

	ZeroMemory(w, sizeof(struct __WinWidget));
	w->type = WIN_WIDGET;
	w->handle = handle;
	w->name = strdup(name);
	if (strcmp(name, "Area") == 0) {
		RECT r;
		GetWindowRect(handle, &r);
		w->buffer = WinCreatePixmap(handle, r.right-r.left, r.bottom-r.top);
		w->bufhdc = CreateCompatibleDC(NULL);
		SelectObject(w->bufhdc, w->buffer);
	}
	SetWindowLong(handle, GWL_USERDATA, (LONG)w);
	return w;
}

static void destroy_widget(HWND handle)
{
	Widget w = (Widget)get_widget(handle);
	WinCallback *cb = w->callbacks, *free_cb;
	WinEventHandler *eh = w->handlers, *free_eh;

	free(w->name);
	while (cb != NULL) {
		free_cb = cb;
		cb = cb->next;
		free(free_cb);
	}
	while (eh != NULL) {
		free_eh = eh;
		eh = eh->next;
		free(free_eh);
	}
	if (w->wndproc != NULL)
		SetWindowLong(handle, GWL_WNDPROC, (LONG)w->wndproc);
	free(w);
}

LRESULT CALLBACK XcTopWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CLOSE:
			quitcheck(NULL, NULL, NULL);
			return 0;
		case WM_ACTIVATE:
			if (wParam != 0) {
				XCWindowData *w;

				for (w=xobjs.windowlist; w!=NULL; w=w->next)
					if (GetAncestor(w->window, GA_ROOT) == hwnd)
						break;
				if (w != NULL && areawin != w) {
					areawin = w;
					top = get_widget(GetAncestor(areawin->window, GA_ROOT));
					topwin = top->handle;
					toolBar = get_widget(GetDlgItem(topwin, TOOLBAR_ID));
					statusBar = GetDlgItem(topwin, STATUSBAR_ID);
					{
						MENUITEMINFO mi_info;
						HMENU hMenu = GetMenu(hwnd);
						int i;

						ZeroMemory(&mi_info, sizeof(mi_info));
						mi_info.cbSize = sizeof(mi_info);
						for (i=0; i<MaxMenuWidgets; i++) {
							mi_info.fMask = MIIM_DATA;
							mi_info.dwItemData = (LONG)NULL;
							if (GetMenuItemInfo(hMenu, 100+i, FALSE, &mi_info))
								menuwidgets[i] = (Widget)mi_info.dwItemData;
							else
								printf("menu not found: %d\n", i);
						}
					}
				}
				SetFocus(areawin->window);
			}
			return 0;
		case WM_SIZE:
			{
				int width = LOWORD(lParam), th, sh, height = HIWORD(lParam), nbrows = 1;
				DWORD sz;
				RECT sr;

				SetWindowPos(toolBar->handle, HWND_TOP, 0, 0, width, 25, 0);

				sz = SendMessage(toolBar->handle, TB_GETBUTTONSIZE, 0, 0);
				nbrows = SendMessage(toolBar->handle, TB_GETROWS, 0, 0);
				th = nbrows*HIWORD(sz)+6;
				GetWindowRect(statusBar, &sr);
				sh = sr.bottom-sr.top;

				SetWindowPos(areawin->area->handle, HWND_TOP, SBARSIZE, th, width-SBARSIZE, height-sh-th-SBARSIZE, 0);
				SetWindowPos(areawin->scrollbarh->handle, HWND_TOP, SBARSIZE, height-sh-SBARSIZE, width-SBARSIZE, SBARSIZE, 0);
				SetWindowPos(areawin->scrollbarv->handle, HWND_TOP, 0, th, SBARSIZE, height-sh-th-SBARSIZE, 0);
				SetWindowPos(corner, HWND_TOP, 0, height-sh-SBARSIZE, 0, 0, SWP_NOSIZE);
				SetWindowPos(statusBar, HWND_TOP, 0, height-sh, width, sh, 0);
				InvalidateRect(statusBar, NULL, FALSE);
				SetWindowPos(toolBar->handle, HWND_TOP, 0, 0, width, th, 0);
				InvalidateRect(toolBar->handle, NULL, FALSE);
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_MENUCOMMAND:
			{
				MENUITEMINFO mi_info;
				MenuItem item;

				ZeroMemory(&mi_info, sizeof(mi_info));
				mi_info.cbSize = sizeof(mi_info);
				mi_info.fMask = MIIM_DATA;
				GetMenuItemInfo((HMENU)lParam, wParam, MF_BYPOSITION, &mi_info);

				if ((item = (MenuItem)mi_info.dwItemData) != NULL && item->callbacks != NULL) {
					W32DEBUG(("Calling callback\n"));
					execute_callback(XtNselect, (Widget)item, NULL);
				}
			}
			return 0;
		case WM_COMMAND:
			if ((HWND)lParam == toolBar->handle) {
				TBBUTTONINFO tbi;

				ZeroMemory(&tbi, sizeof(tbi));
				tbi.cbSize = sizeof(tbi);
				tbi.dwMask = TBIF_LPARAM;
				if (SendMessage(toolBar->handle, TB_GETBUTTONINFO, LOWORD(wParam), (LPARAM)&tbi) != -1) {
					ToolItem item = (ToolItem)tbi.lParam;
					W32DEBUG(("Tool button: %s\n", item->name));
					execute_callback(XtNselect, (Widget)item, NULL);
				}
			}
			return 0;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

void button_event(Widget win, int type, int button, short x, short y)
{
	XButtonEvent event;

	event.type = type;
	event.x = x;
	event.y = y;
	event.button = (button & MK_LBUTTON ? Button1 : 0) |
		       (button & MK_RBUTTON ? Button3 : 0) |
		       (button & MK_MBUTTON ? Button2 : 0) |
		       (button & MK_XBUTTON1 ? Button4 : 0) |
		       (button & MK_XBUTTON2 ? Button5 : 0);
	event.state = (GetKeyState(VK_CONTROL) & 0x8000 ? ControlMask : 0) |
		      (GetKeyState(VK_SHIFT) & 0x8000 ? ShiftMask : 0) |
		      (GetKeyState(VK_MENU) & 0x8000 ? Mod1Mask : 0);

	if (win == areawin->scrollbarh || win == areawin->scrollbarv)
		if (type == ButtonPress)
			SetCapture(win->handle);
		else if (type == ButtonRelease)
			ReleaseCapture();

	switch (type) {
		case ButtonPress:
			execute_callback(XtNselect, win, &event);
			break;
		case ButtonRelease:
			execute_callback(XtNrelease, win, &event);
			break;
		case MotionNotify:
			if (event.button && HAS_HANDLER(win, ButtonMotionMask))
				execute_handler(ButtonMotionMask, win, &event);
			if ((event.button & Button1Mask) && HAS_HANDLER(win, Button1MotionMask))
				execute_handler(Button1MotionMask, win, &event);
			if ((event.button & Button2Mask) && HAS_HANDLER(win, Button2MotionMask))
				execute_handler(Button2MotionMask, win, &event);
			if (HAS_HANDLER(win, PointerMotionMask))
				execute_handler(PointerMotionMask, win, &event);
			break;
	}
}

void key_event(Widget win, int type, int key, int scan)
{
	POINT pt;
	static BYTE keys[256];
	WORD c = 0x0000;
	XKeyEvent event;

	GetCursorPos(&pt);
	ScreenToClient(win->handle, &pt);
	event.type = type;
	event.x = pt.x;
	event.y = pt.y;
	event.state = 0;
	event.keycode = 0;

	GetKeyboardState(keys);
	if (keys[VK_CONTROL] & 0x80) { event.state |= ControlMask; keys[VK_CONTROL] &= ~0x80; }
	if (keys[VK_MENU] & 0x80) { event.state |= Mod1Mask; keys[VK_MENU] &= ~0x80; }
	keys[VK_NUMLOCK] &= 0x01;
	if (ToAscii(key, scan, keys, &c, 0) == 1 && c >= ' ' && c <= '~') {
		W32DEBUG(("char: %c (%08x)\n", c, c));
		event.keycode = 0xff & c;
		switch (c) {
			case '/':
				if (scan & 0x1000000) event.keycode |= KPMOD;
				break;
			case '*':
			case '-':
			case '+':
				if ((VkKeyScan(c)&0xff) != key)
					event.keycode |= KPMOD;
				break;
			default:
				break;
		}
	} else {
		if (keys[VK_SHIFT] & 0x80) event.state |= ShiftMask;
		event.keycode |= VKMOD;
		switch (key) {
			case VK_RETURN:
				if (scan & 0x1000000) event.keycode |= KPMOD;
				else event.keycode |= VKMOD;
				break;
			case VK_HOME:
			case VK_UP:
			case VK_PRIOR:
			case VK_LEFT:
			case VK_RIGHT:
			case VK_END:
			case VK_DOWN:
			case VK_NEXT:
			case VK_DELETE:
				if (!(scan & 0x1000000)) event.keycode |= KPMOD;
				else event.keycode |= VKMOD;
				break;
			case VK_CLEAR:
				if (scan == 0x004c0001 || scan == 0xc04c0001) event.keycode |= KPMOD;
				break;
			default:
				event.keycode |= VKMOD;
				break;
		}
		event.keycode |= key;
		W32DEBUG(("non char: %d\n", event.keycode));
	}

	switch (type) {
		case KeyPress:
			execute_callback(XtNkeyDown, win, &event);
			break;
		case KeyRelease:
			execute_callback(XtNkeyUp, win, &event);
			break;
	}
}

LRESULT CALLBACK XcWidgetProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Widget win = (Widget)get_widget(hwnd);

	switch (msg) {
		case WM_DESTROY:
			destroy_widget(hwnd);
			return 0;
		case WM_PAINT:
			{
				RECT r;
				GetUpdateRect(hwnd, &r, FALSE);
				if (win != NULL)
					execute_callback(XtNexpose, win, NULL);
				ValidateRect(hwnd, &r);
			}
			return 0;
		case WM_SIZE:
			InvalidateRect(hwnd, NULL, FALSE);
			if (win != NULL) {
				execute_callback(XtNresize, win, NULL);
				if (win->buffer != NULL) {
					WinFreePixmap(win->buffer);
					win->buffer = WinCreatePixmap(hwnd, LOWORD(lParam), HIWORD(lParam));
					SelectObject(win->bufhdc, win->buffer);
				}
			}
			return 0;
		case WM_CHAR:
			W32DEBUG(("WM_CHAR: %08x %08x\n", wParam, lParam));
			return 0;
		case WM_SYSKEYDOWN:
			if (wParam == VK_MENU || (wParam == VK_F4 && GetKeyState(VK_MENU) && 0x8000))
				return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_KEYDOWN:
			W32DEBUG(("WM_KEYDOWN: %08x %08x %08x\n", wParam, lParam, MapVirtualKey((0x00ff0000&lParam)>>16, 1)));
			key_event(win, KeyPress, MapVirtualKey((0x00ff0000&lParam)>>16, 1), lParam);
			return 0;
		case WM_SYSKEYUP:
			if (wParam == VK_MENU)
				return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_KEYUP:
			W32DEBUG(("WM_KEYUP: %08x %08x\n", wParam, lParam));
			key_event(win, KeyRelease, MapVirtualKey((0x00ff0000&lParam)>>16, 1), lParam);
			return 0;
		case WM_MOUSEMOVE:
			button_event(win, MotionNotify, wParam, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDOWN:
			button_event(win, ButtonPress, wParam, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_RBUTTONUP:
			button_event(win, ButtonRelease, MK_RBUTTON, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_MBUTTONUP:
			button_event(win, ButtonRelease, MK_MBUTTON, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_LBUTTONUP:
			button_event(win, ButtonRelease, MK_LBUTTON, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_MOUSEWHEEL:
			{
				SHORT delta = HIWORD(wParam);
				panbutton((delta > 0 ? 3 : 4), 0, 0, 0.05);
				refresh(NULL, NULL, NULL);
			}
			return 0;
		case WM_HSCROLL:
			switch (LOWORD(wParam)) {
				case SB_LINELEFT:
					panbutton(1, 0, 0, 0.03);
					break;
				case SB_LINERIGHT:
					panbutton(2, 0, 0, 0.03);
					break;
			}
			refresh(NULL, NULL, NULL);
			return 0;
		default:
			if (msg == WM_BITBLT && win != NULL && win->buffer != NULL) {
				HDC winhdc = GetDC(hwnd);
				RECT r;
				MSG pmsg;

				while (PeekMessage(&pmsg, hwnd, WM_BITBLT, WM_BITBLT, PM_REMOVE));
				GetWindowRect(hwnd, &r);
				BitBlt(winhdc, 0, 0, r.right-r.left, r.bottom-r.top, win->bufhdc, 0, 0, SRCCOPY);
				ReleaseDC(hwnd, winhdc);
				return 0;
			} else
				return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

LRESULT CALLBACK XcHelppixProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_GETDLGCODE:
			return DLGC_WANTARROWS;
		case WM_CREATE:
			if (helppix == NULL)
				printhelppix();
			helptop = 0;
			return 0;
		case WM_SIZE:
			{
				SCROLLINFO si;

				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_ALL;
				si.nMin = 0;
				si.nMax = (helpheight-HIWORD(lParam)+400-1);
				si.nPage = 400;
				si.nPos = 0;
				SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
				return 0;
			}
		case WM_PAINT:
			{
				HDC hdc, pixhdc;
				PAINTSTRUCT ps;
				RECT r;

				hdc = BeginPaint(hwnd, &ps);
				pixhdc = get_hdc(helppix, hgc);
				GetClientRect(hwnd, &r);
				BitBlt(hdc, 0, 0, r.right-r.left, r.bottom-r.top, pixhdc, 0, helptop, SRCCOPY);
				release_hdc(helppix, pixhdc);
				EndPaint(hwnd, &ps);
				return 0;
			}
			break;
		case WM_VSCROLL:
			{
				SCROLLINFO si;
				int pos;

				si.cbSize = sizeof(si);
				si.fMask = SIF_ALL;
				GetScrollInfo(hwnd, SB_VERT, &si);
				switch (LOWORD(wParam)) {
					case SB_PAGEUP:
						si.nPos -= si.nPage;
						break;
					case SB_PAGEDOWN:
						si.nPos += si.nPage;
						break;
					case SB_LINEUP:
						si.nPos -= 15;
						break;
					case SB_LINEDOWN:
						si.nPos += 15;
						break;
					case SB_THUMBTRACK:
						si.nPos = si.nTrackPos;
						break;
				}
				si.fMask = SIF_POS;
				SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
				GetScrollInfo(hwnd, SB_VERT, &si);
				helptop = si.nPos;
				InvalidateRect(hwnd, NULL, FALSE);
			}
			return 0;
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_NEXT:
					SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_PAGEDOWN, 0), 0);
					break;
				case VK_PRIOR:
					SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_PAGEUP, 0), 0);
					break;
				case VK_DOWN:
					SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
					break;
				case VK_UP:
					SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0);
					break;
			}
			return 0;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

static XFontStruct* create_font_struct(HFONT hFont)
{
	TEXTMETRIC tm;
	XFontStruct *fs = NULL;
	HDC hdc = GetDC(NULL);

	SelectObject(hdc, hFont);
	if (GetTextMetrics(hdc, &tm)) {
		fs = (XFontStruct*)malloc(sizeof(XFontStruct));
		fs->ascent = tm.tmAscent;
		fs->descent = tm.tmDescent;
		fs->fid = hFont;
	} else {
		W32DEBUG(("Failed to create font structure\n"));
	}
	ReleaseDC(NULL, hdc);

	return fs;
}

static menuID = 100;

static Menu create_win_menu(HMENU menu)
{
	Menu wmenu = (Menu)malloc(sizeof(struct __WinMenu));
	MENUINFO minfo;

	wmenu->type = WIN_MENU;
	wmenu->handle = menu;
	minfo.cbSize = sizeof(MENUINFO);
	minfo.fMask = MIM_MENUDATA | MIM_STYLE;
	minfo.dwStyle = MNS_NOTIFYBYPOS;
	minfo.dwMenuData = (DWORD)wmenu;
	SetMenuInfo(menu, &minfo);

	return wmenu;
}

static MenuItem insert_win_menuitem(Menu menu, MENUITEMINFO* mi_info, menustruct *ms, int pos)
{
	MenuItem item = (MenuItem)malloc(sizeof(struct __WinMenuItem));

	ZeroMemory(item, sizeof(struct __WinMenuItem));
	item->type = WIN_MENUITEM;
	item->parentMenu = menu;
	if (mi_info->fMask & MIIM_SUBMENU && mi_info->hSubMenu != NULL) {
		MENUINFO popup_info;
		ZeroMemory(&popup_info, sizeof(popup_info));
		popup_info.cbSize = sizeof(popup_info);
		popup_info.fMask = MIM_MENUDATA;
		if (GetMenuInfo(mi_info->hSubMenu, &popup_info))
			item->popup = (Menu)popup_info.dwMenuData;
	}
	else if (mi_info->fMask & MIIM_ID) {
		item->ID = mi_info->wID;
		menuwidgets[mi_info->wID-100] = (Widget)item;
	} else if (!(mi_info->fMask & MIIM_FTYPE && mi_info->fType == MFT_SEPARATOR)){
	}
	if (ms != NULL) {
		if (ms->func != NULL && ms->func != DoNothing)
			WinAddCallback((Widget)item, XtNselect, ms->func, ms->passeddata);
		item->name = strdup(ms->name);
	}
	if (pos < 0)
		pos = GetMenuItemCount(menu->handle);
	item->position = pos;
	item->menudata = mi_info->dwItemData;
	mi_info->fMask |= MIIM_DATA;
	mi_info->dwItemData = (DWORD)item;
	InsertMenuItem(menu->handle, item->position, MF_BYPOSITION, mi_info);
	for (pos++; pos<GetMenuItemCount(menu->handle); pos++) {
		MENUITEMINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		info.fMask = MIIM_DATA;
		GetMenuItemInfo(menu->handle, pos, MF_BYPOSITION, &info);
		TOMENUITEM(info.dwItemData)->position = pos;
	}
	return item;
}

#if 0
#define MENUICON_WIDTH GetSystemMetrics(SM_CXMENUCHECK)
#define MENUICON_HEIGHT GetSystemMetrics(SM_CYMENUCHECK)
#else
#define MENUICON_WIDTH 12
#define MENUICON_HEIGHT 12
#endif

static HBITMAP create_color_icon(int color, int width, int height)
{
	HDC hdc = GetDC(NULL);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
	HBRUSH hBrush = CreateSolidBrush(color);
	RECT r = {0, 0};

	ReleaseDC(NULL, hdc);
	r.right = width;
	r.bottom = height;
	hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, hBitmap);
	FillRect(hdc, &r, hBrush);
	DeleteObject(hBrush);
	DeleteDC(hdc);

	return hBitmap;
}

static HBITMAP create_stipple_icon(int stipdata, int width, int height)
{
	HDC hdc = GetDC(NULL);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
	HBRUSH hBrush = CreatePatternBrush(STIPPLE[stipdata]);
	RECT r = {0, 0};

	ReleaseDC(NULL, hdc);
	r.right = width;
	r.bottom = height;
	hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, hBitmap);
	FillRect(hdc, &r, hBrush);
	DeleteObject(hBrush);
	DeleteDC(hdc);

	return hBitmap;
}

static char* color_string(int c)
{
	static char buf[256];
	char *cname = WinColorName(c);

	if (cname)
		_snprintf(buf, 256, "  %s", cname);
	else
		_snprintf(buf, 256, "  %3d, %3d, %3d", GetRValue(c), GetGValue(c), GetBValue(c));
	return buf;
}

static void makesubmenu(Menu menu, menustruct *items, int n);
static void make_color_submenu(Menu menu, menustruct *items, int n)
{
	if (dpy == NULL)
		makesubmenu(menu, items, n);
	else {
		int i;
		MENUITEMINFO mi_info;
		MenuItem item;

		makesubmenu(menu, items, 3);
		menuID += (n-3);
		for (i=0; i<number_colors; i++) {
			int color = colorlist[i].color.pixel;
			HBITMAP hBitmap = create_color_icon(color, MENUICON_WIDTH, MENUICON_HEIGHT);
			ZeroMemory(&mi_info, sizeof(mi_info));
			mi_info.cbSize = sizeof(mi_info);
			mi_info.fMask = MIIM_BITMAP | MIIM_STRING;
			mi_info.hbmpItem = hBitmap;
			mi_info.dwTypeData = color_string(color);
			mi_info.dwItemData = color;
		
			item = insert_win_menuitem(menu, &mi_info, NULL, -1);
			item->name = strdup(mi_info.dwTypeData);
			WinAddCallback((Widget)item, XtNselect, (XtCallbackProc)setcolor, NULL);
		}
	}
		
}

static void makesubmenu(Menu menu, menustruct *items, int n)
{
	int i;
	MENUITEMINFO mi_info;
	int is_color = -1;
	MenuItem mitem;

	for (i=0; i<n; i++) {
		ZeroMemory(&mi_info, sizeof(mi_info));
		mi_info.cbSize = sizeof(mi_info);
		if (items[i].submenu == NULL) {
			if (items[i].name[0] == ' ') {
				mi_info.fMask = MIIM_FTYPE;
				mi_info.fType = MFT_SEPARATOR;
			}
			else if (items[i].name[0] == '_') {
				int color = WinNamedColor(items[i].name+1);
				HBITMAP hBitmap = create_color_icon(color, MENUICON_WIDTH, MENUICON_HEIGHT);
				mi_info.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_ID;
				mi_info.wID = menuID++;
				mi_info.hbmpItem = hBitmap;
				mi_info.dwTypeData = color_string(color);
				mi_info.dwItemData = color;
				is_color = color;;
			} else if (items[i].name[0] == ':') {
				HBITMAP hBitmap;
				if ((int)items[i].passeddata == (OPAQUE | FILLED | FILLSOLID))
					hBitmap = create_color_icon(RGB(0, 0, 0), MENUICON_WIDTH, MENUICON_HEIGHT);
				else
					hBitmap = create_stipple_icon(((int)items[i].passeddata & FILLSOLID)>>5, MENUICON_WIDTH, MENUICON_HEIGHT);
				mi_info.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_ID;
				mi_info.wID = menuID++;
				mi_info.hbmpItem = hBitmap;
				mi_info.dwTypeData = items[i].name;
				mi_info.dwItemData = (DWORD)items[i].passeddata;
			} else {
				mi_info.fMask = MIIM_STRING | MIIM_ID;
				mi_info.wID = menuID++;
				mi_info.dwTypeData = items[i].name;
			}
		} else {
			Menu popup = create_win_menu(CreatePopupMenu());
			if (strncmp(items[i].name, "Color", 5) == 0)
				make_color_submenu(popup, items[i].submenu, items[i].size);
			else
				makesubmenu(popup, items[i].submenu, items[i].size);
			mi_info.fMask = MIIM_SUBMENU | MIIM_STRING;
			mi_info.hSubMenu = popup->handle;
			mi_info.dwTypeData = items[i].name;
		}
		mitem = insert_win_menuitem(menu, &mi_info, &items[i], -1);
		if (is_color != -1 && dpy == NULL)
			addtocolorlist((Widget)mitem, is_color);
	}
}

static HMENU create_menus()
{
	Menu menu;
	MENUITEMINFO mi_info;
	int i;

	menuID = 100;
	menu = create_win_menu(CreateMenu());
	for (i=0; i<maxbuttons; i++) {
		ZeroMemory(&mi_info, sizeof(mi_info));
		mi_info.cbSize = sizeof(mi_info);
		if (TopButtons[i].submenu == NULL) {
			mi_info.fMask = MIIM_STRING | MIIM_ID;
			mi_info.wID = menuID++;
			mi_info.dwTypeData = TopButtons[i].name;
		} else {
			Menu popup = create_win_menu(CreatePopupMenu());
			makesubmenu(popup, TopButtons[i].submenu, TopButtons[i].size);
			mi_info.fMask = MIIM_SUBMENU | MIIM_STRING;
			mi_info.hSubMenu = popup->handle;
			mi_info.dwTypeData = TopButtons[i].name;
		}
		insert_win_menuitem(menu, &mi_info, &TopButtons[i], -1);
	}

	return menu->handle;
}

static void create_statusbar()
{
	HDC hdc;
	SIZE sz;
	StatusItem msg2, msg3;

	hdc = GetDC(NULL);
	GetTextExtentPoint32(hdc, "Editing: filename (Page 1)", 26, &sz);
	statusBarWidth[0] = sz.cx;
	ReleaseDC(NULL, hdc);

	statusBar = CreateWindow(STATUSCLASSNAME, "", WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
			0, 0, 100, 100, top->handle, (HMENU)STATUSBAR_ID, NULL, NULL);
	SendMessage(statusBar, SB_SETPARTS, 2, (LPARAM)statusBarWidth);
	SendMessage(statusBar, SB_SETTEXT, 0, (LPARAM)"Editing: Page 1");
	SendMessage(statusBar, SB_SETTEXT, 1, (LPARAM)"Don't Panic");

	msg2 = (StatusItem)malloc(sizeof(struct __WinStatusItem));
	msg2->handle = statusBar;
	msg2->type = WIN_STATUS;
	msg2->position = 0;
	message2 = (Widget)msg2;
	
	msg3 = (StatusItem)malloc(sizeof(struct __WinStatusItem));
	msg3->handle = statusBar;
	msg3->type = WIN_STATUS;
	msg3->position = 1;
	message3 = (Widget)msg3;
}

typedef struct { int c; RGBQUAD color; } xpm_color;

static HBITMAP create_bitmap_from_xpm(char **xpm)
{
	int width, height, ncolors, nchar;
	int i, j, k;
	BITMAPINFO *bmi;
	HBITMAP hBitmap = NULL;
	xpm_color *colors;
	BYTE *data;
	HDC hdc;

	if (sscanf(xpm[0], "%d %d %d %d", &width, &height, &ncolors, &nchar) != 4)
		return NULL;
	if (nchar != 1) {
		W32DEBUG(("(XPM) Unsupported number of char per data\n"));
		return NULL;
	}
	colors = (xpm_color*)malloc(sizeof(xpm_color)*ncolors);
	for (i=0; i<ncolors; i++) {
		char *str = xpm[i+1];
		char snum[5];
		colors[i].c = str[0]; str++;
		for (; *str!='c'; str++); str+=2;
		if (*str == '#') {
			str++;
			strncpy(snum, str, 4); snum[4] = 0; str+=4;
			colors[i].color.rgbRed = 0xff&(strtol(snum, NULL, 16)>>8);
			strncpy(snum, str, 4); snum[4] = 0; str+=4;
			colors[i].color.rgbGreen = 0xff&(strtol(snum, NULL, 16)>>8);
			strncpy(snum, str, 4); snum[4] = 0; str+=4;
			colors[i].color.rgbBlue = 0xff&(strtol(snum, NULL, 16)>>8);
		} else {
			COLORREF cr = WinNamedColor(str);
			colors[i].color.rgbRed = GetRValue(cr);
			colors[i].color.rgbGreen = GetGValue(cr);
			colors[i].color.rgbBlue = GetBValue(cr);
		}
	}

	data = (BYTE*)malloc(sizeof(BYTE)*width*height);
	for (j=k=0; j<height; j++) {
		for (i=0; i<width; i++, k++) {
			char pix = xpm[j+ncolors+1][i];
			int idx;
			for (idx=0; idx<ncolors; idx++)
				if (pix == colors[idx].c) break;
			data[k] = idx;
		}
	}

	bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFO)+(ncolors-1)*sizeof(RGBQUAD));
	ZeroMemory(bmi, sizeof(BITMAPINFO));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = width;
	bmi->bmiHeader.biHeight = -height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 8;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biClrUsed = ncolors;
	for (i=0; i<ncolors; i++)
		memcpy(bmi->bmiColors+i, &colors[i].color, sizeof(RGBQUAD));
	hdc = GetDC(NULL);
	hBitmap = CreateDIBitmap(hdc, &bmi->bmiHeader, CBM_INIT, data, bmi, DIB_RGB_COLORS);

	ReleaseDC(NULL, hdc);
	free(bmi);
	free(colors);

	return hBitmap;
}

static void create_toolbar()
{
	TBBUTTON *buttons;
	TBADDBITMAP bitmap;
	int i;

	toolBar = create_widget(CreateWindow(TOOLBARCLASSNAME, NULL, WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|TBSTYLE_WRAPABLE, 0, 0, 0, 0,
			top->handle, (HMENU)TOOLBAR_ID, NULL, NULL), "ToolBar");
	SendMessage(toolBar->handle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(toolBar->handle, TB_SETBITMAPSIZE, 0, MAKELONG(20, 20));

	buttons = (TBBUTTON*)malloc(sizeof(TBBUTTON) * toolbuttons);
	for (i=0; i<toolbuttons; i++) {
		ToolItem item;

		buttons[i].iBitmap = i;
		buttons[i].idCommand = menuID++;
		buttons[i].fsState = TBSTATE_ENABLED;
		buttons[i].fsStyle = TBSTYLE_BUTTON;
		buttons[i].iString = 0;
		bitmap.hInst = NULL;
		bitmap.nID = (INT_PTR)create_bitmap_from_xpm(ToolBar[i].icon_data);
		SendMessage(toolBar->handle, TB_ADDBITMAP, 1, (LPARAM)&bitmap);

		item = (ToolItem)malloc(sizeof(struct __WinToolItem));
		ZeroMemory(item, sizeof(struct __WinToolItem));
		item->type = WIN_TOOLITEM;
		item->toolbar = toolBar;
		item->ID = buttons[i].idCommand;
		item->name = strdup(ToolBar[i].name);
		buttons[i].dwData = (DWORD)item;

		menuwidgets[item->ID-100] = (Widget)item;
		XtAddCallback((Widget)item, XtNselect, (XtCallbackProc)ToolBar[i].func, ToolBar[i].passeddata);

		/*
		if (ToolBar[i].func == changetool ||
		    ToolBar[i].func == exec_or_changetool)
			buttons[i].fsStyle |= TBSTYLE_CHECK;
			*/
	}
	SendMessage(toolBar->handle, TB_ADDBUTTONS, toolbuttons, (LPARAM)buttons);
	free(buttons);
}

static void register_win32_classes()
{
	WNDCLASS wndClass;

	/* Top-level window */
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.lpfnWndProc = XcTopWindowProc;
	wndClass.lpszClassName = "XcTopLevel";
	wndClass.hIcon = LoadIcon(GetModuleHandle(NULL), "xcircuit");
	RegisterClass(&wndClass);

	/* Main area */
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.lpfnWndProc = XcWidgetProc;
	wndClass.lpszClassName = "XcWidget";
	wndClass.cbWndExtra = sizeof(void*);
	RegisterClass(&wndClass);

	/* Scroll bar */
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.lpfnWndProc = XcWidgetProc;
	wndClass.lpszClassName = "XcScrollBar";
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.cbWndExtra = sizeof(void*);
	RegisterClass(&wndClass);

	/* Help widget */
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.lpfnWndProc = XcHelppixProc;
	wndClass.lpszClassName = "XcHelp";
	RegisterClass(&wndClass);

	/* Popup */
	/*
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.lpfnWndProc = XcPopupProc;
	wndClass.lpszClassName = "XcPopup";
	wndClass.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);
	*/
}

XCWindowData* GUI_init(int argc, char *argv[])
{
	int i;
	XGCValues values;
	Arg wargs[2];
	RECT clientRect = { 0, 0, 0, 0 };
	XCWindowData *newwin;

#if 0 && defined(USE_WIN32_COM)
	for (i=0; i<argc; i++)
		if (strcmp(argv[i], "-register") == 0)
			COM_register(1);
		else if (strcmp(argv[i], "-unregister") == 0)
			COM_register(0);
#endif

	if (dpy == NULL) {
		register_win32_classes();

		appdata.globalcolor = WinNamedColor("Orange2");
		appdata.localcolor = WinNamedColor("Red");
		appdata.infocolor = WinNamedColor("SeaGreen");
		appdata.ratsnestcolor = WinNamedColor("Tan4");
		appdata.bboxpix = WinNamedColor("greenyellow");
		appdata.clipcolor = WinNamedColor("powderblue");
		appdata.fg = WinNamedColor("Black");
		appdata.bg = WinNamedColor("White");
		appdata.gridpix = WinNamedColor("Gray95");
		appdata.snappix = WinNamedColor("Red");
		appdata.selectpix = WinNamedColor("Gold3");
		appdata.querypix = WinNamedColor("Turquoise");
		appdata.filterpix = WinNamedColor("SteelBlue3");
		appdata.axespix = WinNamedColor("Antique White");
		appdata.buttonpix = WinNamedColor("Gray85");
		appdata.auxpix = WinNamedColor("Green3");
		appdata.barpix = WinNamedColor("Tan");
		appdata.parampix = WinNamedColor("Plum3");
		appdata.fg2 = WinNamedColor("White");
		appdata.bg2 = WinNamedColor("DarkSlateGray");
		appdata.gridpix2 = WinNamedColor("Gray40");
		appdata.snappix2 = WinNamedColor("Red");
		appdata.selectpix2 = WinNamedColor("Gold");
		appdata.querypix2 = WinNamedColor("Turquoise");
		appdata.filterpix2 = WinNamedColor("SteelBlue1");
		appdata.axespix2 = WinNamedColor("NavajoWhite4");
		appdata.buttonpix2 = WinNamedColor("Gray50");
		appdata.auxpix2 = WinNamedColor("Green");
		appdata.barpix2 = WinNamedColor("Tan");
		appdata.parampix2 = WinNamedColor("Plum3");
		appdata.width = 950;
		appdata.height = 760;
		appdata.timeout = 10;
		appdata.xcfont = create_font_struct(GetStockObject(DEFAULT_GUI_FONT));
		appdata.helpfont = create_font_struct(GetStockObject(DEFAULT_GUI_FONT));
		appdata.filefont = create_font_struct(GetStockObject(DEFAULT_GUI_FONT));
		appdata.textfont = create_font_struct(GetStockObject(DEFAULT_GUI_FONT));
		appdata.titlefont = create_font_struct(GetStockObject(DEFAULT_GUI_FONT));

		for (i=0; i<STIPPLES; i++) {
			STIPPLE[i] = WinCreateBitmapFromData(NULL, STIPDATA[i], 4, 4);
		}

		number_colors = NUMBER_OF_COLORS;
		colorlist = (colorindex *)malloc(NUMBER_OF_COLORS * sizeof(colorindex));
	}

	clientRect.right = appdata.width+SBARSIZE;
	clientRect.bottom = appdata.height+SBARSIZE;
	AdjustWindowRect(&clientRect, WS_TILEDWINDOW, TRUE);
	OffsetRect(&clientRect, -clientRect.left, -clientRect.top);

	top = create_widget(CreateWindow("XcTopLevel", "XCircuit", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, clientRect.right, clientRect.bottom,
			NULL, create_menus(), NULL, NULL), "XCircuit");
	topwin = top->handle;

     	dpy = XtDisplay(top);
	newwin = create_new_window();

	newwin->scrollbarv = create_widget(CreateWindow("XcScrollBar", "SBV", WS_VISIBLE|WS_CHILD,
				0, 0, SBARSIZE, appdata.height-SBARSIZE, top->handle, 0, NULL, NULL), "SBV");
	newwin->scrollbarh = create_widget(CreateWindow("XcScrollBar", "SBH", WS_VISIBLE|WS_CHILD,
				SBARSIZE, appdata.height-SBARSIZE, appdata.width-SBARSIZE, SBARSIZE, top->handle, 0, NULL, NULL), "SBH");
	newwin->area = create_widget(CreateWindow("XcWidget", "Area", WS_VISIBLE|WS_CHILD,
				SBARSIZE, 0, appdata.width, appdata.height, top->handle, 0, NULL, NULL), "Area");
	newwin->window = newwin->area->handle;
	corner = CreateWindow("BUTTON", "", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT, 0, appdata.height-SBARSIZE, SBARSIZE, SBARSIZE, top->handle, NULL, NULL, NULL);

	create_statusbar();
	create_toolbar();

     	XtAddCallback(newwin->area, XtNexpose, (XtCallbackProc)drawarea, NULL);
     	XtAddCallback(newwin->area, XtNresize, (XtCallbackProc)resizearea, NULL);

     	XtAddCallback(newwin->area, XtNselect, (XtCallbackProc)buttonhandler, NULL);
     	XtAddCallback(newwin->area, XtNrelease, (XtCallbackProc)buttonhandler, NULL);
     	XtAddCallback(newwin->area, XtNkeyDown, (XtCallbackProc)keyhandler, NULL);
     	XtAddCallback(newwin->area, XtNkeyUp, (XtCallbackProc)keyhandler, NULL);

	XtAddEventHandler(newwin->area, Button1MotionMask | Button2MotionMask,
			False, (XtEventHandler)xlib_drag, NULL);

     	XtAddEventHandler(newwin->scrollbarh, ButtonMotionMask, False,
			(XtEventHandler)panhbar, NULL);
     	XtAddEventHandler(newwin->scrollbarv, ButtonMotionMask, False,
			(XtEventHandler)panvbar, NULL);

     	XtAddCallback(newwin->scrollbarh, XtNrelease, (XtCallbackProc)endhbar, NULL);
     	XtAddCallback(newwin->scrollbarv, XtNrelease, (XtCallbackProc)endvbar, NULL);

	XtAddCallback(newwin->scrollbarh, XtNexpose, (XtCallbackProc)drawhbar, NULL);
     	XtAddCallback(newwin->scrollbarv, XtNexpose, (XtCallbackProc)drawvbar, NULL);
     	XtAddCallback(newwin->scrollbarh, XtNresize, (XtCallbackProc)drawhbar, NULL);
     	XtAddCallback(newwin->scrollbarv, XtNresize, (XtCallbackProc)drawvbar, NULL);
	
     	values.foreground = RGB(0, 0, 0);
     	values.background = RGB(255, 255, 255);
     	values.font = appdata.xcfont->fid;

     	newwin->gc = XCreateGC(dpy, newwin->window,
			GCForeground | GCBackground | GCFont, &values);

     	XtSetArg(wargs[0], XtNwidth, &newwin->width);
     	XtSetArg(wargs[1], XtNheight, &newwin->height);
    	XtGetValues(newwin->area, wargs, 2);

	SendMessage(((ToolItem)WireToolButton)->toolbar->handle, TB_CHECKBUTTON, ((ToolItem)WireToolButton)->ID, MAKELONG(TRUE, 0));

#ifdef USE_WIN32_COM
	if (!COM_initialize())
		printf("Failed to initialize COM interface\n");
#endif

	return newwin;
}

void WinAddEventHandler(Widget win, int event, void(*proc)(), void *data)
{
	WinEventHandler *h = win->handlers;

	if (win->type != WIN_WIDGET) {
		W32DEBUG(("Can't add event handler to non widget\n"));
		return;
	}

	while (h != NULL) {
		if (h->proc == proc && h->data == data) {
			h->action |= event;
			break;
		} else
			h = h->next;
	}

	if (h == NULL) {
		h = (WinEventHandler*)malloc(sizeof(WinEventHandler));
		h->action = event;
		h->proc = proc;
		h->data = data;
		h->next = win->handlers;
		win->handlers = h;
	}

	update_event_mask(win);
}

void WinRemoveEventHandler(Widget win, int event, void(*proc)(), void *data)
{
	WinEventHandler *h, *prev = NULL;

	if (win->type != WIN_WIDGET) {
		W32DEBUG(("Can't remove event handler from non widget\n"));
		return;
	}

	h = win->handlers;
	for (; h!=NULL;)
		if ((h->action & event) && h->proc == proc && h->data == data) {
			h->action &= ~event;
			if (h->action == 0) {
				WinEventHandler *old_h = h;
				if (prev == NULL)
					win->handlers = h = h->next;
				else
					prev->next = h = h->next;
				free(old_h);
			}
		} else {
			prev = h;
			h = h->next;
		}
	update_event_mask(win);
}

void WinAddCallback(Widget win, int event, void(*proc)(), void *data)
{
	WinCallback *cb, **root, *prev = NULL;

	if (win == NULL)
		return;

	switch (win->type) {
		case WIN_MENUITEM:
			if (TOMENUITEM(win)->popup != NULL) {
				Wprintf("Trying to add a callback to a popup menu");
				return;
			}
			root = &((MenuItem)win)->callbacks;
			break;
		case WIN_TOOLITEM:
			root = &((ToolItem)win)->callbacks;
			break;
		default:
			root = &win->callbacks;
			break;
	}

	cb = *root;
	while (cb) {
		prev = cb;
		cb = cb->next;
	}

	cb = (WinCallback*)malloc(sizeof(WinCallback));
	cb->action = event;
	cb->proc = proc;
	cb->data = data;
	cb->next = NULL;

	if (prev)
		prev->next = cb;
	else
		*root = cb;
}

int WinFindCallback(Widget win, int event, void(**proc)(), void **data)
{
	WinCallback *cb;

	if (win == NULL)
		return 0;

	switch (win->type) {
		case WIN_MENUITEM:
			cb = TOMENUITEM(win)->callbacks;
			break;
		case WIN_TOOLITEM:
			cb = TOTOOLITEM(win)->callbacks;
			break;
		default:
			cb = win->callbacks;
			break;
	}

	while (cb) {
		if (cb->action == event) {
			if (proc == NULL || *proc == NULL || *proc == cb->proc) {
				if (proc != NULL) *proc = cb->proc;
				if (data != NULL) *data = cb->data;
				return 1;
			}
		}
		cb = cb->next;
	}

	return 0;
}

void WinRemoveCallback(Widget win, int event, void(*proc)(), void *data)
{
	WinCallback *cb;
	WinCallback *prev = NULL, **root;

	if (win == NULL)
		return;

	switch (win->type) {
		case WIN_MENUITEM:
			cb = ((MenuItem)win)->callbacks;
			root = &((MenuItem)win)->callbacks;
			break;
		case WIN_TOOLITEM:
			cb = ((ToolItem)win)->callbacks;
			root = &((ToolItem)win)->callbacks;
			break;
		default:
			cb = win->callbacks;
			root = &win->callbacks;
			break;
	}
	while (cb != NULL) {
		if (cb->action == event && (proc == NULL || (cb->proc == proc && cb->data == data))) {
			WinCallback *old_cb = cb;
			if (prev == NULL)
				*root = cb = cb->next;
			else
				prev->next = cb = cb->next;
			free(old_cb);
		} else {
			prev = cb;
			cb = cb->next;
		}
	}
}

void WinRemoveAllCallbacks(Widget win, int event)
{
	if (win == NULL)
		return;

	WinRemoveCallback(win, event, NULL, NULL);
}

static Arg* look_arg(Arg *args, int type, int n)
{
	int i;
	for (i=0; i<n; i++)
		if (args[i].type == type)
			return &args[i];
	return NULL;
}

static void get_window_location(Arg *args, int n, POINT *pt, SIZE *sz)
{
	int i;
	for (i=0; i<n; i++)
		switch (args[i].type) {
			case XtNx: if (pt) pt->x = (int)args[i].data; break;
			case XtNy: if (pt) pt->y = (int)args[i].data; break;
			case XtNwidth: if (sz) sz->cx = (int)args[i].data; break;
			case XtNheight: if (sz) sz->cy = (int)args[i].data; break;
		}
}

Widget WinCreateWidget(const char *name, int cls, Widget parent, Arg *args, int n, int show)
{
	int i;
	Widget win;

	switch (cls) {
		case XwcascadeWidgetClass:
			{
				MENUITEMINFO mi_info;
				MenuItem item;
				Menu popup = create_win_menu(CreatePopupMenu());
				Arg *a;
				int pos = -1;

				ZeroMemory(&mi_info, sizeof(mi_info));
				mi_info.cbSize = sizeof(mi_info);
				mi_info.fMask = MIIM_SUBMENU | MIIM_STRING;
				mi_info.hSubMenu = popup->handle;
				mi_info.dwTypeData = (char*)name;
				if (IS_MENUITEM(parent))
					parent = (Widget)TOMENUITEM(parent)->popup;
				if ((a = look_arg(args, XtNattachTo, n)) != NULL) {
					Widget after = WinNameToWindow(parent, (char*)a->data);
					if (after != NULL)
						pos = TOMENUITEM(after)->position + 1;
				}
				item = insert_win_menuitem((Menu)parent, &mi_info, NULL, pos);
				item->name = strdup(name);
				DrawMenuBar(topwin);
				return (Widget)item;
			}
		case XwmenubuttonWidgetClass:
			{
				MENUITEMINFO mi_info;
				Arg *a;
				MenuItem item;
				int pos = -1;

				ZeroMemory(&mi_info, sizeof(mi_info));
				mi_info.cbSize = sizeof(mi_info);
				if ((a=look_arg(args, XtNrectColor, n)) != NULL) {
					HBITMAP hBitmap = create_color_icon((int)a->data, MENUICON_WIDTH, MENUICON_HEIGHT);
					mi_info.fMask = MIIM_BITMAP | MIIM_STRING /*| MIIM_FTYPE*/;
					/*mi_info.fType = MFT_OWNERDRAW*/;
					mi_info.hbmpItem = hBitmap;
					mi_info.dwTypeData = color_string((int)a->data);
					mi_info.dwItemData = (int)a->data;
				} else {
					mi_info.fMask = MIIM_STRING;
					mi_info.dwTypeData = (char*)name;
				}
				if (IS_MENUITEM(parent))
					parent = (Widget)TOMENUITEM(parent)->popup;
				if ((a = look_arg(args, XtNattachTo, n)) != NULL) {
					Widget after = WinNameToWindow(parent, (char*)a->data);
					if (after != NULL)
						pos = TOMENUITEM(after)->position + 1;
				}
				item = insert_win_menuitem((Menu)parent, &mi_info, NULL, pos);
				item->name = strdup(name);
				DrawMenuBar(topwin);
				return (Widget)item;
			}
			break;
			/*
		case XwstaticTextWidgetClass:
			{
				POINT pt = {CW_USEDEFAULT, CW_USEDEFAULT};

				get_window_location(args, n, &pt, NULL);
				win = create_widget(CreateWindow("STATIC", "", WS_CHILD|WS_VISIBLE, pt.x, pt.y,
							CW_USEDEFAULT, CW_USEDEFAULT, parent->handle, NULL, NULL, NULL), (char*)name);
				win->wndproc = (WNDPROC)SetWindowLong(win->handle, GWL_WNDPROC, (LONG)XcStaticProc);
				WinSetValues(win, args, n);
			}
			break;
		case XwtextEditWidgetClass:
			{
				POINT pt = {CW_USEDEFAULT, CW_USEDEFAULT};
				SIZE sz = {CW_USEDEFAULT, CW_USEDEFAULT};

				get_window_location(args, n, &pt, &sz);
				win = create_widget(CreateWindow("EDIT", "", WS_BORDER|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|WS_TABSTOP, pt.x, pt.y, sz.cx, sz.cy,
							parent->handle, NULL, NULL, NULL), (char*)name);
				//win->wndproc = (WNDPROC)SetWindowLong(win->handle, GWL_WNDPROC, (LONG)XcEditProc);
			}
			break;
		case XwpushButtonWidgetClass:
		case XwmenuButtonWidgetClass:
			{
				POINT pt = {CW_USEDEFAULT, CW_USEDEFAULT};

				get_window_location(args, n, &pt, NULL);
				win = create_widget(CreateWindow("BUTTON", name, BS_PUSHBUTTON|WS_CHILD|WS_VISIBLE, pt.x, pt.y,
							CW_USEDEFAULT, CW_USEDEFAULT, parent->handle, NULL, NULL, NULL), (char*)name);
				win->wndproc = (WNDPROC)SetWindowLong(win->handle, GWL_WNDPROC, (LONG)XcButtonProc);
				WinSetValues(win, args, n);
			}
			break;
		case XwtoggleWidgetClass:
			{
				POINT pt = {CW_USEDEFAULT, CW_USEDEFAULT};

				get_window_location(args, n, &pt, NULL);
				win = create_widget(CreateWindow("BUTTON", name, BS_CHECKBOX|WS_CHILD|WS_VISIBLE, pt.x, pt.y,
							CW_USEDEFAULT, CW_USEDEFAULT, parent->handle, NULL, NULL, NULL), (char*)name);
				win->wndproc = (WNDPROC)SetWindowLong(win->handle, GWL_WNDPROC, (LONG)XcToggleProc);
				WinSetValues(win, args, n);
			}
			break;
		case XwbulletinWidgetClass:
			win = parent;
			break;
			*/
		default:
			W32DEBUG(("Should create widget: %02x, %s\n", cls, name));
			win = NULL;
			break;
	}

	if (show)
		ShowWindow(win->handle, SW_SHOW);
	else
		ShowWindow(win->handle, SW_HIDE);

	return win;
}

Widget WinCreatePopup(const char *name, Arg *args, int n)
{
	HWND hwnd;

	hwnd = CreateWindow("XcPopup", name, WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			300, 200, NULL, NULL, NULL, NULL);
	return create_widget(hwnd, (char*)name);
}

void WinPopup(Widget popup)
{
	ShowWindow(popup->handle, SW_SHOWNORMAL);
}

char* WinGetText(Widget win)
{
	static char buffer[1024];

	GetWindowText(win->handle, buffer, 1024);
	return buffer;
}

void WinClearText(Widget win)
{
	SetWindowText(win->handle, "");
}

int WinNamedColor(const char *cname)
{
	int i;

	for (i=0; X11Colors[i].name; i++) {
		if (_stricmp(X11Colors[i].name, cname) == 0)
			return RGB(X11Colors[i].red, X11Colors[i].green, X11Colors[i].blue);
	}
	W32DEBUG(("Unknown color: %s\n", cname));
	return -1;
}

int WinLookupColor(const char *cname, XColor *color)
{
	int cval = WinNamedColor(cname);
	if (cval == -1)
		return 0;
	else {
		color->pixel = cval;
		WinQueryColors(color, 1);
		return 1;
	}
}

char *WinColorName(int c)
{
	int red = GetRValue(c), green = GetGValue(c), blue = GetBValue(c);
	int i;

	for (i=0; X11Colors[i].name; i++)
		if (red == X11Colors[i].red && green == X11Colors[i].green && blue == X11Colors[i].blue)
			return X11Colors[i].name;
	return NULL;
}

HWND WinWindow(Widget w)
{
	return (w ? w->handle : NULL);
}

void WinManageChild(Widget w)
{
	if (w != NULL)
		ShowWindow(w->handle, SW_SHOW);
}

void WinUnmanageChild(Widget w)
{
	if (w != NULL)
		ShowWindow(w->handle, SW_HIDE);
}

void WinDebug(char *c)
{
	W32DEBUG(("%s", c));
}

void WinLookupString(XKeyEvent *event, KeySym *key)
{
	XKeyEvent *kevent = (XKeyEvent*)event;
	int vk = kevent->keycode & 0xff;

	if (vk == 0)
		*key = 0;
	else {
		W32DEBUG(("Translating key event: %d\n", vk));
		*key = kevent->keycode;
	}
}

void WinPostPopup(Widget _menu, Widget _button, int dx, int dy)
{
	Menu menu = (Menu)_menu;
	ToolItem button = (ToolItem)_button;
	RECT bRect = { 0, 0, 0, 0 };
	POINT pt;

	SendMessage(button->toolbar->handle, TB_GETRECT, button->ID, (LPARAM)&bRect);
	pt.x = bRect.left;
	pt.y = bRect.bottom;
	ClientToScreen(button->toolbar->handle, &pt);
	SendMessage(button->toolbar->handle, TB_CHECKBUTTON, button->ID, MAKELONG(TRUE, 0));
	TrackPopupMenu(menu->handle, TPM_LEFTALIGN|TPM_TOPALIGN, pt.x, pt.y, 0, top->handle, NULL);
	SendMessage(button->toolbar->handle, TB_CHECKBUTTON, button->ID, MAKELONG(FALSE, 0));
}

void overdrawpixmap(Widget button)
{
	MenuItem mitem = (MenuItem)button;
	ToolItem titem = NULL;
	int idx;

	if (button == NULL) return;
	idx = mitem->toolbar_idx;

	if (mitem->parentMenu == ((MenuItem)ColorInheritColorButton)->parentMenu)
		titem = (ToolItem)ColorsToolButton;
	else if (mitem->parentMenu == ((MenuItem)FillBlackButton)->parentMenu)
		titem = (ToolItem)FillsToolButton;
	else
		return;

	if (idx == 0) {
		if (mitem == (MenuItem)ColorInheritColorButton) {
			idx = ((ToolItem)ColorsToolButton)->ID-((ToolItem)PanToolButton)->ID;
		} else if (mitem == (MenuItem)FillWhiteButton) {
			idx = ((ToolItem)FillsToolButton)->ID-((ToolItem)PanToolButton)->ID;
		} else {
			HBITMAP hBitmap = NULL;
			TBADDBITMAP tb;
			if (titem == (ToolItem)ColorsToolButton)
				hBitmap = create_color_icon(mitem->menudata, 20, 20);
			else {
				if (mitem->menudata == (OPAQUE | FILLED | FILLSOLID))
					hBitmap = create_color_icon(RGB(0, 0, 0), 20, 20);
				else if (mitem != (MenuItem)FillOpaqueButton && mitem != (MenuItem)FillTransparentButton)
					hBitmap = create_stipple_icon((mitem->menudata & FILLSOLID) >> 5, 20, 20);
			}
			if (hBitmap != NULL) {
				tb.hInst = NULL;
				tb.nID = (INT_PTR)hBitmap;
				idx = mitem->toolbar_idx = SendMessage(titem->toolbar->handle, TB_ADDBITMAP, 1, (LPARAM)&tb);
			}
		}
	}

	W32DEBUG(("%s: %d\n", titem->name, idx));
	if (idx > 0) {
		SendMessage(titem->toolbar->handle, TB_CHANGEBITMAP, titem->ID, MAKELPARAM(idx, 0));
	}
}

void starthelp(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HELPDLG), areawin->window, HelpDlgProc, (LPARAM)NULL);
}

/*
static void get_text_size(HWND hwnd, HFONT hfont, SIZE *sz)
{
	HDC hdc = GetDC(hwnd);
	char str[64];
	int n;

	SelectObject(hdc, hfont);
	n = GetWindowText(hwnd, str, 64);
	GetTextExtentPoint32(hdc, str, n, sz);
	ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK XcStaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Widget w = (Widget)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg) {
		case WM_SETFONT:
			{
				SIZE sz;
				get_text_size(hwnd, (HFONT)wParam, &sz);
				SetWindowPos(hwnd, HWND_TOP, 0, 0, sz.cx, sz.cy, SWP_NOMOVE);
			}
			break;
	}

	return CallWindowProc(w->wndproc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK XcEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Widget w = (Widget)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg) {
		case WM_KEYDOWN:
			if (wParam == VK_RETURN) {
				execute_callback(XtNexecute, w, NULL);
				return 0;
			}
			break;
	}

	return CallWindowProc(w->wndproc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK XcButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Widget w = (Widget)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg) {
		case WM_SETFONT:
			{
				SIZE sz;
				get_text_size(hwnd, (HFONT)wParam, &sz);
				printf("%d %d\n", sz.cx, sz.cy);
				SetWindowPos(hwnd, HWND_TOP, 0, 0, sz.cx+20, sz.cy+10, SWP_NOMOVE);
			}
			break;
		case WM_SETTEXT:
			{
				LRESULT result = CallWindowProc(w->wndproc, hwnd, msg, wParam, lParam);
				SIZE sz;
				get_text_size(hwnd, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0), &sz);
				SetWindowPos(hwnd, HWND_TOP, 0, 0, sz.cx+20, sz.cy+10, SWP_NOMOVE);
				return result;
			}
		case WM_LBUTTONDOWN:
			execute_callback(XtNselect, w, NULL);
			break;
	}

	return CallWindowProc(w->wndproc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK XcToggleProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Widget w = (Widget)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg) {
		case WM_SETFONT:
			{
				SIZE sz;
				get_text_size(hwnd, (HFONT)wParam, &sz);
				SetWindowPos(hwnd, HWND_TOP, 0, 0, sz.cx+20, sz.cy+10, SWP_NOMOVE);
			}
			break;
		case WM_LBUTTONUP:
			{
				LRESULT result = CallWindowProc(w->wndproc, hwnd, msg, wParam, lParam);
				if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED)
					execute_callback(XtNselect, w, NULL);
				else
					execute_callback(XtNrelease, w, NULL);
			}
			break;
	}

	return CallWindowProc(w->wndproc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK XcPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefDlgProc(hwnd, msg, wParam, lParam);
}
*/

static void change_char(char *str, char cfrom, char cto)
{
	char *c = str;
	for (; *c; c++)
		if (*c == cfrom)
			*c = cto;
}

LRESULT CALLBACK OutputDlgEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Widget win = get_widget (hwnd);

	switch (msg) {
		case WM_GETDLGCODE:
			if (lParam) {
				MSG *message = (MSG*)lParam;
				if (message->message == WM_KEYDOWN && message->wParam == VK_RETURN)
					return DLGC_WANTMESSAGE;
			}
			break;
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_RETURN:
					execute_callback(XtNexecute, win, NULL);
					break;
			}
			break;
	}
	return CallWindowProc(win->wndproc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OutputDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Widget popup = get_widget(hwnd);

	switch (msg) {
		case WM_INITDIALOG:
			{
				RECT r, pr;
				Widget win;
				Pagedata *curpage;
				Widget *entertext;
				int i;
				void (*function[MAXPROPS])() = { setfilename, setpagelabel, setfloat, setscalex, setscaley, setorient, setpmode };
				void (*update[MAXPROPS])() = { updatename, NULL, updatetext, updatetext, updatetext, updatetext, NULL };
				char statics[MAXPROPS][50], edit[MAXPROPS][75], request[150];
				void *data[MAXPROPS];
				struct {int edit; int apply; int label;} ID[MAXPROPS] = {
					{ IDC_FILENAME, IDC_APPLY1, IDC_STATIC1 },
					{ IDC_PAGELABEL, IDC_APPLY2, IDC_STATIC2 },
					{ IDC_SCALE, IDC_APPLY3, IDC_STATIC3 },
					{ IDC_WIDTH, IDC_APPLY4, IDC_STATIC4 },
					{ IDC_HEIGHT, IDC_APPLY5, IDC_STATIC5 },
					{ IDC_ORIENTATION, -1, IDC_STATIC6 },
					{ IDC_MODE, -1, IDC_STATIC7 } };
				struct stat statbuf;

				GetWindowRect(GetParent(hwnd), &pr);
				GetWindowRect(hwnd, &r);
				SetWindowPos(hwnd, HWND_TOP, pr.left+((pr.right-pr.left)-(r.right-r.left))/2,
					       	pr.top+((pr.bottom-pr.top)-(r.bottom-r.top))/2, 0, 0, SWP_NOSIZE);

				SendDlgItemMessage(hwnd, IDC_ORIENTATION, CB_ADDSTRING, 0, (LPARAM)"Landscape");
				SendDlgItemMessage(hwnd, IDC_ORIENTATION, CB_ADDSTRING, 0, (LPARAM)"Portrait");
				SendDlgItemMessage(hwnd, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Embedded (EPS)");
				SendDlgItemMessage(hwnd, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Full page");

				curpage = xobjs.pagelist[areawin->page];
				entertext = (Widget*)malloc(sizeof(Widget)*MAXPROPS);
				memset(okstruct, 0, sizeof(propstruct)*MAXPROPS);

				sprintf(edit[0], "%s", curpage->filename);
				sprintf(edit[1], "%s", topobject->name);
				calcbbox(areawin->topinstance);
				if (curpage->pmode & 2) autoscale(areawin->page);
				writescalevalues(edit[2], edit[3], edit[4]);
				sprintf(edit[5], "%s", (curpage->orient == 0) ? "Portrait" : "Landscape");
				sprintf(edit[6], "%s", (curpage->pmode & 1) ? "Full page" : "Embedded (EPS)");
				data[0] = &(curpage->filename);
				data[1] = topobject->name;
				data[2] = data[3] = data[4] = &(curpage->outscale);
				data[5] = &(curpage->orient);
				data[6] = &(curpage->pmode);

				popup = create_widget(hwnd, "outputpopup");
				popup->bufhdc = (HDC)entertext;
				create_widget(GetDlgItem(hwnd, IDOK), "Write File");
				create_widget(GetDlgItem(hwnd, IDCANCEL), "Close");
				
				sprintf(request, "PostScript output properties (Page %d):", areawin->page + 1);
				sprintf(statics[0], "Filename:");
				sprintf(statics[1], "Page label:");
				sprintf(statics[2], "Scale:");
				if (curpage->coordstyle == CM) {
					sprintf(statics[3], "X Size (cm):");
					sprintf(statics[4], "Y Size (cm):");
				} else {
					sprintf(statics[3], "X Size (in):");
					sprintf(statics[4], "Y Size (in):");
				}
				sprintf(statics[5], "Orientation:");
				sprintf(statics[6], "Mode:");
				SetDlgItemText(hwnd, IDC_PSINFO, request);

				strcpy(request, edit[0]);
				if (strstr(request, ".") == NULL)
					strcat(request, ".ps");
				if (stat(request, &statbuf) == 0) {
					SetWindowText(GetDlgItem(hwnd, IDOK), "Overwrite File");
					Wprintf("  Warning: File exists");
				} else {
					if (errno == ENOTDIR)
						Wprintf("Error: Incorrect pathname");
					else if (errno == EACCES)
						Wprintf("Error: Path not readable");
					else
						Wprintf("  ");
				}

				for (i=0; i<MAXPROPS; i++) {
					SetDlgItemText(hwnd, ID[i].label, statics[i]);
					if (i<5) {
						okstruct[i].textw = entertext[i] = create_widget(GetDlgItem(hwnd, ID[i].edit), "Edit");
						okstruct[i].buttonw = create_widget(GetDlgItem(hwnd, ID[i].apply), "Apply");
						okstruct[i].setvalue = function[i];
						okstruct[i].dataptr = data[i];
						SetWindowText(okstruct[i].textw->handle, edit[i]);

						WinAddCallback(okstruct[i].buttonw, XtNselect, getproptext, &okstruct[i]);
						WinAddCallback(okstruct[i].textw, XtNexecute, getproptext, &okstruct[i]);
						if (update[i] != NULL) {
							WinAddCallback(okstruct[i].buttonw, XtNselect, update[i], entertext);
							WinAddCallback(okstruct[i].textw, XtNexecute, update[i], entertext);
						}
						
						okstruct[i].textw->wndproc = (WNDPROC)SetWindowLong(okstruct[i].textw->handle,
								GWL_WNDPROC, (LONG)OutputDlgEditProc);
					} else {
						entertext[i] = create_widget(GetDlgItem(hwnd, ID[i].edit), "Toggle");
						SendMessage(entertext[i]->handle, CB_SELECTSTRING, -1, (LPARAM)edit[i]);
						WinAddCallback(entertext[i], XtNselect, function[i], data[i]);
						if (update[i] != NULL)
							WinAddCallback(entertext[i], XtNselect, update[i], entertext);
					}
				}

				sprintf(request, "%d Pages", pagelinks(areawin->page));
				win = create_widget(GetDlgItem(hwnd, IDC_PAGENUM), "LToggle");
				SetWindowText(win->handle, request);
				CheckDlgButton(hwnd, IDC_PAGENUM, BST_CHECKED);
				WinAddCallback(win, XtNrelease, linkset, &okstruct[0]);

				win = create_widget(GetDlgItem(hwnd, IDC_AUTOFIT), "Auto-fit");
				SendMessage(win->handle, BM_SETCHECK, (curpage->pmode & 2 ? BST_CHECKED : BST_UNCHECKED), 0);
				WinAddCallback(win, XtNselect, autoset, entertext);
				WinAddCallback(win, XtNrelease, autostop, NULL);

				if (curpage->coordstyle == CM) {
					sprintf(request, "%3.2f x %3.2f cm",
							(float)curpage->pagesize.x / IN_CM_CONVERT,
							(float)curpage->pagesize.y / IN_CM_CONVERT);
				} else {
					sprintf(request, "%3.2f x %3.2f in",
							(float)curpage->pagesize.x / 72.0,
							(float)curpage->pagesize.y / 72.0);
				}
				fpokstruct.textw = create_widget(GetDlgItem(hwnd, IDC_FPEDIT), "fpedit");
				SetWindowText(fpokstruct.textw->handle, request);
				fpokstruct.buttonw = create_widget(GetDlgItem(hwnd, IDC_APPLY6), "fpokay");
				fpokstruct.setvalue = setpagesize;
				fpokstruct.dataptr = &(curpage->pagesize);
				WinAddCallback(fpokstruct.buttonw, XtNselect, getproptext, &fpokstruct);
				WinAddCallback(fpokstruct.buttonw, XtNselect, updatetext, entertext);
				WinAddCallback(fpokstruct.textw, XtNexecute, getproptext, &fpokstruct);
				WinAddCallback(fpokstruct.textw, XtNexecute, updatetext, entertext);

				WinAddCallback(get_widget(GetDlgItem(hwnd, IDOK)), XtNselect, setfile, entertext[0]);

				if (curpage->pmode > 0) {
					XtManageChild(get_widget(GetDlgItem(hwnd, IDC_AUTOFIT)));
					XtManageChild(fpokstruct.textw);
					XtManageChild(fpokstruct.buttonw);
				}

				if (pagelinks(areawin->page) > 1)
					XtManageChild(get_widget(GetDlgItem(hwnd, IDC_PAGENUM)));

				SetFocus(entertext[0]->handle);
			}
			return FALSE;
		case WM_DESTROY:
			if (popup->bufhdc)
				free((Widget*)popup->bufhdc);
			destroy_widget(hwnd);
			{
				int ID[] = { IDOK, IDCANCEL, IDC_APPLY1, IDC_FILENAME, IDC_APPLY2, IDC_PAGELABEL,
					     IDC_APPLY3, IDC_SCALE, IDC_APPLY4, IDC_WIDTH, IDC_APPLY5, IDC_HEIGHT,
					     IDC_PAGENUM, IDC_AUTOFIT, IDC_FPEDIT, -1 };
				int i;

				for (i=0; ID[i]!=-1; i++)
					destroy_widget(GetDlgItem(hwnd, ID[i]));
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					if (HIWORD(wParam) == BN_CLICKED) {
						execute_callback(XtNselect, get_widget((HWND)lParam), NULL);
						EndDialog(hwnd, 1);
					}
					break;
				case IDCANCEL:
					if (HIWORD(wParam) == BN_CLICKED)
						EndDialog(hwnd, 0);
					break;
				case IDC_FILEOPEN:
					{
						OPENFILENAME ofn;
						int len;

						ZeroMemory(&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = hwnd;
						ofn.lpstrFile = _STR2;
						ofn.nMaxFile = 250;
						ofn.lpstrFilter = "XCircuit Files (*.ps;*.eps)\000*.ps;*.eps\000All Files (*.*)\000*.*\000\000\000";
						GetDlgItemText(hwnd, IDC_FILENAME, _STR2, 250);
						change_char(_STR2, '/', '\\');
						if (strstr(_STR2, ".") == NULL)
							strncat(_STR2, ".ps", 250);
						if (GetSaveFileName(&ofn)) {
							change_char(_STR2, '\\', '/');
							len = strlen(_STR2);
							if (len >= 3 && strncmp(_STR2+len-3, ".ps", 3) == 0)
								_STR2[len-3] = '\0';
							SetDlgItemText(hwnd, IDC_FILENAME, _STR2);
							SendDlgItemMessage(hwnd, IDC_APPLY1, BM_CLICK, 0, 0);
						}
					}
					break;
				case IDC_APPLY1:
				case IDC_APPLY2:
				case IDC_APPLY3:
				case IDC_APPLY4:
				case IDC_APPLY5:
				case IDC_APPLY6:
					if (HIWORD(wParam) == BN_CLICKED)
						execute_callback(XtNselect, get_widget((HWND)lParam), NULL);
					break;
				case IDC_PAGENUM:
				case IDC_AUTOFIT:
					if (HIWORD(wParam) == BN_CLICKED) {
						if (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED)
							execute_callback(XtNselect, get_widget((HWND)lParam), NULL);
						else
							execute_callback(XtNrelease, get_widget((HWND)lParam), NULL);
					}
					break;
				case IDC_MODE:
					if (HIWORD(wParam) == CBN_SELENDOK)
						execute_callback(XtNselect, get_widget((HWND)lParam), NULL);
					break;
			}
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

extern int main(int argc, char **argv);

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	LPWSTR *wargv;
	char **argv;
	int argc, i;

	wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	argv = (char**)malloc(sizeof(char*)*argc);
	for (i=0; i<argc; i++) {
		int len = wcslen(wargv[i]);
		argv[i] = (char*)malloc(sizeof(char)*2*(len+1));
		wcstombs(argv[i], wargv[i], 2*(len+1));
		if (argv[i][0] != '-')
			change_char(argv[i], '\\', '/');
	}

	LocalFree(wargv);
	return main(argc, argv);
}

const char* WinBuiltinsDir()
{
	static char buffer[MAX_PATH] = {0};
	
	if (buffer[0] == 0) {
		char *c;
		GetModuleFileName(NULL, buffer, MAX_PATH);
		c = strrchr(buffer, '\\');
		if (c == NULL)
			c = strrchr(buffer, '/');
		if (c != NULL) {
			*c = '\0';
			change_char(buffer, '\\', '/');
		}
	}
	return buffer;
}

static Widget find_menu_from_name(Menu menu, const char *name)
{
	int count, i;
	MENUITEMINFO mi_info;
	MenuItem item;
	Widget found = NULL;

	count = GetMenuItemCount((HMENU)menu->handle);
	ZeroMemory(&mi_info, sizeof(mi_info));
	mi_info.cbSize = sizeof(mi_info);
	mi_info.fMask = MIIM_DATA;
	for (i=0; found == NULL && i<count; i++) {
		mi_info.dwItemData = (LONG)NULL;
		if (GetMenuItemInfo((HMENU)menu->handle, i, MF_BYPOSITION, &mi_info) &&
		    (item = (MenuItem)mi_info.dwItemData) != NULL) {
			if (item->name && strcmp(item->name, name) == 0)
				found = (Widget)item;
			else if (item->popup != NULL)
				found = find_menu_from_name(item->popup, name);
		}
	}
	return found;
}

Widget WinGetMenu(const char *name)
{
	MENUINFO minfo;

	ZeroMemory(&minfo, sizeof(minfo));
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIM_MENUDATA;
	if (GetMenuInfo(GetMenu(topwin), &minfo)) {
		Menu menu = (Menu)minfo.dwMenuData;
		if (name == NULL || *name == '\0')
			return (Widget)menu;
		else
			return find_menu_from_name(menu, name);
	}
	return NULL;
}

int WinIsPopupMenu(Widget w)
{
	if (IS_MENU(w) ||
	    (IS_MENUITEM(w) && TOMENUITEM(w)->popup != NULL))
		return 1;
	return 0;
}

void exec_script_command(void *button, void *data)
{
#ifdef USE_WIN32_COM
	COM_execute(_STR2);
#endif
}

void docommand()
{
	popupprompt(NULL, "Enter command to execute:", "", exec_script_command, NULL, NULL);
}

#ifdef USE_WIN32_COM

void win32_comscript()
{
	COM_runscript(_STR2);
}

void win32_comdotnet()
{
	COM_load_dotnet(_STR2);
}

#endif

void win32_new_window(Widget w, void *clientData, void *callData)
{
	XCWindowData *newwin = GUI_init(0, NULL);

	newwin->page = areawin->page;
	newwin->vscale = areawin->vscale;
	newwin->pcorner = areawin->pcorner;
	newwin->topinstance = areawin->topinstance;

	areawin = newwin;
	ShowWindow(top->handle, SW_SHOW);
}
