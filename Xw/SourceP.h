/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        SourceP.h
 **
 **   Project:     X Widgets
 **
 **   Description: private include file for TextEdit widget sources
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

#ifndef _XwTextEditSourcePrivate_h
#define _XwTextEditSourcePrivate_h

#define applySource(method) (*(self->text.source->method))

#define XwEstringSrc "stringsrc"
#define XwEdiskSrc   "disksrc"

typedef struct _StringSourceData {
    XwEditType     editMode;
    unsigned char  *buffer;
    unsigned char  *initial_string;
    XwTextPosition length,          /* current data size of buffer */
                   buffer_size,     /* storage size of buffer */
                   max_size;        /* user specified buffer limit */
    int            max_size_flag;   /* flag to test max_size set */
} StringSourceData, *StringSourcePtr;

  
#endif
/* DON'T ADD STUFF AFTER THIS #endif */
