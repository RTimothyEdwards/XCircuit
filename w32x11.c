#include <tk.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xcircuit.h>

#include <tk.h>
#include <tkPlatDecls.h>

#include <X11/cursorfont.h>

extern Tcl_Interp *xcinterp;
extern XCWindowData *areawin;
extern char* TkKeysymToString(KeySym ks);

static void unimplemented(const char *str)
{
	printf(">>>>>>>>> Unimplemented: %s\n", str);
}

typedef struct font_item {
	Tk_Font tkfont;
	Font fid;
	struct font_item *next;
};
static struct font_item *font_map = NULL;

static void add_tkfont(Tk_Font tkfont, Font fid)
{
	struct font_item *item = (struct font_item*)malloc(sizeof(struct font_item));

	item->next = font_map;
	font_map = item;
	item->tkfont = tkfont;
	item->fid = fid;
}

static Tk_Font get_tkfont(Font fid)
{
	struct font_item *item = font_map;

	while (item != NULL) {
		if (item->fid == fid)
			return item->tkfont;
		item = item->next;
	}
	fprintf(stderr, "Font not found: ID=%d\n", fid);
	return (Tk_Font)NULL;
}

#ifndef STATIC_BUILD
int XDrawPoint(Display *dpy, Drawable d, GC gc, int x, int y)
{
	HDC hdc;
	HWND hwnd;

	hwnd = Tk_GetHWND(d);
	if (IsWindow(hwnd)) {
		hdc = GetDC(hwnd);
		SetPixelV(hdc, x, y, gc->foreground);
		ReleaseDC(hwnd, hdc);
		hdc = NULL;
	} else {
		/* This is pretty slow when called a lot
		hdc = CreateCompatibleDC(NULL);
		SelectObject(hdc, hwnd);
		SetPixelV(hdc, x, y, gc->foreground);
		DeleteDC(hdc);
		*/
	}
	return 1;
}
#endif

int XClearArea(Display *dpy, Window w, int x, int y, unsigned int width, unsigned int height, Bool exposures)
{
	HWND hnd;
	RECT r;
	HDC hdc;
	int oldROP;

	hnd = Tk_GetHWND(w);
	hdc = GetDC(hnd);
	oldROP = SetROP2(hdc, R2_COPYPEN);
	GetClientRect(hnd, &r);
	if (width != 0 || height != 0) {
		r.left = x;
		r.top = y;
		r.right = x+width;
		r.bottom = y+height;
		FillRect(hdc, &r, (HBRUSH)(COLOR_WINDOW+1));
	}
	SetROP2(hdc, oldROP);
	ReleaseDC(hnd, hdc);
	return 1;
}

Pixmap XCreatePixmap(Display *dpy, Drawable d, unsigned int width, unsigned int height, unsigned int depth)
{
	return Tk_GetPixmap(dpy, d, width, height, depth);
}

int XLookupString(XKeyEvent *event, char *buf, int buflen, KeySym *keysym, XComposeStatus *status)
{
#if 0
	printf("code : %04x\n", event->keycode);
	printf("state: %04x\n", event->state);
#endif
	switch (event->keycode) {
	  case VK_NUMPAD0:
	  case VK_NUMPAD1:
	  case VK_NUMPAD2:
	  case VK_NUMPAD3:
	  case VK_NUMPAD4:
	  case VK_NUMPAD5:
	  case VK_NUMPAD6:
	  case VK_NUMPAD7:
	  case VK_NUMPAD8:
	  case VK_NUMPAD9:
		  *keysym = XK_KP_0 + (event->keycode - VK_NUMPAD0);
		  event->state |= ShiftMask;
		  break;
	  default:
		  *keysym = XKeycodeToKeysym(NULL, event->keycode, event->state);
		  break;
	}
	return 1;
}

Bool XCheckWindowEvent(Display *dpy, Window w, long event_mask, XEvent *event)
{
	unimplemented("XCheckWindowEvent");
	return False;
}

int XTextWidth(XFontStruct *font, char *string, int len)
{
	Tk_Font tkfont = get_tkfont(font->fid);
	return Tk_TextWidth(tkfont, string, len);
}

int XDrawString(Display *dpy, Drawable d, GC gc, int x, int y, char *string, int len)
{
	Tk_Font tkfont = get_tkfont(gc->font);
	Tk_DrawChars(dpy, d, gc, tkfont, string, len, x, y); 
	return 1;
}

int XFreePixmap(Display *dpy, Pixmap pix)
{
	Tk_FreePixmap(dpy, pix);
	return 1;
}

#ifndef STATIC_BUILD
int XPutImage(Display *dpy, Drawable d, GC gc, XImage *img, int src_x, int src_y, int dest_x, int dest_y, unsigned int width, unsigned int height)
{
	// unimplemented("XPutImage");

	TkPutImage(NULL, 0, dpy, d, gc, img, src_x, src_y, dest_x, dest_y,
			width, height);
	return 1;
}
#endif

// The following two functions may be defined as macros. . .

#ifndef XSync
int XSync(Display *dpy, Bool discard)
{
	unimplemented("XSync");
	return 1;
}
#endif

#ifndef XFlush
int XFlush(Display *dpy)
{
	unimplemented("XFlush");
	return 1;
}
#endif

#ifndef STATIC_BUILD
Cursor XCreateFontCursor(Display *dpy, unsigned int shape)
{
	Tk_Window win = Tk_MainWindow(xcinterp);

	switch (shape) {
		case XC_xterm: return (Cursor)Tk_GetCursor(xcinterp, win, "xterm");
		case XC_watch: return (Cursor)Tk_GetCursor(xcinterp, win, "watch");
		default: return (Cursor)NULL;
	}
}
#endif

void XDefineCursor_TkW32(Display *dpy, Window w, Cursor c)
{
	Tk_DefineCursor(Tk_IdToWindow(dpy, w), (Tk_Cursor)c);
}

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
	*mask = reverse_byte(~pixsrc & pixmask);
}

typedef struct tkw32cursor {
	struct tkcursor {
		Tk_Cursor cursor;
		Display *display;
		int resourceCount;
		int objRefCount;
		Tcl_HashTable *otherTable;
		Tcl_HashEntry *hashPtr;
		Tcl_HashEntry *idHashPtr;
		struct tkcursor *nextPtr;
	} info;
	HCURSOR winCursor;
	int system;
} w32cursor;

static
Cursor Tk_GetCursorFromData_TkW32(Tcl_Interp *interp, Tk_Window w, u_char *src, u_char *mask, int width, int height, int xhot, int yhot, Tk_Uid fg, Tk_Uid bg)
{
	w32cursor *wcursor;

	wcursor = (w32cursor*)ckalloc(sizeof(w32cursor));
	wcursor->info.cursor = (Tk_Cursor)wcursor;
	wcursor->winCursor = NULL;
	wcursor->system = 0;

	wcursor->winCursor = CreateCursor(Tk_GetHINSTANCE(), xhot, yhot, width, height, src, mask);
	if (wcursor->winCursor == NULL) {
		ckfree((char*)wcursor);
		return (Cursor)NULL;
	}

	return (Cursor)wcursor;
}

Cursor CreateW32Cursor(Tcl_Interp *interp, Tk_Window w, u_char *src, u_char *mask, int width, int height, int xhot, int yhot, Tk_Uid fg, Tk_Uid bg)
{
	u_char *new_src, *new_mask;
	int nb = (width-1)/(8*sizeof(char))+1;
	int nb2 = (GetSystemMetrics(SM_CXCURSOR)-1)/(8*sizeof(char))+1, height2 = GetSystemMetrics(SM_CYCURSOR);
	int i, j, idx1 = 0, idx2 = 0;
	Cursor cursor;

	new_src = (u_char*)malloc(sizeof(char)*nb2*height2);
	new_mask = (u_char*)malloc(sizeof(char)*nb2*height2);

	for (j=0; j<height; j++) {
		for (i=0; i<nb; i++, idx1++, idx2++) {
			new_src[idx2] = src[idx1];
			new_mask[idx2] = mask[idx1];
			compute_cursor_src_mask(&new_src[idx2], &new_mask[idx2]);
			/*printf("%02x ", new_src[idx2]);*/
		}
		for (i=0; i<(nb2-nb); i++, idx2++) {
			new_src[idx2] = 0xff;
			new_mask[idx2] = 0x00;
			/*printf("%02x ", new_src[idx2]);*/
		}
		/*printf("\n");*/
	}
	for (j=0; j<(height2-height); j++) {
		for (i=0; i<nb2; i++, idx2++) {
			new_src[idx2] = 0xff;
			new_mask[idx2] = 0x00;
			/*printf("%02x ", new_src[idx2]);*/
		}
		/*printf("\n");*/
	}
	/*printf("\n");*/

	cursor = Tk_GetCursorFromData_TkW32(interp, w, new_src, new_mask, nb2*8, height2, xhot, yhot, fg, bg);

	free(new_src);
	free(new_mask);

	return cursor;
}

int XRecolorCursor(Display *dpy, Cursor cursor, XColor *foreground, XColor *background)
{
	unimplemented("XRecolorCursor");
	return 1;
}

Status XAllocNamedColor(Display *dpy, Colormap cm, char* cname, XColor *screen_return, XColor *exact_return)
{
	XColor *c = Tk_GetColor(xcinterp, (areawin ? areawin->area : NULL), cname);

	if (c != NULL) {
		screen_return->pixel = c->pixel;
		exact_return->pixel = c->pixel;
		return True;
	}
	return False;
}

Status XLookupColor_TkW32(Display *dpy, Colormap cmap, const char *name, XColor *cvcolor, XColor *cvexact)
{
	if (XParseColor(dpy, cmap, name, cvcolor)) {
		return True;
	}
	else {
		return False;
	}
}

int XQueryColors_TkW32(Display *dpy, Colormap cmap, XColor *colors, int ncolors)
{
	int i;

	for (i=0; i<ncolors; i++) {
		int pixel = colors[i].pixel;
		colors[i].red = ((pixel&0x000000ff)<<8)|(pixel&0x000000ff);
		colors[i].green = (pixel&0x0000ff00)|((pixel&0x0000ff00)>>8);
		colors[i].blue = ((pixel&0x00ff0000)>>8)|((pixel&0x00ff0000)>>16);
	}

	return 1;
}

Bool XQueryPointer_TkW32(Display *dpy, Window w, Window *root_return, Window *child_return,
		int *root_x, int *root_y, int *win_x, int *win_y, unsigned int *mask)
{
	POINT p;

	GetCursorPos(&p);
	*root_x = p.x;
	*root_y = p.y;
	ScreenToClient(Tk_GetHWND(w), &p);
	*win_x = p.x;
	*win_y = p.y;
	*mask = 0;

	return True;
}

Colormap XCopyColormapAndFree(Display *dpy, Colormap cmap)
{
	unimplemented("XCopyColormapAndFree");
	return cmap;
}

char* XDisplayString(Display *dpy)
{
	/*printf("XDisplayString\n");*/
	return "localhost:0.0\n";
}


char* XKeysymToString_TkW32(KeySym ks)
{
	return TkKeysymToString(ks);
}

XFontStruct* XLoadQueryFont(Display *dpy, char *fontname)
{
	Tk_Font tkfont;
	Tk_FontMetrics tkfm;
	XFontStruct *fs;

	tkfont = Tk_GetFont(xcinterp, Tk_MainWindow(xcinterp), fontname);
	if (tkfont != NULL)
	{
		fs = (XFontStruct*)malloc(sizeof(XFontStruct));
		fs->fid = Tk_FontId(tkfont);
		Tk_GetFontMetrics(tkfont, &tkfm);
		fs->ascent = tkfm.ascent;
		fs->descent = tkfm.descent;
		add_tkfont(tkfont, fs->fid);
		return fs;
	}
	else
		return NULL;
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
