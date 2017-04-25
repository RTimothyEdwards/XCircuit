#ifndef __TKWIN32_H__

#include <windows.h>
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

typedef short Dimension;
typedef Bool Boolean;
typedef unsigned long Pixel;
typedef void* caddr_t;
#define TRUE 1
#define FALSE 0
typedef void* XtAppContext;
typedef struct {
	unsigned int    size;
	XPointer        addr;
} XrmValue, *XrmValuePtr;
#define snprintf _snprintf
#define pipe _pipe
#define unlink _unlink
#define putenv _putenv
#define vsnprintf _vsnprintf
#define XDefineCursor XDefineCursor_TkW32
#define XLookupColor XLookupColor_TkW32
#define XQueryColors XQueryColors_TkW32

extern XFontStruct* XLoadQueryFont(Display *dpy, char *fontname);

#endif
