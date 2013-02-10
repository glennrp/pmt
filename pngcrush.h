/* pngcrush.h */

/*
 * This software is released under a license derived from the libpng
 * license (see LICENSE, in pngcrush.c).
 */

/* Special defines for pngcrush version 1.7.48 */

#ifndef PNGCRUSH_H
#define PNGCRUSH_H

/* This allows png_default_error() to return, when it is called after our
 * own exception handling, which only returns after "Too many IDAT's",
 * or anything else that we might want to handle as a warning instead of
 * an error.
 */

#if PNGCRUSH_LIBPNG_VER >= 10700
#  define PNG_ABORT exit(1)
#else
#  define PNG_ABORT() exit(1)
#endif

/* Suppress libpng pedantic warnings */
#define PNG_NORETURN    /* This function does not return */

#endif /* !PNGCRUSH_H */
