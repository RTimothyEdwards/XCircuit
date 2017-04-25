/*
 * flate.c
 *
 *   Copyright (c) Sergei Gerasenko 2003-2004. 
 *
 *   This defines the prototypes of the functions used for compressing 
 *   and decompressing PDF streams. All of them utilize the zlib library
 *   and in fact the functions themselves are slightly modified versions 
 *   of those included in an example file that came with the library. Kudos
 *   to the authors of zlib and everybody else involved in its development.
 *
 * This file was taken from the SourceForge project "acroformtool".  Files
 * "compression.c" and "compression.h" have been merged and modified as
 * needed to facilitate compilation in xcircuit.  Thanks once again to the
 * open source community.
 */

#ifdef HAVE_LIBZ

#include <stdio.h>
#include <zlib.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#ifdef TCL_WRAPPER
#include <tk.h>
#else
#include "Xw/Xw.h"
#endif

#include "xcircuit.h"
#include "prototypes.h" /* (jdk) */

/*
 * Substitute for original macro CHECK_ERROR
 * Return 1 on error, 0 on success
 */

int check_error(int err, char *prompt, char *msg) {
   if (err != Z_OK) {
      Fprintf(stderr, "%s error: %d", prompt, err);
      if (msg) Fprintf(stderr, "(%s)", msg);
      Fprintf(stderr, "\n");
      return 1;
   }
   return 0;
}

/*
 * deflate() with large buffers
 */

u_long large_deflate (u_char *compr, u_long compr_len,
	u_char *uncompr, u_long uncompr_len) {

   z_stream c_stream; /* compression stream */
   int err;

   c_stream.zalloc = (alloc_func)0;
   c_stream.zfree = (free_func)0;
   c_stream.opaque = (voidpf)0;

   err = deflateInit(&c_stream, Z_BEST_SPEED);
   if (check_error(err, "deflateInit", c_stream.msg)) return 0;

   c_stream.next_out = compr;
   c_stream.avail_out = (u_int)compr_len;

   c_stream.next_in = uncompr;
   c_stream.avail_in = (u_int)uncompr_len;
   err = deflate(&c_stream, Z_NO_FLUSH);
   if (check_error(err, "deflate", c_stream.msg)) return 0;
   if (c_stream.avail_in != 0) {
      Fprintf(stderr, "deflate not greedy");
   }

   err = deflate(&c_stream, Z_FINISH);
   if (err != Z_STREAM_END) {
      Fprintf(stderr, "deflate should report Z_STREAM_END");
   }
   err = deflateEnd(&c_stream);
   if (check_error(err, "deflateEnd", c_stream.msg)) return 0;

   /* Fprintf(stdout, "large_deflate(): OK\n"); */

   return c_stream.total_out;
}

/* 
 * inflate() with large buffers
 */

u_long large_inflate(u_char *compr, u_long compr_len,
	u_char **uncompr, u_long uncompr_len) {

   char *new_out_start;
   int err;
   z_stream d_stream; /* decompression stream */

   d_stream.zalloc = (alloc_func)0;
   d_stream.zfree = (free_func)0;
   d_stream.opaque = (voidpf)0;

   d_stream.next_in  = compr;
   d_stream.avail_in = (u_int)compr_len;

   err = inflateInit(&d_stream);
   if (check_error(err, "inflateInit", d_stream.msg)) return 0;

   d_stream.next_out = *uncompr;
   d_stream.avail_out = (u_int)uncompr_len;

   for (;;) {
      if (!d_stream.avail_out) {
	 /* Make more memory for the decompression buffer */
	 *uncompr = realloc(*uncompr, uncompr_len * 2);

 	 /* Initialize new memory */
	 new_out_start = *uncompr + uncompr_len;
 	 memset(new_out_start, 0, uncompr_len);

	 /* Point next_out to the next unused byte */
	 d_stream.next_out = new_out_start;

	 /* Update the size of the uncompressed buffer */
	 d_stream.avail_out = (u_int)uncompr_len;
      }

      err = inflate(&d_stream, Z_NO_FLUSH);

      if (err == Z_STREAM_END) break;

      if (check_error(err, "large inflate", d_stream.msg)) return 0;
   }

   err = inflateEnd(&d_stream);
   if (check_error(err, "inflateEnd", d_stream.msg)) return 0;

   /* Fprintf(stdout, "large_inflate(): OK\n"); */

   return d_stream.total_out;
}

#endif /* HAVE_LIBZ */
