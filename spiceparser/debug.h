/********************
    This file is part of the software library CADLIB written by Conrad Ziesler
    Copyright 2003, Conrad Ziesler, all rights reserved.

*************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

******************/
/* debug.h, header controlling inclusion of debugging code, specifically dmalloc 
   Conrad Ziesler
*/



#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>

#include <assert.h>

#ifndef CONFIGURED
#define  DEBUG
#endif


#ifdef DEBUG
#include <dmalloc.h>
#endif

void *cmdline_read_rc(int *p_argc, char ***p_argv, char *path);
void  cmdline_free_rc(void *p);

#ifdef PRODUCTION
#define DEBUG_CMDLINE(c,v,s) NULL
#else
#define DEBUG_CMDLINE(c,v,s) cmdline_read_rc(&c,&v,s)
#endif

