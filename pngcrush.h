/* pngcrush.h */

/*
 * This software is released under a license derived from the libpng
 * license (see LICENSE, in pngcrush.c).
 */

/* Special defines for pngcrush version 1.7.49 */

#ifndef PNGCRUSH_H
#define PNGCRUSH_H

#undef PNG_PROGRESSIVE_READ_SUPPORTED

#if PNG_LIBPNG_VER >= 10700
#  undef PNG_SIMPLIFIED_READ_AFIRST_SUPPORTED
#  undef PNG_SIMPLIFIED_READ_BGR_SUPPORTED
#  undef PNG_SIMPLIFIED_READ_SUPPORTED
#  undef PNG_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
#  undef PNG_SIMPLIFIED_WRITE_BGR_SUPPORTED
#  undef PNG_SIMPLIFIED_WRITE_SUPPORTED
#endif

#if PNG_LIBPNG_VER < 10400
/* This allows png_default_error() to return, when it is called after our
 * own exception handling, which only returns after "Too many IDAT's",
 * or anything else that we might want to handle as a warning instead of
 * an error.  Doesn't work in libpng-1.4.0 and later.
 */
#  undef PNG_ABORT()
#  define PNG_ABORT()
/* Suppress libpng pedantic warnings */
#define PNG_NORETURN /* This function does not return */
#endif

#endif /* !PNGCRUSH_H */
