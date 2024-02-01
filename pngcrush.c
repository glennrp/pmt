
/* pngcrush.c - recompresses png files
 * Copyright (C) 1998-2002, 2006-2017 Glenn Randers-Pehrson
 *                                   (glennrp at users.sf.net)
 * Portions Copyright (C) 2005 Greg Roelofs
 */

#define PNGCRUSH_VERSION "1.8.14"

#undef BLOCKY_DEINTERLACE

/* This software is released under a license derived from the libpng
 * license (see LICENSE, below).
 *
 * The most recent version of pngcrush can be found at SourceForge in
 * http://pmt.sf.net/pngcrush/
 *
 * This program reads in a PNG image, and writes it out again, with the
 * optimum filter_method and zlib_level.  It uses brute force (trying
 * filter_method none, and libpng adaptive filtering, with compression
 * levels 3 and 9).
 *
 * Optionally, it can remove unwanted chunks or add gAMA, sRGB, bKGD,
 * tEXt/zTXt, and tRNS chunks.  It will remove some chunks such as gAMA,
 * cHRM, pHYs, and oFFs when their data fields contain all zero, which is a
 * mistake.
 *
 * Uses libpng and zlib.  This program was based upon libpng's pngtest.c.
 *
 * NOTICES:
 *
 * If you have modified this source, you may insert additional notices
 * immediately after this sentence.
 *
 * COPYRIGHT:
 *
 * Copyright (C) 1998-2002, 2006-2017 Glenn Randers-Pehrson
 *                                   (glennrp at users.sf.net)
 * Portions Copyright (C) 2005 Greg Roelofs
 *
 * LICENSE:
 *
 * Permission is hereby irrevocably granted to everyone to use, copy, modify,
 * and distribute this source code, or portions hereof, or executable programs
 * compiled from it, for any purpose, without payment of any fee, subject to
 * the following restrictions:
 *
 * 1. The origin of this source code must not be misrepresented.
 *
 * 2. Altered versions must be plainly marked as such and must not be
 *    misrepresented as being the original source.
 *
 * 3. This Copyright notice, disclaimers, and license may not be removed
 *    or altered from any source or altered source distribution.
 *
 * DISCLAIMERS:
 *
 * The pngcrush computer program is supplied "AS IS".  The Author disclaims all
 * warranties, expressed or implied, including, without limitation, the
 * warranties of merchantability and of fitness for any purpose.  The
 * Author assumes no liability for direct, indirect, incidental, special,
 * exemplary, or consequential damages, which may result from the use of
 * the computer program, even if advised of the possibility of such damage.
 * There is no warranty against interference with your enjoyment of the
 * computer program or against infringement.  There is no warranty that my
 * efforts or the computer program will fulfill any of your particular purposes
 * or needs.  This computer program is provided with all faults, and the entire
 * risk of satisfactory quality, performance, accuracy, and effort is with
 * the user.
 *
 * EXPORT CONTROL
 *
 * I am not a lawyer, but I believe that the Export Control Classification
 * Number (ECCN) for pngcrush is EAR99, which means not subject to export
 * controls or International Traffic in Arms Regulations (ITAR) because it
 * and cexcept.c, libpng, and zlib, which may be bundled with pngcrush, are
 * all open source, publicly available software, that do not contain any
 * encryption software. See the EAR, paragraphs 734.3(b)(3) and 734.7(b).
 *
 * TRADEMARK:
 *
 * The name "pngcrush" has not been registered by the Copyright owner
 * as a trademark in any jurisdiction.  However, because pngcrush has
 * been distributed and maintained world-wide, continually since 1998,
 * the Copyright owner claims "common-law trademark protection" in any
 * jurisdiction where common-law trademark is recognized.
 *
 * CEXCEPT COPYRIGHT, DISCLAIMER, and LICENSE:
 *
 * The cexcept.h header file which is bundled with this software
 * is conveyed under the license and disclaimer described in lines 10
 * through 18 of cexcept.h.
 *
 * LIBPNG COPYRIGHT, DISCLAIMER, and LICENSE:
 *
 * If libpng is bundled with this software, it is conveyed under the
 * libpng license (see COPYRIGHT NOTICE, DISCLAIMER, and LICENSE, in png.h).
 *
 * If intel_init.c and filter_sse2_intrinsics.c are bundled with this
 * software, they are conveyed under the libpng license (see the
 * copyright notices within those files and the COPYRIGHT NOTICE, DISCLAIMER,
 * and LICENSE in png.h).
 *
 * ZLIB COPYRIGHT, DISCLAIMER, and LICENSE:
 *
 * If zlib is bundled with this software, it is conveyed under the
 * zlib license (see the copyright notice, disclaimer, and license
 * appearing in zlib.h).
 *
 * ACKNOWLEDGMENTS:
 *
 * Thanks to Greg Roelofs for various bug fixes, suggestions, and
 * occasionally creating Linux executables.
 *
 * Thanks to Stephan Levavej for some helpful suggestions about gcc compiler
 * options and for a suggestion to increase the Z_MEM_LEVEL from default.
 *
 * Thanks to others who have made bug reports and suggestions mentioned
 * in the change log.
 *
 * CAUTION:
 *
 * There is another version of pngcrush that has been distributed by
 * Apple since mid-2008 as a part of the Xcode SDK.   Although it claims
 * to be pngcrush by Glenn Randers-Pehrson, it has additional options
 * "-iPhone", "-speed", "-revert-iphone-optimizations", and perhaps others.
 * It is an "altered version".  I've seen output from a 2006 version that
 * says on its help screen, "and modified by Apple as indicated in the
 * sources".
 *
 * It writes files that have the PNG 8-byte signature but are not valid PNG
 * files (instead they are "IOS-optimized PNG files"), due to at least
 *
 *  1. the presence of the CgBI chunk ahead of the IHDR chunk;
 *  2. nonstandard deflate compression in IDAT, iCCP, and perhaps zTXt chunks
 *     (I believe this only amounts to the omission of the zlib header from
 *     the IDAT and perhaps other compressed chunks);
 *  3. Omission of the CRC bytes from the IDAT chunk and perhaps other chunks;
 *  4. the use of premultiplied alpha in color_type 6 files; and
 *  5. the sample order, which is ARGB instead of RGBA in color_type 6 files.
 *
 * See http://iphonedevwiki.net/index.php/CgBI_file_format for more info.
 *
 * Although there is no loss in converting a CgBI PNG back to a regular
 * PNG file, the original PNG file cannot be losslessly recovered from such
 * files because of the losses that occurred during the conversion to
 * premultiplied alpha.
 *
 * Most PNG decoders will recognize the fact that an unknown critical
 * chunk "CgBI" is present and will immediately reject the file.
 *
 * It is said that the Xcode version of pngcrush is automatically applied
 * when PNG files are prepared for downloading to the iPhone unless the
 * user takes special measures to prevent it.
 *
 * It is said that the Xcode pngcrush does have a command to undo the
 * premultiplied alpha.  It's not theoretically possible, however, to recover
 * the original file without loss.  The underlying color data will either be
 * reduced in precision, or, in the case of fully-transparent pixels,
 * completely lost.
 *
 * I have not seen the source for the Xcode version of pngcrush.  All I
 * know, for now, is from running "strings -a" on an old copy of the
 * executable, looking at two Xcode-PNG files, and reading Apple's patent
 * application <http://www.freepatentsonline.com/y2008/0177769.html>.  Anyone
 * who does have access to the revised pngcrush code cannot show it to me
 * anyhow because of their Non-Disclosure Agreement with Apple.
 */

/* To do (TODO, TO DO):
 *
 *   (As noted below, some of the features that aren't yet implemented
 *   in pngcrush are already available in ImageMagick; you can try a
 *   workflow that makes a first pass over the image with ImageMagick
 *   to select the bit depth, color type, interlacing, etc., and then makes
 *   another pass with pngcrush to optimize the compression, and finally
 *   makes a pass with libpng's "pngfix" app to optimize the zlib CMF
 *   bytes.)
 *
 *   0. Make pngcrush optionally operate as a filter, writing the crushed PNG
 *   to standard output, if an output file is not given, and reading
 *   the source file from standard input if no input file is given.
 *
 *   1. Reset CINFO to reflect decoder's required window size (instead of
 *   libz-1.1.3 encoder's required window size, which is 262 bytes larger).
 *   See discussion about zlib in png-list archives for April 2001.
 *   libpng-1.2.9 does some of this and libpng-1.5.4 does better.
 *   But neither has access to the entire datastream, so pngcrush could
 *   do even better.
 *
 *   This has no effect on the "crushed" filesize.  The reason for setting
 *   CINFO properly is to provide the *decoder* with information that will
 *   allow it to request only the minimum amount of memory required to decode
 *   the image (note that libpng-based decoders don't make use of this
 *   hint, because of the large number of files found in the wild that have
 *   incorrect CMF bytes).
 *
 *   In the meantime, one can just run
 *
 *      pc input.png crushed.png
 *
 *      where the "pc" script is the following:
 *
 *      #!/bin/sh
 *      cp $1 temp-$$.png
 *      for w in 512 1 2 4 8 16 32
 *      do
 *      pngcrush -ow -w $w -brute temp-$$.png
 *      done
 *      mv temp-$$.png $2
 *
 *   It turns out that sometimes this finds a smaller compressed PNG
 *   than plain "pngcrush -brute input.png crushed.png" finds.
 *
 *   There are several ways that pngcrush could implement this.
 *
 *      a. Revise the bundled zlib to report the maximum window size that
 *      it actually used, then rewrite CINFO to contain the next power-of-two
 *      size equal or larger than the size.  This method would of course
 *      only work when pngcrush is built with the bundled zlib, and won't
 *      work with zopfli compression.
 *
 *      b. Do additional trials after the best filter method, strategy,
 *      and compression level have been determined, using those settings
 *      and reducing the window size until the measured filesize increases,
 *      then choosing the smallest size which did not cause the filesize
 *      to increase.  This is expensive.
 *
 *      c. After the trials are complete, replace CINFO with smaller
 *      settings, then attempt to decode the zlib datastream, and choose
 *      the smallest setting whose datastream can still be decoded
 *      successfully.  This is likely to be the simplest and fastest
 *      solution; however, it will only work with a version of libpng
 *      in which the decoder actually uses the CINFO hint.
 *      This seems to be the only method that would work with zopfli
 *      compression which always writes "7" (i.e., a 32k window) in CINFO.
 *
 *      d. The simplest is to use pngfix, which comes with libpng16 and
 *      later, as a final step:
 *
 *           pngcrush input.png temp.png
 *           pngfix --optimize --out=output.png temp.png
 *           # To do: find out if this works with zopfli output
 *
 *   2. Check for the possiblity of using the tRNS chunk instead of
 *      the full alpha channel.  If all of the transparent pixels are
 *      fully transparent, and they all have the same underlying color,
 *      and no opaque pixel has that same color, then write a tRNS
 *      chunk and reduce the color-type to 0 or 2. This is a lossless
 *      operation.  ImageMagick already does this, as of version 6.7.0.
 *      If the lossy "-blacken" option is present, do that operation first.
 *
 *   3. Add choice of interlaced or non-interlaced output. Currently you
 *   can change interlaced to non-interlaced and vice versa by using
 *   ImageMagick before running pngcrush.  Note, when implementing this,
 *   disallow changing interlacing if APNG chunks are being copied.
 *
 *   4. Use a better compression algorithm for "deflating" (result must
 *   still be readable with zlib!)  e.g., http://en.wikipedia.org/wiki/7-Zip
 *   says that the 7-zip deflate compressor achieves better compression
 *   (smaller files) than zlib.  If tests show that this would be worth
 *   while, incorporate the 7-zip compressor as an optional alternative
 *   or additional method of pngcrush compression. See the GPL-licensed code
 *   at http://en.wikipedia.org/wiki/AdvanceCOMP and note that if this
 *   is incorporated in pngcrush, then pngcrush would have to be re-licensed,
 *   or released in two versions, one libpng-licensed and one GPL-licensed!
 *
 *   Also consider Google's "zopfli" compressor, which is said to slow but
 *   achieves better compression.  It is Apache-2.0 licensed and available from
 *   a GIT repository at SourceForge (see https://code.google.com/p/zopfli/).
 *   See also the "pngzop" directory under the pmt.sourceforge.net project.
 *   Note paragraph 1.c above; zopfli always writes "7" in CINFO.  See
 *   my "pmt/pngzop" project at SourceForge and GitHub.
 *
 *   5. Optionally recognize any sRGB iCCP profile and replace it with the
 *   sRGB chunk.  Turn this option on if the "-reduce" option is on.  Also,
 *   if "-reduce" is on, delete any gAMA and cHRM chunks if the sRGB chunk
 *   is being written.
 *
 *   6. Accept "--long-option".  For starters, just accept -long_option
 *   and --long_option equally.
 *
 *   7. Implement a "copy_idat" option that simply copies the IDAT data,
 *   implemented as "-m 176". This will still repackage the IDAT chunks
 *   in a possibly different IDAT chunk size.
 *
 *   8. Implement palette-building (from ImageMagick-6.7.0 or later, minus
 *   the "PNG8" part) -- actually ImageMagick puts the transparent colors
 *   first, then the semitransparent colors, and finally the opaque colors,
 *   and does not sort colors by frequency of use but just adds them
 *   to the palette/colormap as it encounters them, so it might be improved.
 *   Also it might be made faster by using a hash table as was partially
 *   implemented in pngcrush-1.6.x.  If the latter is done, also port that
 *   back to ImageMagick/GraphicsMagick.  See also ppmhist from the NetPBM
 *   package which counts RGB pixels in an image; this and its supporting
 *   lib/libppmcmap.c would need to be revised to count RGBA pixels instead.
 *
 *   9. Improve the -help output and/or write a good man page.
 *
 *   10. Finish pplt (MNG partial palette) feature.
 *
 *   11. Remove text-handling and color-handling features and put
 *   those in a separate program or programs, to avoid unnecessary
 *   recompressing.  Note that in pngcrush-1.7.34, pngcrush began doing
 *   this extra work only once instead of for every trial, so the potential
 *   benefit in CPU savings is much smaller now.
 *
 *   12. Add a "pcRu" ancillary chunk that keeps track of the best method,
 *   methods already tried, and whether "loco crushing" was effective.
 *
 *   13. Try both transformed and untransformed colors when "-loco" is used.
 *
 *   14. Move the Photoshop-fixing stuff into a separate program.
 *
 *   15. GRR: More generally (superset of previous 3 items):  split into
 *   separate "edit" and "crush" programs (or functions).  Former is fully
 *   libpng-aware, much like current pngcrush; latter makes little or no use of
 *   libpng (maybe IDAT-compression parts only?), instead handling virtually
 *   all chunks as opaque binary blocks that are copied to output file _once_,
 *   with IDATs alone replaced (either by best in-memory result or by original
 *   _data_ resplit into bigger IDATs, if pngcrush cannot match/beat).  "edit"
 *   version should be similar to current code but more efficient:  make
 *   _one_ pass through args list, creating table of PNG_UINTs for removal;
 *   then make initial pass through PNG image, creating (in-order) table of
 *   all chunks (and byte offsets?) and marking each as "keep" or "remove"
 *   according to args table.  Could start with static table of 16 or 32 slots,
 *   then double size & copy if run out of room:  still O(n) algorithm.
 *
 *   16. With libpng17, png_write throws an "affirm()" with tc.format=256
 *   when attempting to write a sub-8-bit grayscale image.
 *
 *   17. Figure out why we aren't calling png_read_update_info() and fix.
 *
 *   18. Fix ADLER32 checksum handling in conjunction with iCCP chunk
 *   reading.
 *
 *   19. Fix Coverity "TOCTOU" warning about our "stat()" usage.
 *
 *   20. Warn about removing copyright and license info appearing in
 *   PNG text chunks.
 *
 *   21. Implement the "eXIf" chunk if it is approved by the PNG
 *   Development Group.  Optionally, remove preview and thumbnail
 *   images from the Exif profile contained in the eXIf chunk.
 *
 */

#if 0 /* changelog */

Change log:

Version 1.8.14 (built with libpng-1.6.34 and zlib-1.2.11)
  Recognize the "-bail" option properly (bug fix by Hadrien Lacour).
  Fix documentation about "-force/-noforce" to represent the default
    behavior since 1.8.1 (bug report by Hadrien Lacour).

Version 1.8.13 (built with libpng-1.6.32 and zlib-1.2.11)
  Add "exit(0)" after processing "-version" argument, to avoid
    displaying the Usage information (bug report by Peter Hagan,
    Issue #76).
  Fix problem with MacOS prior to Sierra; it uses CLOCK_MONOTONIC
    for some other purpose (bug report and help developing patch by
    Github user "ilovezfs", Homebrew/homebrew-core PR#16391).

Version 1.8.12 (built with libpng-1.6.31 and zlib-1.2.11)
  Added POWERPC-VSX support.
  Report whether using optimizations.
  Added filter_method 6 (same as filter 5 with -speed).
  Added "methods" 149-176 (that use filter_method 6).
  Changed default verbosity from 1 (normal) to 0 (quiet). Use "-v" to get
    the previous default behavior and "-v -v" to get the previous "verbose"
    behavior. The "-s" (silent) and "-q" (quiet) options behave as before.

Version 1.8.11 (built with libpng-1.6.28 and zlib-1.2.11)
  Use png_set_option(PNG_IGNORE_ADLER32) to control ADLER32 handling.
  Changed LD=gcc to LD=$(CC) in Makefile and Makefile-nolib (suggested
    by Helmut G in Bug#850927 of some GIT project)

Version 1.8.10 (built with libpng-1.6.26 and zlib-1.2.8.1)
  Changed ADLER32 checksum handling to only use inflateValidate()
    during IDAT chunk handling; it broke iCCP chunk handling.

Version 1.8.9 (built with libpng-1.6.26 and zlib-1.2.8.1)
  Added "-warn" option, to show only warnings.
  Enabled method 149. For now it writes uncompressed IDAT but in
    a future version of pngcrush it will just copy the IDAT data.

Version 1.8.8 (built with libpng-1.6.26beta06 and zlib-1.2.8.1)
  Fixed "nolib" build (bug report by Hanspeter Niederstrasser).
    Make sure we use system-png.h, and not the local file.  It is now
    possible to build either the regular pngcrush or the "nolib"
    pngcrush in the complete pngcrush source directory (use
    "make clean" before rebuilding!)
  Fixed timing when using "clock()". Sometimes an additional second
    was added when the timer crossed a one-second boundary, since
    version 1.8.5.
  Upgrade libpng to version 1.6.26beta06 and zlib to 1.2.8.1.
  Use zlib-1.2.8.1 new "inflateValidate()" function to avoid checking
    ADLER32 checksums. Version 1.8.7 did not work when the "-fix"
    option was used.

Version 1.8.7 (built with libpng-1.6.25 and zlib-1.2.8)
  Do not check the ADLER32 CRC while reading except during the final write
    pass (requires libpng-1.6.26 or later and zlib-1.2.4 or later).
    This saves some CPU time, around five to ten percent, in decoding.
  Do not calculate the ADLER32 CRC while writing except during the final
    write pass (writing raw deflate streams instead of zlib streams to
    the IDAT chunks; these are invalid PNGs but since all we do is count
    the bytes that does not matter). This saves some CPU time in encoding
    but it is barely perceptible. Requires zlib 1.2.4 or later and modified
    pngwutil.c.

Version 1.8.6 (built with libpng-1.6.25 and zlib-1.2.8)
  Enabled ARM_NEON support.
  Fixed error in handling of timer wraparound when interval exceeds one
    second.
  Disable high resolution timers by default in Makefile.  To enable them,
    you must enable/disable relevant CPPFLAGS and LIBS in Makefile.

Version 1.8.5 (built with libpng-1.6.24 and zlib-1.2.8)
  Added "-benchmark n" option.  It runs the main loop "n" times, and
    records the minimum value for each timer.
  After checking for CLOCK_ID, use clock() if none is found.
  Avoid some timing when verbose<0 ("-s" or "--silent")
  Added PNGCRUSH_CHECK_CRC (off by default) to use libpng default
    CRC checking. Otherwise, CRC are only computed and checked during
    the first read pass and while writing.
  Accept "--option" (for now, by simply skipping the first "-").

Version 1.8.4 (built with libpng-1.6.24 and zlib-1.2.8)
  Fixed handling of CLOCK_ID, removed some "//"-delimited comments.
    Revised intel_init.c to always optimize 4bpp images, because the
    poor optimization noted previously has been fixed.

Version 1.8.3 (built with libpng-1.6.24 and zlib-1.2.8)
  Fixed bug introduced in 1.8.2 that causes trial 10 to be skipped when
    using the default heuristic method.
  Fixed incorrect typecast in call to png_create_write_struct_2() (Bug
    report by Richard K. Lloyd).
  Added intel_init.c, filter_sse2_intrinsics.c, and Makefile-sse,
    to enable INTEL SSE optimization.  Revised intel_init.c to optionally
    only optimize 4bpp images, because the optimization appears to slow
    down reading of 3bpp images.
  Added PNGCRUSH_TIMERS (nanosecond resolution using clock_gettime), to
    measure defiltering time in png_read_filter_row(). For now, this only
    works with a modified libpng, on platforms that provide "clock_gettime()".
    To enable all timers, define PNGCRUSH_TIMERS=11.
  Revised "-q" to show a short summary of results (final size and timing)
  To do: Add ZLIB_AMALGAMATED configuration; currently produces different output
    https://blog.forrestthewoods.com/
    improving-open-source-with-amalgamation-cf293592c5f4#.g9fb2tyhs
    (added #ifndef NO_GZ / #endif to skip the gz* code in zlib_amalg.[ch]).
  Added LIBPNG_UNIFIED configuration.

Version 1.8.2 (built with libpng-1.6.23 and zlib-1.2.8)
  Fixed filesize reduction report when "-ow" option is used (Bug report #68).
  When a single method is specified, turn -reduce off by default and skip
    trial 0.

Version 1.8.1 (built with libpng-1.6.21 and zlib-1.2.8)
  Added the LICENSE file to the tar and zip distributions.
  Made "-force" force output even when the IDAT is larger, and added
    "-noforce" option; "-noforce" is now the default behavior (Bug
    report #68 at SourceForge by "wintakeall")
  Use right filename in filesize reduction report (overwrite?inname:outname)
    (bug report #69 by "wintakeall").
  Removed some superfluous spaces from the Copyright statement.
  Added "-speed" option; it avoids using the AVG or PAETH filters which
    are slower to decode.

Version 1.8.0 (built with libpng-1.6.21 and zlib-1.2.8)
  Made "-reduce" and "-force" the default behavior.  Removed obsolete
    options "-plte_len", "-cc", "-nocc", "-double_gamma", "-already_crushed",
    and "-bit_depth". Removed "things_have_changed" code.

Version 1.7.92 (built with libpng-1.6.20 and zlib-1.2.8)
  Deleted png_read_update_info() statement that was mistakenly added to
    version 1.7.89. It caused "bad adaptive filter value" errors.

Version 1.7.91 (built with libpng-1.6.20 and zlib-1.2.8)
  Suppress warning about "damaged LZ stream" when bailing out and building
    with libpng-1.7.0beta.
  Added a LICENSE file to the distribution. It points to the actual
    license appearing in the NOTICES section near the top of pngcrush.c
  Show if pngcrush is built with bundled or system libpng and zlib.
  Fixed segfault while writing a -loco MNG (bug found with AFL, reported
    by Brian Carpenter). Bug was introduced in pngcrush-1.7.35.

Version 1.7.88 (built with libpng-1.6.19 and zlib-1.2.8)
  Eliminated a potential overflow while adding iTXt chunk (over-length
    text_lang or text_lang_key), reported by Coverity.

Version 1.7.87 (built with libpng-1.6.18 and zlib-1.2.8)
  Fixed a double-free bug (CVE-2015-7700). There was a "free" of the
    sPLT chunk structure in pngcrush and then again in png.c (Bug report
    by Brian Carpenter).
  Added common-law trademark notice and export control information.
  Rearranged some paragraphs in the comments at the beginning of pngcrush.c
  Increased some buffer sizes in an attempt to prevent possible overflows.

Version 1.7.86 (built with libpng-1.6.18 and zlib-1.2.8)
  Increased maximum size of a text chunk input from 260 to 2048
    (STR_BUF_SIZE) bytes, to agree with the help screen (bug report by
    Tamas Jursonovics).
  Fixed bug that caused text chunks after IDAT to be written only when
    the "-save" option is used.

Version 1.7.85 (built with libpng-1.6.16 and zlib-1.2.8)
  Improved reporting of invalid chunk names. Does not try to put
    non-printable characters in STDERR; displays hex numbers instead.
  Fixed include path for utime.h on MSVC (Louis McLaughlin).
  Eliminated "FAR" memory support (it was removed from libpng at version
    1.6.0).
  Disabled the "-already_crushed" option which does not really work well.

Version 1.7.84 (built with libpng-1.6.16 and zlib-1.2.8)
  Cleaned up more Coverity-scan warnings. Fixing those also fixed
    CVE-2015-2158.

Version 1.7.83 (built with libpng-1.6.16 and zlib-1.2.8)
  Cleaned up some Coverity-scan warnings.  Unfortunately one of these
    changes introduced the vulnerability reported in CVE-2015-2158.

Version 1.7.82 (built with libpng-1.6.16 and zlib-1.2.8)

Version 1.7.81 (built with libpng-1.6.15 and zlib-1.2.8)
  Fixed off-by-one error in calculation of plte_len. Bug reports by
    Ivan Kuchin and Frederic Kayser.

Version 1.7.80 (built with libpng-1.6.14 and zlib-1.2.8)
  Added "-reduce_palette" and "-noreduce_palette" options.  Enable
    reduce_palette when the "-new" or "-reduce" option is used.

Version 1.7.79 (built with libpng-1.6.14 and zlib-1.2.8)
  Fixed bug in -plte_len N option.

Version 1.7.78 (built with libpng-1.6.14 and zlib-1.2.8)
  Made "-s" and "-silent" options suppress libpng warnings.

Version 1.7.77 (built with libpng-1.6.13 and zlib-1.2.8)
  Updated libpng to version 1.6.13.

Version 1.7.76 (built with libpng-1.6.12 and zlib-1.2.8)
  Updated libpng to version 1.6.12.

Version 1.7.75 (built with libpng-1.6.10 and zlib-1.2.8)
  Reverted libpng to version 1.6.10 due to a misplaced statement in png.c

Version 1.7.74 (built with libpng-1.6.11 and zlib-1.2.8)
  Fixed "-zmem" option (only "-zm" would work since version 1.7.62).

Version 1.7.73 (built with libpng-1.6.10 and zlib-1.2.8)
  Restored calls to png_set_crc_action() which had been removed from
    version 1.7.72 for some testing and inadvertently not restored. 
  Changed "fix" internal variable name to "salvage" (still set with "-fix")
  Added code to fix/salvage PNG with "bad adaptive filter value" error.
  Avoid calculating CRC during compression trials except for the last trial,
    when the output is actually written.
  Fixed a bug with reducing 16-bit images to 8-bit using "-reduce" option.
    
Version 1.7.72 (built with libpng-1.6.10 and zlib-1.2.8)

Version 1.7.71 (built with libpng-1.6.9 and zlib-1.2.8)
  Built the Windows binaries using -DTOO_FAR=32767; neglected to do this
    in versions 1.7.42 through 1.7.70, which caused the Windows binaries
    to produce different (usually a few bytes larger) results than Linux.
    Thanks to F. Kayser for reporting the discrepancy.

Version 1.7.70 (built with libpng-1.6.8 and zlib-1.2.8)

Version 1.7.69 (built with libpng-1.6.6 and zlib-1.2.8)
  Updated libpng to version 1.6.6.

Version 1.7.68 (built with libpng-1.6.4 and zlib-1.2.8)
  Check for NULL return from malloc().
  Undefine CLOCKS_PER_SECOND "1000" found in some version of MinGW.
  Replaced most "atoi(argv[++i])" with "pngcrush_get_long" which does
    "BUMP_I; strtol(argv[i],ptr,10)" and added pngcrush_check_long macro
    to detect malformed or missing parameters (debian bug 716149).
  Added global_things_have_changed=1 when reading -bkgd.
  The "-bit_depth N" option did not work reliably and has been removed.

Version 1.7.67 (built with libpng-1.5.17 and zlib-1.2.8)
  Fixed handling of "-text" and "-ztext" options for text input. They had been
    reduced to "-t" and "-z" with an incorrect argument (3 instead of 2) in
    version 1.7.62. Bug report and patch from Tsukasa Oi.

Version 1.7.66 (built with libpng-1.5.17 and zlib-1.2.8)
  Revised pngcrush_examine_pixels_fn() to fix some incorrect reductions.

Version 1.7.65 (built with libpng-1.5.17 and zlib-1.2.8)
  Do not allow any colortype or depth reductions if acTL is present.
  Added warnings to explain why any requested reductions were not allowed.

Version 1.7.64 (built with libpng-1.5.17 and zlib-1.2.8)

Version 1.7.63 (built with libpng-1.5.16 and zlib-1.2.8)
  Add "int dowildcard=-1;" in an attempt to get wildcard arguments working
    in the cross-compiled MinGW executables.

Version 1.7.62 (built with libpng-1.5.16 and zlib-1.2.8)
  Remove old filename before renaming, when using the "-ow" option on
    any Windows platform, not just CYGWIN (see log entry for pngcrush-1.7.43).
  Reverted error with handling single-character options like "-v", introduced
    in 1.7.61.

Version 1.7.61 (built with libpng-1.5.16 and zlib-1.2.8)
  Check sBIT chunk data to see if reduction to gray or to 8-bit is permitted,
    i.e., the RGB sBIT values are equal to each other or the sBIT values are
    not greater than 8, respectively.
  Do not try to make_opaque if the tRNS chunk is found.
  Added warning when ignoring an invalid commandline option.
  Improved brute_force handling with specified level, filter, or strategy.

Version 1.7.60 (built with libpng-1.5.16 and zlib-1.2.8)
  Revise -reduce so reducing from color-type 6 to grayscale works.
  Issue a warning if reducing bit depth or color type would violate various
    chunk dependencies, and do not perform the action:
    Do not reduce to grayscale if a color bKGD chunk, sBIT or iCCP chunk
       is present.
    Do not reduce bit depth if bKGD or sBIT chunk is present.
    Do not reduce palette length if the hIST chunk is present.
  Set "found_iCCP" flag to zero to avoid re-reading a bad iCCP chunk.

Version 1.7.59 (built with libpng-1.5.16 and zlib-1.2.8)
  Show the acTL chunk in the chunk list in verbose output.
  Fixed several bugs reported by pornel at users.sf.net:
    Do not call png_set_benign_errors when PNG_BENIGN_ERRORS_SUPPORTED
      is not defined
    Renamed PNG_UNUSED() macro PNGCRUSH_UNUSED().
    Moved a closing bracket inside the PNGCRUSH_LOCO block.
    Moved the declaration of "new_mng" outside a PNGCRUSH_LOCO block.
    Put reference to "input_format" inside a PNGCRUSH_LOCO block.
    Moved declarations of mng_out and mngname inside a PNGCRUSH_LOCO block.

Version 1.7.58 (built with libpng-1.5.15 and zlib-1.2.7-1)
  Do not enable reduce_palette by default for "-reduce", "-new", or "-old".
    It still is failing for some files.

Version 1.7.57 (built with libpng-1.5.15 and zlib-1.2.7-1)
  Added "-new" option that turns on "-reduce" which will be
    the default setting for version 1.8.0 and beyond.
  Added "-old" option that turns off "-reduce" which is the
    current default setting.
  Updated copyright year for zlib-1.2.7-1.
  Reverted to libpng-1.5.15 to be able to read old PNG files with TOO FAR
    errors.  This will of course only work with the embedded libpng.

Version 1.7.56 (built with libpng-1.6.1 and zlib-1.2.7-1)
  Only use pngcrush_debug_malloc() and pngcrush_debug_free() if the result
    is going to be shown.
  Added PNG_PASS_ROWS, PNG_UNUSED, and other macro definitions, when building
    with libpng-1.4.x and older libpng versions.
  Multiplied rowbytes by 8/bit_depth when using the system library because
    we do not call png_read_transform_info(). This prevents a crash when
    reading sub-8-bit input files.

Version 1.7.55 (built with libpng-1.6.1 and zlib-1.2.7-1)

Version 1.7.54 (built with libpng-1.6.1rc01 and zlib-1.2.7-1)

Version 1.7.53 (built with libpng-1.6.1rc01 and zlib-1.2.7)
  Removed plte_len stuff from the "To do" list because it is done.
  Shorten the indexed-PNG tRNS chunk length if it has more entries than the PLTE chunk.

Version 1.7.52 (built with libpng-1.6.1beta06 and zlib-1.2.7)
  Added license info for cexcept.h, libpng, and zlib.
  Added consideration of "zopfli" compression to the "To do" list.
  Fixed a typo that caused a cHRM chunk to be "found" if an iCCP chunk
    were present.
  Reset best_byte_count before trial loop.
  Revise global png_set_keep_unknown_chunks() calls to avoid a libpng16
    warning.
  Reset "intent" to "specified_intent" before trial loop.
  Reset "plte_len" to "specified_plte_len" before trial loop.
  Initialize length of each trial to 0x7fffffff so any untried method
    is not the "best method".
 
Version 1.7.51 (built with libpng-1.6.0 and zlib-1.2.7)
  Added "-noreduce" option, in preparation for "-reduce" becoming the
    default behaviour in version 1.8.0.  This turns off lossless bit depth,
    color type, palette reduction, and opaque alpha channel removal.
  Zero out the high byte of transparent color for color-type 0 and 2,
    when reducing from 16 bits to 8.
  Undefined a bunch of stuff in pngcrush.h that we do not use, saves about
    100 kbytes of executable file size in addition to about 50k saved by
    undefining the simplified API.
  Fixed double-underscore typo in an #ifdef in png.c
  If "-reduce" is on and the background index is larger than the reduced
    palette_length+1, reduce it to the palette_length+1.
  Increased required_window_size if necessary to account for slightly larger
    size of interlaced files due to additional filter bytes and padding.

Version 1.7.50 (built with libpng-1.6.0 and zlib-1.2.7)
  Removed completed items from the "To do" list.
  Ignore the argument of the "plte_len" argument and just set the
    "reduce_palette" flag.

Version 1.7.49 (built with libpng-1.5.14 and zlib-1.2.7)
  Use png_set_benign_errors() to allow certain errors in the input file
    to be handled as warnings.
  Skip PNG_ABORT redefinition when using libpng-1.4.0 and later.
  Implemented "-reduce" option to identify and reduce all-gray images,
    all-opaque images, unused PLTE entries, and 16-bit images that can be
    reduced losslessly to 8-bit.

Version 1.7.48 (built with libpng-1.5.14 and zlib-1.2.7)
  Reserved method==0 for examining the pixels during trial 0, if necessary.
    Changed blacken_fn() to separate pngcrush_examine_pixels_fn() and
    pngcrush_transform_pixels_fn() callback functions.  The "examine"
    function is only done during trial 0; it sets a flag whether
    any fully transparent pixels were found, and pngcrush only runs
    pngcrush_transform_pixels_fn() if necessary.
  This is in preparation for future versions, which will examine other
    conditions such as if the image is opaque or gray or can be losslessly
    reduced in bit depth, set flags in trial 0 and accomplish the
    transformations in the remaining trials (see the To do list starting
    about line 200 in the pngcrush.c source).
  Removed "PNGCRUSH_COUNT_COLORS" blocks again.

Version 1.7.47 (built with libpng-1.5.13 and zlib-1.2.7)
  Do not do the heuristic trials of the first 10 methods when -brute is
    specified, because it did not save time as I hoped.
  Fixed a mistake in 1.7.45 and 1.7.46 that caused the output file to
    not be written.

Version 1.7.46 (built with libpng-1.5.13 and zlib-1.2.7)
  Moved the new level 0 methods to the end of the trial list (methods 137-148)

Version 1.7.45 (built with libpng-1.5.13 and zlib-1.2.7)
  Added method 0 (uncompressed). "-m 0" now simply turns on method 0.
  Added "-try10" option that has the same effect that "-m 0" previously did,
    namely to try only the first ten methods.
  Inserted new methods 17 through 21 with zlib level 0.
  Do the heuristic trials of the first 10 methods when -brute is specified,
    to get quickly to a small solution, so we can bail out of most of the
    remaining trials early. Previously these 10 methods were skipped during
    a -brute run.
  Removed the "-reduce" line from the help screen when PNGCRUSH_COUNT_COLORS
    is disabled.

Version 1.7.44 (built with libpng-1.5.14 and zlib-1.2.7)

Version 1.7.43 (built with libpng-1.5.13 and zlib-1.2.7)
  Added "remove(inname)" before "rename(outname, inname)" when using the "-ow"
    option on CYGWIN/MinGW because "rename()" does not work if the target file
    exists.
  Use the bundled "zlib.h" when PNGCRUSH_H is defined, otherwise use the
    system <zlib.h>.

Version 1.7.42 (built with libpng-1.5.13 and zlib-1.2.7)
  Use malloc() and free() instead of png_malloc_default() and
    png_free_default().  This will be required to run with libpng-1.7.x.
  Revised the PNG_ABORT definition in pngcrush.h to work with libpng-1.7.x.
  Revised zutil.h to avoid redefining ptrdiff_t on MinGW/CYGWIN platforms.

Version 1.7.41 (built with libpng-1.5.13 and zlib-1.2.7)
  Reverted to version 1.7.38.  Versions 1.7.39 and 1.7.40 failed to
    open an output file.

Version 1.7.40 (built with libpng-1.5.13 and zlib-1.2.7)
  Revised the "To do" list.

Version 1.7.39 (built with libpng-1.5.13 and zlib-1.2.7)
  Removed "PNGCRUSH_COUNT_COLORS" blocks which I no longer intend to
    implement because that feature is already available in ImageMagick.  Kept
    "reduce_to_gray" and "it_is_opaque" flags which I do hope to implement
    soon.
  Changed NULL to pngcrush_default_read_data in png_set_read_fn() calls, to fix
    an insignificant error introduced in pngcrush-1.7.14, that caused most
    reads to not go through the alternate read function.  Also always set this
    function, instead of depending on STDIO_SUPPORTED.

Version 1.7.38 (built with libpng-1.5.13 and zlib-1.2.7)
  Bail out of a trial if byte count exceeds best byte count so far.  This
    avoids wasting CPU time on trial compressions of trials that exceed the
    best compression found so far.
  Added -bail and -nobail options.  Use -nobail to get a complete report
    of filesizes; otherwise the report just says ">N" for any trial
    that exceeds size N where N is the best size achieved so far.
  Added -blacken option, to enable changing the color samples of any
    fully-transparent pixels to zero in PNG files with color-type 4 or 6,
    potentially improving their compressibility. Note that this is an
    irreversible lossy change: the underlying colors of all fully transparent
    pixels are lost, if they were not already black.

Version 1.7.37 (built with libpng-1.5.12 and zlib-1.2.7)
  Reverted pngcrush.c back to 1.7.35 and fixed the bug with PLTE handling.

Version 1.7.36 (built with libpng-1.5.12 and zlib-1.2.7)
  Reverted pngcrush.c to version 1.7.34 because pngcrush is failing with
    some paletted PNGs.
  Separated CFLAGS and CPPFLAGS in the makefile (with "-I" and "-DZ_SOLO"
    in CPPFLAGS)

Version 1.7.35 (built with libpng-1.5.12 and zlib-1.2.7)
  Removed FOPEN of fpout except for the last trial.  The open files caused
    "pngcrush -brute -e _ext.png *.png" to fail on the 10th file (about the
    1024th compression trial) due to being unable to open the output file.

Version 1.7.34 (built with libpng-1.5.12 and zlib-1.2.7)
  Compute and report sum of critical chunk lengths IHDR, PLTE, IDAT, and IEND,
    plus the 8-byte PNG signature instead of just the total IDAT data length.
    Simplify finding the lengths from the trial compressions, by replacing
    the write function with one that simply counts the bytes that would have
    been written to a trial PNG, instead of actually writing a PNG, reading it
    back, and counting the IDAT bytes.
  Removed comments about the system library having to be libpng14 or earlier.
    This restriction was fixed in version 1.7.20.

Version 1.7.33  (built with libpng-1.5.12 and zlib-1.2.7)
  Ignore all ancillary chunks except during the final trial.  This can be
    significantly faster when large ancillary chunks such as iCCP and zTXt
    are present.

Version 1.7.32  (built with libpng-1.5.12 and zlib-1.2.7)
  Fixed bug introduced in 1.7.30: Do not call png_set_check_for_invalid_index()
    when nosave != 0 (otherwise pngcrush crashes with the "-n" option).

Version 1.7.31  (built with libpng-1.5.11 and zlib-1.2.7)
  Dropped *.tar.bz2 from distribution.
  Added a comma that was missing from one of the "usage" strings (error
    introduced in version 1.7.29).

Version 1.7.30  (built with libpng-1.5.11 and zlib-1.2.7)
  Only run the new (in libpng-1.5.10) test of palette indexes during the
    first trial.

Version 1.7.29  (built with libpng-1.5.10 and zlib-1.2.7)
  Set "things_have_changed" flag when adding text chunks, so the "-force"
    option is no longer necessary when adding text to an already-compressed
    file.
  Direct usage message and error messages to stderr instead of stdout. If
    anyone is still using DOS they may have to change the "if 0" at line
    990 to "if 1".  If you need to have the messages on standard output
    as in the past, use 2>&1 to redirect them.
  Added "pngcrush -n -v files.png" to the usage message.

Version 1.7.28  (built with libpng-1.5.10 and zlib-1.2.7)
  Write proper copyright year for zlib, depending upon ZLIB_VERNUM

Version 1.7.27  (built with libpng-1.5.10 and zlib-1.2.6)
  Increased row_buf malloc to row_bytes+64 instead of row_bytes+16, to
    match the size of big_row_buf in pngrutil.c (it is 48 in libpng14, 15, 16,
    and 64 in libpng10, 12.  Otherwise there is a double-free crash when the
    row_buf is destroyed.

Version 1.7.26  (built with libpng-1.5.10 and zlib-1.2.6)
  Increased the text_text buffer from 2048 to 10*2048 (Ralph Giles), and
    changed an incorrect test for keyword length "< 180" to "< 80".  The
    text_text buffer was inadvertently reduced from 20480 to 2048 in
    pngcrush-1.7.9.
  Added -DZ_SOLO to CFLAGS, needed to compile zlib-1.2.6.
  Changed user limits to width and height max 500000, malloc max 2MB,
    cache max 500.
  Added -nolimits option which sets the user limits to the default
    unlimited values.

Version 1.7.25  (built with libpng-1.5.9 and zlib-1.2.5)

Version 1.7.24  (built with libpng-1.5.7 and zlib-1.2.5)
  Do not append a slash to the directory name if it already has one.

Version 1.7.23  (built with libpng-1.5.7 and zlib-1.2.5)
  Ignore any attempt to use "-ow" with the "-d" or "-e" options, with warning.
  Include zlib.h if ZLIB_H is not defined (instead of checking the libpng
    version; see entry below for pngcrush-1.7.14), and include string.h
    if _STRING_H_ is not defined (because libpng-1.6 does not include string.h)
  Define SLASH = backslash on Windows platforms so the "-d" option will work..

Version 1.7.22  (built with libpng-1.5.6 and zlib-1.2.5)
  Added "-ow" (overwrite) option.  The input file is overwritten and the
    output file is just used temporarily and removed after it is copied
    over the input file..  If you do not specify an output file, "pngout.png"
    is used as the temporary file. Caution: the temporary file must be on
    the same filesystem as the input file.  Contributed by a group of students
    of the University of Paris who were taking the "Understanding of Programs"
    course and wished to gain familiarity with an open-source program.

Version 1.7.21  (built with libpng-1.5.6 and zlib-1.2.5)
  Defined TOO_FAR=32767 in Makefile (instead of in pngcrush.h)

Version 1.7.20  (built with libpng-1.5.5 and zlib-1.2.5)
  Removed the call to png_read_transform_info() when the system libpng
    is being used, so it can be built with a system libpng.

Version 1.7.19  (built with libpng-1.5.5 and zlib-1.2.5)
  pngcrush-1.7.18 failed to read interlaced PNGs.  Reverted the change
    from calling png_read_transform_info() to png_read_update_info().
    Since png_read_transform_info() is not exported we again cannot build
    with the system libpng15.

Version 1.7.18  (built with libpng-1.5.5 and zlib-1.2.5)
  This version will work with either a "system" libpng14 or libpng15, or with
    the embedded libpng15.  The deprecated usage of libpng png_struct members
    and unexported functions has been removed.
  Fixing "too far back" errors does not work with libpng15.
  Revised the format of the time report (all on one line so you can get
    a nice compact report by piping the output to "grep coding").

Version 1.7.17  (built with libpng-1.5.5beta08 and zlib-1.2.5)
  Changed "#if !defined(PNG_NO_STDIO)" to "#ifdef PNG_STDIO_SUPPORTED"
    as recommended in the libpng documentation.
  Added PNG_UINT_32_NAME macro and used it to simplify chunk_type integer
    definitions.

Version 1.7.16  (built with libpng-1.5.4 and zlib-1.2.5)
  Only report best method==0 if pngcrush cannot match the input filesize.
    Otherwise, if there is no improvement, report the first matching method.

Version 1.7.15  (built with libpng-1.5.2rc02 and zlib-1.2.5)
  Force bit_depth to 1, 2, or 4 when -plte_len is <=2, <=4, or <=16 and
    the -bit_depth option is not present, to avoid writing invalid palette
    indexes.

Version 1.7.14  (built with libpng-1.5.1beta08 and zlib-1.2.5)
  Removed WIN32_WCE support (libpng has dropped it already)
  Include zlib.h and define png_memcpy, etc., and revise the
    png_get_iCCP() and png_set_iCCP() calls to be able to build
    with bundled libpng-1.5.x.  Pngcrush cannot be built yet with
    a system libpng-1.5.x.
  Dropped most of pngcrush.h, that eliminates various parts of libpng.

Version 1.7.13  (built with libpng-1.4.5 and zlib-1.2.5)

Version 1.7.12  (built with libpng-1.4.4beta05 and zlib-1.2.5)

Version 1.7.11  (built with libpng-1.4.2 and zlib-1.2.5)

Version 1.7.10  (built with libpng-1.4.1 and zlib-1.2.3.9)
  Added missing "(...)" in png_get_uint_32().
  Only compile png_get_uint_32(), etc., when PNG_LIBPNG_VER < 1.2.9
  Revised help info for "-zitxt".

Version 1.7.9  (built with libpng-1.4.1 and zlib-1.2.3.9)
  Defined TOO_FAR == 32767 in pngcrush.h (instead of in deflate.c)
  Revised the "nolib" Makefiles to remove reference to gzio.c and
    pnggccrd.c
  Imposed user limits of chunk_malloc_max=4000000 and chunk_cache_max=500.

Version 1.7.8  (built with libpng-1.4.0 and zlib-1.2.3.5)
  Removed gzio.c

Version 1.7.7  (built with libpng-1.4.0 and zlib-1.2.3.4)
  Updated bundled libpng to version 1.4.0.
  Check the "-plte_len n" option for out-of-range value of n.
  Changed local variable "write" to "z_write" in inffast.c (zlib-1.2.3.4)
    to avoid shadowed declaration warning.

Version 1.7.6  (built with libpng-1.4.0rc02 and zlib-1.2.3.2)
  Change some "#if defined(X)" to "#ifdef X" according to libpng coding style.
  Added some defines to suppress pedantic warnings from libpng-1.2.41beta15
    and later.  A warning about deprecated access to png_ptr->zstream is
    otherwise unavoidable.  When building the embedded libpng, a warning
    about png_default_error() returning is also otherwise unavoidable.
  Write premultiplied alpha if output extension is .ppng and
    PNG_READ_PREMULTIPLIED_ALPHA_SUPPORTED is set (needs libpng-1.5.0).
  Check the "-m method" option for out-of-range method value.

Version 1.7.5  (built with libpng-1.2.41beta14 and zlib-1.2.3.2)

Version 1.7.4  (built with libpng-1.2.40rc01 and zlib-1.2.3.2)
  Use unmodified pngconf.h from libpng-1.2.41beta05 or later.

Version 1.7.3  (built with libpng-1.2.40 and zlib-1.2.3.2)
  Print contents of text chunks after IDAT, even when the -n option
    is used.  This requires a slight modification of pngconf.h,
    when libpng-1.2.x is used.

Version 1.7.2  (built with libpng-1.2.40 and zlib-1.2.3.2)
  Added check for "verbose" on some printf statements.

Version 1.7.1  (built with libpng-1.2.39 and zlib-1.2.3.2)
  Revised some prototypes to eliminate "Shadowed Declaration" warnings.
  Moved warning about discarding APNG chunks to the end.
  Replaced *.tar.lzma with *.tar.xz in the distribution.

Version 1.7.0  (built with libpng-1.2.38 and zlib-1.2.3.2)
  Save (but do not recompress) APNG chunks if the output file has the
    ".apng" extension and the color_type and bit_depth are not changed.

Version 1.6.20 (built with libpng-1.2.38 and zlib-1.2.3.2)
  Changed local variable "write" to "wwrite" in inffast.c (zlib) to avoid
    shadowed declaration warning.

Version 1.6.19 (built with libpng-1.2.37 and zlib-1.2.3.2)
  Added missing braces that cause an incorrect png_error() to be issued.

Version 1.6.18 (built with libpng-1.2.37 and zlib-1.2.3.2)
  Removed extra FCLOSE(fpin) and FCLOSE(fpout) in the first Catch{} block,
    since they get removed anyway right after that (Hanno Boeck).
  Define PNG_NO_READ|WRITE_cHRM and PNG_NO_READ_|WRITEiCCP in pngcrush.h
    and reordered pngcrush.h

Version 1.6.17 (built with libpng-1.2.36 and zlib-1.2.3.2)
  Defined TOO_FAR == 32767 in deflate.c (again).  The definition
    has continually been inadvertently omitted during zlib updates
    since pngcrush version 1.6.4.
  Revised handling of xcode files so at least we can get printout
    of IHDR values with "pngcrush -fix -n -v xcode.png".
  Moved ChangeLog.txt back into pngcrush.c so it does not get lost.
  Removed single quotes from the ChangeLog.

Version 1.6.16 (built with libpng-1.2.35 and zlib-1.2.3.2)
  Added -newtimestamp and -oldtimestamp options and changed
    default condition to timestamping the output file with
    the current time (i.e., -newtimestamp is default)
  If the -oldtimestamp option is used then the output file
    has the same timestamp as the input file.
  Added CgBI chunk detection.

Version 1.6.15 (built with libpng-1.2.35 and zlib-1.2.3.2)
  Fixes some missing typecasts on png_malloc() calls, patch from
    an anonymous reporter to the SourceForge bug tracker.
  Added -time_stamp option to change time stamping from default
    condition.

Version 1.6.14 (built with libpng-1.2.35 and zlib-1.2.3.2)
  Avoids CVE-2009-0040.

Version 1.6.12 (built with libpng-1.2.34 and zlib-1.2.3.2)

Version 1.6.11 (built with libpng-1.2.33 and zlib-1.2.3.2)
  Eliminated a memory leak in libpng with writing bad tEXt chunks.

Version 1.6.10 (built with libpng-1.2.31 and zlib-1.2.3.2)
  Add sTER chunk support.

Version 1.6.9 (built with libpng-1.2.31 and zlib-1.2.3.2)
  Updated cexcept.h to version 2.0.1
  Add missing curly brackets.

Version 1.6.8 (built with libpng-1.2.29 and zlib-1.2.3.2)
  Fixed bug with handling of -z and -zi options.

Version 1.6.7 (built with libpng-1.2.29 and zlib-1.2.3.2)
  Moved PNG_UINT_CHNK and some other defines from pngcrush.h to pngcrush.c
  Reject invalid color_type or bit_depth.

Version 1.6.6 (built with libpng-1.2.29 and zlib-1.2.3.2)
  Added dSIG support.  Pngcrush will not rewrite an image containing
  a dSIG chunk immediately following the IHDR chunk, unless the
  dSIG is explicitly removed with "-rem dSIG" or explicitly kept
  with "-keep dSIG".  In the latter case the saved dSIG chunks will
  become invalid if any changes are made to the datastream.

  Fixed bug in writing unknown chunks from the end_info_ptr.

Version 1.6.5 (built with libpng-1.2.29 and zlib-1.2.3.2)
  Discontinued adding a new gAMA chunk when writing sRGB chunk.

Version 1.6.4 (built with libpng-1.2.9rc1 and zlib-1.2.3)
  Fixed bug in handling of undocumented -trns_a option (Michal Politowski).
  Fixed bug with "nosave" handling of unknown chunks.

Version 1.6.3 (built with libpng-1.2.9beta11 and zlib-1.2.3)

  Fixed documentation of iTXt input (Shlomi Tal).
  Removed #define PNG_INTERNAL and provided prototypes for some
  internal libpng functions that are duplicated in pngcrush.c

Version 1.6.2 (built with libpng-1.2.8 and zlib-1.2.3)

  Fixed bug with "PNG_ROWBYTES" usage, introduced in version 1.6.0.
  The bug could cause a crash and only affects the "nolib" builds.

  Converted C++ style (// ...) comments to C style (/* ... */).

  Defined TOO_FAR == 32767 in deflate.c (again).  The definition was
  omitted from version 1.6.0 when zlib was upgraded to version 1.2.3.

Version 1.6.1 (distributed as 1.6.0, built with libpng-1.2.8 and zlib-1.2.3)

  Copied non-exported libpng functions from libpng into pngcrush, to make
  pngcrush play more nicely with shared libpng.  These are not compiled
  when a static library is being built with the bundled libpng and
  pngcrush.h is included.

Version 1.6.0-grr (built with libpng-1.2.4 and zlib-1.1.4pc or zlib-1.2.2)

  Moved ChangeLog out of pngcrush.c comments and into a separate file.

  Filtered pngcrush.c through "indent -kr" and "expand" for readability.

  Moved 550 lines of usage/help/copyright/license/version info to separate
  function(s) and cleaned up significantly.

  Added some comments for ease of navigation and readability.

  Stripped out a bunch of ancient-libpng compatibility stuff.

  Defined PNG_UINT_* macros (pngcrush.h for now).

  Fixed unknown-chunk handling ("-rem alla" and "-rem gifx" now work).

  Created modified version of makefile that supports external zlib.

  Added support for methods using Z_RLE zlib strategy (zlib 1.2.x only).

  Documented -huffman option in usage screen.

  Added IDAT statistics to final per-file summary.

  Added utime() support to give output files same timestamps as input files.

Version 1.5.10 (built with libpng-1.2.4 and zlib-1.1.4pc)

  Fixed bug, introduced in 1.5.9, that caused defaults for method 0 to
  be used instead of copying the original image, when the original was
  already smallest.

Version 1.5.9 (built with libpng-1.2.4beta3 and zlib-1.1.4pc)

  Work around CPU timer wraparound at 2G microseconds.

  Upgraded zlib from 1.1.3 to 1.1.4.  Pngcrush is believed not to
  be vulnerable to the zlib-1.1.3 buffer-overflow bug.

  Choose the first instance of smallest IDAT instead of the last,
  for faster final recompression, suggested by TSamuel.

Version 1.5.8 (built with libpng-1.2.1)

  Added -trns_a option for entering a tRNS array.

Version 1.5.7 (built with libpng-1.2.0)

  Added setargv.obj to Makefile.msc to expand wildcards, e.g., *.png

  Use constant string "pngcrush" instead of argv[0] when appropriate.

  Only check stats for infile==outfile once per input file, or not at all
  if "-nofilecheck" option is present or if a directory was created.

  Fixed bugs with changing bit_depth of grayscale images.

Version 1.5.6 (built with libpng-1.0.12)

  Eliminated extra "Removed the cHNK chunk" messages generated by version
  1.5.5 when "-rem alla" or "-rem allb" is used.

  All unknown chunks including safe-to-copy chunks are now removed in
  response to the "-rem alla" or "-rem allb" options.

  Issue a warning if the user tries "-cc" option when it is not supported.

Version 1.5.5 (built with libpng-1.0.12)

  Reset reduce_to_gray and it_is_opaque flags prior to processing each
  image.

  Enable removal of safe-to-copy chunks that are being handled as unknown
  e.g., "-rem time".

Version 1.5.4 (built with libpng-1.0.11)

  Added 262 to the length of uncompressed data when calculating
  required_window_size, to account for zlib/deflate implementation.

  Added "-bit_depth n" to the help screen.

  Call png_set_packing() when increasing bit_depth to 2 or 4.

  Added warning about not overwriting an existing tRNS chunk.

  Reduced the memory usage

  Write 500K IDAT chunks even when system libpng is being used.

  Ignore all-zero cHRM chunks, with a warning.

Version 1.5.3 (built with libpng-1.0.9beta5)

  Added "-loco" option (writes MNG files with filter_method 64)

  "-dir" and "-ext" options are no longer mutually exclusive, e.g.:
  pngcrush -loco -dir Crushed -ext .mng *.png

Version 1.5.2 (built with libpng-1.0.9beta1)

  Added "-iccp" option.

  Increased the zlib memory level, which improves compression (typically
  about 1.3 percent for photos) at the expense of increased memory usage.

  Enabled the "-max max_idat_size" option, even when max_idat_size
  exceeds the default 1/2 megabyte size.

  Added missing "png_ptr" argument to png_error() call

  Added "-loco" option, to enable the LOCO color transformation
  (R->R-G, G, B->B-G) while writing a MNG with filter_method 64. Undo
  the transformation and write the regular PNG filter_method (0) if the
  MNG filter_method 64 is detected.

  Revised the "-help" output slightly and improved the "-version" output.

  The "-already[_crushed]" option is now ignored if the "-force" option
  is present or if chunks are being added, deleted, or modified.

  Improved "things_have_changed" behavior (now, when set in a particular
  file, it is not set for all remaining files)

Version 1.5.1 (built with libpng-1.0.8)

  Disabled color counting by default and made it controllable with new
  -cc and -no_cc commandline arguments.

  Added some #ifdef PNGCRUSH_COUNT_COLORS around code that needs it.

  Revised count_colors() attempting to avoid stack corruption that has
  been observed on RedHat 6.2

  Added the word "irrevocably" to the license and changed "without fee"
  to "without payment of any fee".

Version 1.5.0 (built with libpng-1.0.8)

  After encountering an image with a bad Photoshop iCCP chunk, pngcrush
  1.4.5 through 1.4.8 write sRGB and gAMA=45455 chunks in all
  remaining PNG files on the command line.  This has been fixed so the
  correction is only applied to the particular bad input file.

Version 1.4.8 (built with libpng-1.0.8rc1)

  Detect and remove all-opaque alpha channel.
  Detect and reduce all-gray truecolor images to grayscale.

Version 1.4.7 (built with libpng-1.0.8rc1)

  Restored the "-ext" option that was inadvertently overridden with
  a new "-exit" option in version 1.4.6 ("-exit" is used to force an
  "exit" instead of a "return" from the main program).

Version 1.4.6 (built with libpng-1.0.8rc1)

  Fixed bug in color-counting of noninterlaced images.

  Added capability of processing multiple rows at a time (disabled by
  default because it turns out to be no faster).

  Replaced "return" statements in main() with "exit" statements.
  Force exit instead of return with "-exit" argument.

  Added the UCITA disclaimers to the help output.

Version 1.4.5 (built with libpng-1.0.7rc2 and cexcept-1.0.0)

  Added color-counting and palette-building capability (enable by
  defining PNGCRUSH_COUNT_COLORS).  In a future version, this will
  give pngcrush the ability to reduce RGBA images to indexed-color
  or grayscale when fewer than 257 RGBA combinations are present,
  and no color is present that requires 16-bit precision.  For now,
  it only reports the frequencies.

  Added "-fix" option, for fixing bad CRCs and other correctable
  conditions.

  Write sBIT.alpha=1 when adding an opaque alpha channel and sBIT
  is present.

  Identify the erroneous 2615-byte sRGB monitor profile being written
  by Photoshop 5.5, which causes many apps to crash, and replace it with
  an sRGB chunk.

  Added a check for input and output on different devices before rejecting
  the output file as being the same as the input file based on inode.

  Added some UCITA language to the disclaimer.

Version 1.4.4 (built with libpng-1.0.6i and cexcept-0.6.3)

  Can be built on RISC OS platforms, thanks to Darren Salt.

Version 1.4.3 (built with libpng-1.0.6h and cexcept-0.6.3)

  Reduced scope of Try/Catch blocks to avoid nesting them, and
  removed returns from within the Try blocks, where they are not
  allowed.

  Removed direct access to the png structure when possible, and isolated
  the remaining direct accesses to the png structure into new
  png_get_compression_buffer_size(), png_set_compression_buffer_size(),
  and png_set_unknown_chunk_location() functions that were installed
  in libpng version 1.0.6g.

Version 1.4.2 (built with libpng-1.0.6f and cexcept-0.6.0)

  Removes extra IDAT chunks (such as found in some POV-ray PNGs) with
  a warning instead of bailing out (this feature requires libpng-1.0.6f
  or later, compiled with "#define PNG_ABORT()").

  Removed old setjmp interface entirely.

Version 1.4.1 (built with libpng-1.0.6e and cexcept-0.6.0)

  Uses cexcept.h for error handling instead of the libpng built-in
  setjmp/longjmp mechanism.  See http://cexcept.sf.net/

  Pngcrush.c will now run when compiled with old versions of libpng back
  to version 0.96, although some features will not be available.

Version 1.4.0 (built with libpng-1.0.6 + libpng-1.0.6-patch-a)

Version 1.3.6 (built with libpng-1.0.5v)

  RGB to Grayscale conversion is more accurate (15-bit instead of 8-bit)
  and now uses only integer arithmetic.

  "#ifdefed" out PNG_READ_DITHER

  Changed "Compressed" to "Uncompressed" in help for -itxt.

  Stifled some compiler warnings

Version 1.3.5 (built with libpng-1.0.5s)

  Add test on stat_buf.st_size to verify fpin==fpout, because stat in
  MSVC++6.0 standard version returns stat_buf.st_ino=0 for all files.

  Revised pngcrush.h to make it easier to control PNG_ZBUF_SIZE and
  PNG_NO_FLOATING_POINT_SUPPORTED from a makefile.

  Restored ability to enter "replace_gamma" value as a float even when
  floating point arithmetic is not enabled.

  Enabled removing tEXt, zTXt, or iTXt chunks by chunk type, i.e.,
  "-rem tEXt" only removes tEXt chunks, while "-rem text" removes all
  three types of text chunk.

  Removed definition of TOO_FAR from pngcrush.h

  Uses new libpng error handler; if a file has errors, pngcrush now will
  continue on and compress the remaining files instead of bailing out.

Version 1.3.4 (built with libpng-1.0.5m)

  Do not allow pngcrush to overwrite the input file.

Version 1.3.3 (built with libpng-1.0.5m)

  Restored ability to enter gamma as a float even when floating point
  arithmetic is not enabled.

Version 1.3.2 (built with libpng-1.0.5k)

  Renamed "dirname" to "directory_name" to avoid conflict with "dirname"
  that appears in string.h on some platforms.

  Fixed "PNG_NO_FLOAING_POINT" typo in pngcrush.h

  "#ifdefed" out parts of the help screen for options that are unsupported.

Version 1.3.1 (built with libpng-1.0.5k): Eliminated some spurious warnings
  that were being issued by libpng-1.0.5j.  Added  -itxt, -ztxt, and
  -zitxt descriptions to the help screen.

  Dropped explicit support for pCAL, hIST, sCAL, sPLT, iCCP, tIME, and
  cHRM chunks and handle them as unknown but safe-to-copy instead, using
  new png_handle_as_unknown function available in libpng-1.0.5k.

Version 1.3.0 (built with libpng-1.0.5j): Added support for handling
  unknown chunks.

  pngcrush is now fixed-point only, unless PNG_NO_FLOATING_POINT_SUPPORTED
  is undefined in pngcrush.h.

  Added support for the iCCP, iTXt, sCAL, and sPLT chunks, which
  are now supported by libpng (since libpng-1.0.5j).  None of these have
  been adequately tested.

  "#ifdefed" out more unused code (weighted filters and progressive read;
  this saves about 15k in the size of the executable).

  Moved the special definitions from pngconf.h into a new pngcrush.h

  Disallow 256-byte compression window size when writing, to work around
  an apparent zlib bug.  Either deflate was producing incorrect results in a
  21x21 4-bit image or inflate was decoding it incorrectly; the uncompressed
  stream is 252 bytes, which is uncomfortably close to the resulting
  256-byte compression  window.  This workaround can be removed when zlib
  is fixed.

  The "-m method" can be used any of the 124 methods, without having to
  specify the filter, level, and strategy, instead of just the first 10.

Version 1.2.1 (built with libpng-1.0.5f): Fixed -srgb parameter so it
  really does take an argument, and so it continues to use "0" if an
  integer does not follow the -srgb.

  Added "-plte_len n" argument for truncating the PLTE.  Be sure not to
  truncate it to less than the greatest index actually appearing in IDAT.

Version 1.2.0: Removed registration requirement.  Added open source
  license.  Redefined TOO_FAR=32k in deflate.c.

Changes prior to going "open source":

Version 1.1.8: built with libpng-1.0.5a.  Runs OK with pngvcrd.c.

Version 1.1.7: added ability to add tEXt/zTXt chunks.  Fixed bug with
closing a file that was not opened when using "pngcrush -n".  Fixed
bug with tEXt/zTXt chunks after IDAT not being copied.
Added alpha to the displayed palette table.  Rebuilt with libpng-1.0.5.

Version 1.1.6: fixed bug with one file left open after each image is
processed

Version 1.1.5: Shorten or remove tRNS chunks that are all opaque or have
opaque entries at the end.  Added timing report.

Version 1.1.4: added ability to restrict brute_force to one or more filter
  types, compression levels, or compression strategies.

#endif /* end of changelog */

static int verbose = 0;
static int show_warnings = 0; /* =1 to show warnings even with verbose < 0 */
static int copy_idat = 0; /* = 1 to simply copy the IDAT chunk data */

/* Experimental: define these if you wish, but, good luck.
#define PNGCRUSH_COUNT_COLORS
#define PNGCRUSH_MULTIPLE_ROWS
*/

#define PNGCRUSH_LARGE

#define PNGCRUSH_ROWBYTES(pixel_bits, width) \
    ((pixel_bits) >= 8 ? \
    ((png_size_t)(width) * (((png_size_t)(pixel_bits)) >> 3)) : \
    (( ((png_size_t)(width) * ((png_size_t)(pixel_bits))) + 7) >> 3) )

/* Suppress libpng pedantic warnings */
#if 0
#define PNG_DEPSTRUCT   /* Access to this struct member is deprecated */
#endif

#ifndef PNGCRUSH_TIMERS
# define PNGCRUSH_TIMERS 4
#endif

#if PNGCRUSH_TIMERS > 0

/* TIMER function
   ===== ====================== 
   set in pngcrush.c:
     0   total time
     1   total decode
     2   total encode
     3   total other
   set in pngread.c:
     4   decode deinterlace
     5   decode filter 0 (none)
     6   decode filter 1 (sub)
     7   decode filter 2 (up)
     8   decode filter 3 (avg)
     9   decode filter 4 (paeth)
   set in pngwutil.c:
    10   encode filter setup
*/
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L /* for clock_gettime */

#include <time.h>

#ifdef CLOCKS_PER_SECOND
#  if CLOCKS_PER_SECOND == '"1000"'
#    undef CLOCKS_PER_SEC
#  endif
#endif

#ifndef CLOCKS_PER_SEC
#  define CLOCKS_PER_SEC 1000
#endif

#ifdef __STDC__
#  define TIME_T clock_t
#else
#  if CLOCKS_PER_SEC <= 100
#    define TIME_T long
#  else
#    define TIME_T float
#  endif
#endif

#ifdef __APPLE__
#  include <AvailabilityMacros.h>
#endif

/* As in GraphicsMagick */
#  if PNGCRUSH_USE_CLOCK_GETTIME == 0
#    define PNGCRUSH_USING_CLOCK "clock()"
#  elif defined(CLOCK_HIGHRES) /* Solaris */
#    define PNGCRUSH_CLOCK_ID CLOCK_HIGHRES
#    define PNGCRUSH_USING_CLOCK "clock_gettime(CLOCK_HIGHRES,&t)"
#    define PNGCRUSH_USE_CLOCK_GETTIME 1
#  elif defined(CLOCK_MONOTONIC_RAW) /* Linux */
#    define PNGCRUSH_CLOCK_ID CLOCK_MONOTONIC_RAW
#    define PNGCRUSH_USING_CLOCK "clock_gettime(CLOCK_MONOTONIC_RAW,&t)"
#    define PNGCRUSH_USE_CLOCK_GETTIME 1
#  elif defined(CLOCK_MONOTONIC_PRECISE) /* FreeBSD */
#    define PNGCRUSH_CLOCK_ID CLOCK_MONOTONIC_PRECISE
#    define PNGCRUSH_USING_CLOCK "clock_gettime(CLOCK_MONOTONIC_PRECISE,&t)"
#    define PNGCRUSH_USE_CLOCK_GETTIME 1
#  elif defined(CLOCK_MONOTONIC) /* Linux, FreeBSD & macOS Sierra & up */ && \
     !(defined(__APPLE__) && MAC_OS_X_VERSION_MIN_REQUIRED < 101200)
#    define PNGCRUSH_CLOCK_ID CLOCK_MONOTONIC
#    define PNGCRUSH_USING_CLOCK "clock_gettime(CLOCK_MONOTONIC,&t)"
#    define PNGCRUSH_USE_CLOCK_GETTIME 1
#  else
#    define PNGCRUSH_USING_CLOCK "clock()"
#    undef PNGCRUSH_USE_CLOCK_GETTIME
#    define PNGCRUSH_USE_CLOCK_GETTIME 0
#  endif
#else /* PNGCRUSH_TIMERS */
#    define PNGCRUSH_USING_CLOCK "clock()"
#    define PNGCRUSH_USE_CLOCK_GETTIME 0
#endif

#ifndef LIBPNG_UNIFIED
#include <png.h>
#define PNGCRUSH_TIMER_UINT_API extern unsigned int PNGAPI
#define PNGCRUSH_TIMER_VOID_API extern void PNGAPI
#else
#define PNGCRUSH_TIMER_UINT_API unsigned int
#define PNGCRUSH_TIMER_VOID_API void
#endif

#define PNGCRUSH_TIMER_DECODE 1
#define PNGCRUSH_TIMER_ENCODE 2
#define PNGCRUSH_TIMER_MISC   3
#define PNGCRUSH_TIMER_TOTAL  0 

static unsigned int pngcrush_timer_hits[PNGCRUSH_TIMERS];
static unsigned int pngcrush_timer_secs[PNGCRUSH_TIMERS];
static unsigned int pngcrush_timer_nsec[PNGCRUSH_TIMERS];
static unsigned int pngcrush_clock_secs[PNGCRUSH_TIMERS];
static unsigned int pngcrush_clock_nsec[PNGCRUSH_TIMERS];

PNGCRUSH_TIMER_UINT_API
pngcrush_timer_get_seconds(unsigned int n);
PNGCRUSH_TIMER_UINT_API
pngcrush_timer_get_hits(unsigned int n);
PNGCRUSH_TIMER_UINT_API
pngcrush_timer_get_nanoseconds(unsigned int n);
PNGCRUSH_TIMER_VOID_API
pngcrush_timer_reset(unsigned int n);
PNGCRUSH_TIMER_VOID_API
pngcrush_timer_start(unsigned int n);
PNGCRUSH_TIMER_VOID_API
pngcrush_timer_stop(unsigned int n);

#if PNGCRUSH_TIMERS > 0
PNGCRUSH_TIMER_UINT_API
pngcrush_timer_get_hits(unsigned int n)
{
   if (n < PNGCRUSH_TIMERS)
   {
      return pngcrush_timer_hits[n];
   }
   return 0;
}
PNGCRUSH_TIMER_UINT_API
pngcrush_timer_get_seconds(unsigned int n)
{
   if (n < PNGCRUSH_TIMERS)
   {
      return pngcrush_timer_secs[n];
   }
   return 0;
}
PNGCRUSH_TIMER_UINT_API
pngcrush_timer_get_nanoseconds(unsigned int n)
{
   if (n < PNGCRUSH_TIMERS)
   {
      return pngcrush_timer_nsec[n];
   }
   return 0;
}
PNGCRUSH_TIMER_VOID_API
pngcrush_timer_reset(unsigned int n)
{
   if (n < PNGCRUSH_TIMERS)
   {
      pngcrush_timer_secs[n] = 0;
      pngcrush_timer_nsec[n] = 0;
      pngcrush_timer_hits[n] = 0;
   }
}
PNGCRUSH_TIMER_VOID_API
pngcrush_timer_start(unsigned int n)
{
   if (verbose >= 0 && n < PNGCRUSH_TIMERS)
   {
#if PNGCRUSH_USE_CLOCK_GETTIME
      struct timespec t;
      clock_gettime(PNGCRUSH_CLOCK_ID, &t);
      pngcrush_clock_secs[n] = t.tv_sec;
      pngcrush_clock_nsec[n] = t.tv_nsec;
#else
     TIME_T t = clock();
     pngcrush_clock_secs[n] = t/CLOCKS_PER_SEC;
     pngcrush_clock_nsec[n] = (t*1000000000/CLOCKS_PER_SEC) -
       pngcrush_clock_secs[n]*1000000000;
#endif
   }
}
PNGCRUSH_TIMER_VOID_API
pngcrush_timer_stop(unsigned int n)
{
   if (verbose >= 0 && n < PNGCRUSH_TIMERS)
   {
      unsigned long seconds;
      unsigned long nseconds;
      unsigned long delta_secs;
      unsigned long delta_nsec;
#if PNGCRUSH_USE_CLOCK_GETTIME
      struct timespec t;
      clock_gettime(PNGCRUSH_CLOCK_ID, &t);
      seconds = t.tv_sec;
      nseconds = t.tv_nsec;
#else
     TIME_T t = clock();
     seconds = t/CLOCKS_PER_SEC;
     nseconds = (t*1000000000/CLOCKS_PER_SEC) - seconds*1000000000;
#endif
      delta_secs = (unsigned int)seconds - pngcrush_clock_secs[n];
      if ((unsigned long)nseconds < pngcrush_clock_nsec[n])
      {
         delta_nsec  = 1000000000;
         delta_secs--;
      }
      else
      {
         delta_nsec  = 0;
      }

      delta_nsec  += (unsigned long)nseconds - pngcrush_clock_nsec[n];
      pngcrush_timer_secs[n] += delta_secs;
      pngcrush_timer_nsec[n] += delta_nsec;

      if (pngcrush_timer_nsec[n] >= 1000000000)
      {
         pngcrush_timer_nsec[n] -= 1000000000;
         pngcrush_timer_secs[n]++;
      }

      pngcrush_clock_secs[n] = (unsigned long)seconds;
      pngcrush_clock_nsec[n] = (unsigned long)nseconds;
      pngcrush_timer_hits[n]++;
   }
}

static unsigned long pngcrush_timer_min_secs[PNGCRUSH_TIMERS];
static unsigned long pngcrush_timer_min_nsec[PNGCRUSH_TIMERS];
#endif /* PNGCRUSH_TIMERS */


#ifdef ZLIB_AMALGAMATED
/* See
 https://blog.forrestthewoods.com/
 improving-open-source-with-amalgamation-cf293592c5f4#.g9fb2tyhs
 */
#include "zlib_amalg.c"
#define ZLIB_H
#endif /* ZLIB_AMALGAMATED */

#ifdef ZLIB_UNIFIED  /* Not working */
#include "zutil.h"

#include "adler32.c"
#undef DO1
#undef DO8

#include "compress.c"
#include "crc32.c"
#include "deflate.c"
#include "infback.c"
#undef PULLBYTE

#include "inffast.c"
#undef CHECK
#undef CODES
#undef DISTS
#undef DONE
#undef LENGTH
#undef LENS

#include "inflate.c"
#undef CHECK
#undef CODES
#undef DISTS
#undef DONE
#undef LENGTH
#undef LENS
#undef PULLBYTE

#include "inftrees.c"
#undef CHECK
#undef CODES
#undef DISTS
#undef DONE
#undef LENGTH
#undef LENS

#include "trees.c"
#include "uncompr.c"
#include "zutil.c"
#endif /* ZLIB_UNIFIED */

#ifdef LIBPNG_UNIFIED
#include "pngcrush.h"
#include "png.c"
#include "pngerror.c"
#include "pngget.c"
#include "pngmem.c"
#include "pngpread.c"
#include "pngread.c"
#include "pngrio.c"
#include "pngrtran.c"
#include "pngrutil.c"
#include "pngset.c"
#include "pngtrans.c"
#include "pngwio.c"
#include "pngwrite.c"
#include "pngwtran.c"
#include "pngwutil.c"
#ifdef PNGCRUSH_USE_ARM_NEON
# include "arm_init.c"
# include "filter_neon_intrinsics.c"
#endif
#ifdef PNGCRUSH_USE_MIPS_NSA
# include "mips_init.c"
# include "filter_msa_intrinsics.c"
#endif
#ifdef PNGCRUSH_USE_INTEL_SSE
# include "intel_init.c"
# include "filter_sse2_intrinsics.c"
#endif
#ifdef PNGCRUSH_USE_POWERPC_VSX
# include "powerpc_init.c"
# include "filter_vsx_intrinsics.c"
#endif
#endif /* LIBPNG_UNIFIED */

#include "png.h"

#ifndef PNGCBAPI  /* Needed when building with libpng-1.4.x and earlier */
# define PNGCBAPI PNGAPI
#endif

/* internal libpng macros */

#ifdef PNG_LIBPNG_VER
#define PNGCRUSH_LIBPNG_VER PNG_LIBPNG_VER
#else
/*
 * This must agree with PNG_LIBPNG_VER; you have to define it manually
 * here if you are using libpng-1.0.6h or earlier
 */
#define PNGCRUSH_LIBPNG_VER 10007
#endif

#define PNGCRUSH_UNUSED(param) (void)param;
#define pngcrush_get_uint_31 png_get_uint_31
#define pngcrush_get_uint_32 png_get_uint_32
#define pngcrush_save_uint_32 png_save_uint_32

#undef PNG_ABORT
#if (PNGCRUSH_LIBPNG_VER < 10400)
/* This allows png_default_error() to return, when it is called after our
 * own exception handling, which only returns after "Too many IDAT's",
 * or anything else that we might want to handle as a warning instead of
 * an error.  Doesn't work in libpng-1.4.0 and later; there we use
 * png_benign_error() instead.
 */
#  define PNG_ABORT() (void)0
#elif (PNGCRUSH_LIBPNG_VER < 10700)
#  define PNG_ABORT() abort()
#else
#  define PNG_ABORT abort();
#endif

#if (PNGCRUSH_LIBPNG_VER < 10500)
/* Two macros to return the first row and first column of the original,
 * full, image which appears in a given pass.  'pass' is in the range 0
 * to 6 and the result is in the range 0 to 7.
 */
#define PNG_PASS_START_ROW(pass) (((1&~(pass))<<(3-((pass)>>1)))&7)
#define PNG_PASS_START_COL(pass) (((1& (pass))<<(3-(((pass)+1)>>1)))&7)

/* A macro to return the offset between pixels in the output row for a pair of
 * pixels in the input - effectively the inverse of the 'COL_SHIFT' macro that
 * follows.  Note that ROW_OFFSET is the offset from one row to the next whereas
 * COL_OFFSET is from one column to the next, within a row.
 */
#define PNG_PASS_ROW_OFFSET(pass) ((pass)>2?(8>>(((pass)-1)>>1)):8)
#define PNG_PASS_COL_OFFSET(pass) (1<<((7-(pass))>>1))

/* Two macros to help evaluate the number of rows or columns in each
 * pass.  This is expressed as a shift - effectively log2 of the number or
 * rows or columns in each 8x8 tile of the original image.
 */
#define PNG_PASS_ROW_SHIFT(pass) ((pass)>2?(8-(pass))>>1:3)
#define PNG_PASS_COL_SHIFT(pass) ((pass)>1?(7-(pass))>>1:3)

/* Hence two macros to determine the number of rows or columns in a given
 * pass of an image given its height or width.  In fact these macros may
 * return non-zero even though the sub-image is empty, because the other
 * dimension may be empty for a small image.
 */
#define PNG_PASS_ROWS(height, pass) (((height)+(((1<<PNG_PASS_ROW_SHIFT(pass))\
   -1)-PNG_PASS_START_ROW(pass)))>>PNG_PASS_ROW_SHIFT(pass))
#define PNG_PASS_COLS(width, pass) (((width)+(((1<<PNG_PASS_COL_SHIFT(pass))\
   -1)-PNG_PASS_START_COL(pass)))>>PNG_PASS_COL_SHIFT(pass))
#endif /* PNGCRUSH_LIBPNG_VER < 10500 */

#if PNGCRUSH_LIBPNG_VER >= 10500
#if !defined(ZLIB_AMALGAMATED) && !defined(ZLIB_UNIFIED)
   /* "#include <zlib.h>" is not provided by libpng15 */
#ifdef PNGCRUSH_H
   /* Use the bundled zlib */
#  include "zlib.h"
#else
   /* Use the system zlib */
#  include <zlib.h>
#endif
#endif /* ZLIB_AMALGAMATED || ZLIB_UNIFIED */

   /* Not provided by libpng16 */
#  include <string.h>

   /* The following became unavailable in libpng16 (and were
    * deprecated in libpng14 and 15)
    */
#  ifdef _WINDOWS_  /* Favor Windows over C runtime fns */
#    define png_memcmp  memcmp
#    define png_memcpy  CopyMemory
#    define png_memset  memset
#  else
#    define png_memcmp  memcmp      /* SJT: added */
#    define png_memcpy  memcpy
#    define png_memset  memset
#  endif
#endif /* PNGCRUSH_LIBPNG_VER >= 10500 */

#if PNGCRUSH_LIBPNG_VER < 10800 || defined(PNGCRUSH_H)

/* Changed in version 0.99 */
#if PNGCRUSH_LIBPNG_VER < 99
#  undef PNG_CONST
#  ifndef PNG_NO_CONST
#    define PNG_CONST const
#  else
#    define PNG_CONST
#  endif
#endif

#ifndef LIBPNG_UNIFIED
#define PNG_IDAT const png_byte png_IDAT[5] = { 73,  68,  65,  84, '\0'}
#define PNG_IHDR const png_byte png_IHDR[5] = { 73,  72,  68,  82, '\0'}
#define PNG_acTL const png_byte png_acTL[5] = { 97,  99,  84,  76, '\0'}
#define PNG_dSIG const png_byte png_dSIG[5] = {100,  83,  73,  71, '\0'}
#define PNG_fcTL const png_byte png_fcTL[5] = {102,  99,  84,  76, '\0'}
#define PNG_fdAT const png_byte png_fdAT[5] = {102, 100,  65,  84, '\0'}
#define PNG_iCCP const png_byte png_iCCP[5] = {105,  67,  67,  80, '\0'}
#define PNG_IEND const png_byte png_IEND[5] = { 73,  69,  78,  68, '\0'}
#endif

/* GRR 20050220:  added these, which apparently aren't defined anywhere else */
/* GRP 20110714:  define PNG_UINT_32_NAME macro and used that instead */
#define PNG_UINT_32_NAME(a,b,c,d) \
                    ((png_uint_32) ((a) & 0xff) << 24  | \
                    ((png_uint_32) (b) << 16) | \
                    ((png_uint_32) (c) <<  8) | \
                    ((png_uint_32) (d)      ))
#ifndef PNG_UINT_IHDR
#  define PNG_UINT_IHDR PNG_UINT_32_NAME(73, 72, 68, 82)
#endif

#ifndef PNG_UINT_IDAT
#  define PNG_UINT_IDAT PNG_UINT_32_NAME(73, 68, 65, 84)
#endif

#ifndef PNG_UINT_IEND
#  define PNG_UINT_IEND PNG_UINT_32_NAME(73, 69, 78, 68)
#endif

#ifndef PNG_UINT_PLTE
#  define PNG_UINT_PLTE PNG_UINT_32_NAME(80, 76, 84, 69)
#endif

#ifndef PNG_UINT_bKGD
#  define PNG_UINT_bKGD PNG_UINT_32_NAME(98, 75, 71, 68)
#endif

/* glennrp added CgBI at pngcrush-1.6.16 */
#ifndef PNG_UINT_CgBI
#  define PNG_UINT_CgBI PNG_UINT_32_NAME(67,103, 66, 73)
#endif

/* glennrp added acTL, fcTL, and fdAT at pngcrush-1.7.0 */
#  define PNG_UINT_acTL PNG_UINT_32_NAME(97, 99, 84, 76)
#  define PNG_UINT_fcTL PNG_UINT_32_NAME(102, 99, 84, 76)
#  define PNG_UINT_fdAT PNG_UINT_32_NAME(102,100, 65, 84)

#ifndef PNG_UINT_cHRM
#  define PNG_UINT_cHRM PNG_UINT_32_NAME(99, 72, 82, 77)
#endif

#ifndef PNG_UINT_dSIG
#  define PNG_UINT_dSIG PNG_UINT_32_NAME(100, 83, 73, 71)
#endif

#ifndef PNG_UINT_gAMA
#  define PNG_UINT_gAMA PNG_UINT_32_NAME(103, 65, 77, 65)
#endif

#ifndef PNG_UINT_hIST
#  define PNG_UINT_hIST PNG_UINT_32_NAME(104, 73, 83, 84)
#endif

#ifndef PNG_UINT_iCCP
#  define PNG_UINT_iCCP PNG_UINT_32_NAME(105, 67, 67, 80)
#endif

#ifndef PNG_UINT_iTXt
#  define PNG_UINT_iTXt PNG_UINT_32_NAME(105, 84, 88, 116)
#endif

#ifndef PNG_UINT_oFFs
#  define PNG_UINT_oFFs PNG_UINT_32_NAME(111, 70, 70, 115)
#endif

#ifndef PNG_UINT_pCAL
#  define PNG_UINT_pCAL PNG_UINT_32_NAME(112, 67, 65, 76)
#endif

#ifndef PNG_UINT_pHYs
#  define PNG_UINT_pHYs PNG_UINT_32_NAME(112, 72, 89, 115)
#endif

#ifndef PNG_UINT_sBIT
#  define PNG_UINT_sBIT PNG_UINT_32_NAME(115, 66, 73, 84)
#endif

#ifndef PNG_UINT_sCAL
#  define PNG_UINT_sCAL PNG_UINT_32_NAME(115, 67, 65, 76)
#endif

#ifndef PNG_UINT_sPLT
#  define PNG_UINT_sPLT PNG_UINT_32_NAME(115, 80, 76, 84)
#endif

#ifndef PNG_UINT_sRGB
#  define PNG_UINT_sRGB PNG_UINT_32_NAME(115, 82, 71, 66)
#endif

/* glennrp added sTER at pngcrush-1.6.10 */
#ifndef PNG_UINT_sTER
#  define PNG_UINT_sTER PNG_UINT_32_NAME(115, 84, 69, 82)
#endif

#ifndef PNG_UINT_tEXt
#  define PNG_UINT_tEXt PNG_UINT_32_NAME(116, 69, 88, 116)
#endif

#ifndef PNG_UINT_tIME
#  define PNG_UINT_tIME PNG_UINT_32_NAME(116, 73, 77, 69)
#endif

#ifndef PNG_UINT_tRNS
#  define PNG_UINT_tRNS PNG_UINT_32_NAME(116, 82, 78, 83)
#endif

#ifndef PNG_UINT_zTXt
#  define PNG_UINT_zTXt PNG_UINT_32_NAME(122, 84, 88, 116)
#endif

#ifndef LIBPNG_UNIFIED
#define PNG_FLAG_CRC_ANCILLARY_USE        0x0100
#define PNG_FLAG_CRC_ANCILLARY_NOWARN     0x0200
#define PNG_FLAG_CRC_CRITICAL_USE         0x0400
#define PNG_FLAG_CRC_CRITICAL_IGNORE      0x0800
#define PNG_FLAG_CRC_ANCILLARY_MASK (PNG_FLAG_CRC_ANCILLARY_USE | \
                                     PNG_FLAG_CRC_ANCILLARY_NOWARN)
#define PNG_PACK               0x0004
#define PNG_DITHER             0x0040
#define PNG_BACKGROUND         0x0080
#define PNG_16_TO_8            0x0400
#define PNG_RGBA               0x0800
#define PNG_EXPAND             0x1000
#define PNG_GAMMA              0x2000
#define PNG_GRAY_TO_RGB        0x4000
#define PNG_FILLER             0x8000L
#define PNG_USER_TRANSFORM   0x100000L
#define PNG_RGB_TO_GRAY      0x600000L  /* two bits, RGB_TO_GRAY_ERR|WARN */
#endif /* LIBPNG_UNIFIED */

/*
 * We don't need some of the extra libpng transformations
 * so they are ifdef'ed out in pngcrush.h, which is included by
 * pngcrush's local copy of libpng's pngconf.h which is included
 * by png.h
 *
 */

/* Defined so I can write to a file on gui/windowing platforms */
#if 0 /* Change this to "#if 1" if you need to. */
#  define STDERR stdout /* for DOS */
#else
#  define STDERR stderr
#endif

#ifdef PNG_MNG_FEATURES_SUPPORTED
# define PNGCRUSH_LOCO
#endif

#ifdef PNGCRUSH_H
int png_ignore_crc = 0;
#else
png_uint_32 pngcrush_crc;
#endif

#ifndef PNG_UINT_31_MAX
#define PNG_UINT_31_MAX ((png_uint_32)0x7fffffffL)
#endif

/* These macros were renamed in libpng-1.2.6 */
#ifndef PNG_HANDLE_CHUNK_ALWAYS
#define PNG_HANDLE_CHUNK_ALWAYS  HANDLE_CHUNK_ALWAYS
#define PNG_HANDLE_CHUNK_NEVER   HANDLE_CHUNK_NEVER
#define PNG_HANDLE_CHUNK_IF_SAFE HANDLE_CHUNK_IF_SAFE
#endif

#if defined(__DJGPP__) && ((__DJGPP__ == 2) && (__DJGPP_MINOR__ == 0))
#  include <libc/dosio.h>     /* for _USE_LFN, djgpp 2.0 only */
#endif

#if ( defined(_Windows) || defined(_WINDOWS) || defined(WIN32) ||  \
   defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__) || \
   defined(__DJGPP__) )
#  define SLASH "\\"
#  define DOT "."
#else
#  ifdef __riscos
#    define SLASH "."
#    define DOT "/"
#  else
#    define SLASH "/"
#    define DOT "."
#  endif
#endif

#define BACK_SLASH "\\"
#define FWD_SLASH "/"

#ifndef GAS_VERSION
#  define GAS_VERSION "2.9.5(?)"  /* used only in help/usage screen */
#endif

#if !defined(__TURBOC__) && !defined(_MSC_VER) && !defined(_MBCS) && \
    !defined(__riscos)
#  include <unistd.h>
#endif

#ifndef __riscos
#  include <sys/types.h>
#  include <sys/stat.h>
#  ifndef _MSC_VER
#    include <utime.h>
#  else
#    include <sys/utime.h>
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#if defined(_MBCS) || defined(WIN32) || defined(__WIN32__)
#  include <direct.h>
#endif

#define DEFAULT_MODE     0
#define DIRECTORY_MODE   1
#define EXTENSION_MODE   2
#define DIREX_MODE       3
#define OVERWRITE_MODE   4
#define FOPEN(file, how) fopen(file, how)
#define FCLOSE(file)     {fclose(file); file=NULL;--number_of_open_files;};

#define P0 if(last_trial && verbose > 0)printf
#define P1 if(verbose > 1)printf
#define P2 if(verbose > 2)printf

#define STRNGIFY_STAGE1(x) #x
#define STRNGIFY(x) STRNGIFY_STAGE1(x)

#define STR_BUF_SIZE      2048
#define MAX_IDAT_SIZE     524288L
#define MAX_METHODS       177
#define MAX_METHODSP1     (MAX_METHODS+1)
#define DEFAULT_METHODS   10
#define FAKE_PAUSE_STRING "P"

#ifdef Z_RLE
#  define NUM_STRATEGIES  4
#else
#  define NUM_STRATEGIES  3
#endif

#ifdef __TURBOC__
#  include <mem.h>
#endif

struct options_help
{
    int verbosity;          /* if verbose >= this value, then print line */
    const char *textline;   /* static string with newline chopped off */
};

#ifdef MINGW32
   /* enable commandline wildcard interpretation (doesn't necessarily work!) */
   int _dowildcard = 1;
#endif

/* Input and output filenames */
static PNG_CONST char *progname;
static PNG_CONST char *inname = "pngtest" DOT "png";
static PNG_CONST char *outname = "pngout" DOT "png";
#ifdef PNGCRUSH_LOCO
static PNG_CONST char *mngname = "mngout" DOT "mng";
#endif
static PNG_CONST char *directory_name = "pngcrush" DOT "bak";
static PNG_CONST char *extension = "_C" DOT "png";

static png_uint_32 width, height;
static png_uint_32 measured_idat_length;
static int found_bKGD = 0;
static int found_color_bKGD = 0;
#ifdef PNG_cHRM_SUPPORTED
static int found_cHRM = 0;
#endif
static int found_gAMA = 0;
static int found_hIST = 0;
static int found_iCCP = 0;
static int found_IDAT = 0;
static int found_sBIT = 0;
static int found_sBIT_max = 0;
static int found_sBIT_different_RGB_bits = 0;
static int found_sRGB = 0;
static int found_tRNS = 0;

static int premultiply = 0;
static int printed_version_info = 0;
static int interlace_method = 0;
#if (PNGCRUSH_LIBPNG_VER < 10400)
png_size_t max_bytes;
#else
png_alloc_size_t max_bytes;
#endif

       /* 0: not premultipled
        * 1: premultiplied input (input has .ppng suffix)
        * 2: premultiplied output (output has .ppng suffix)
        * 3: premultiplied input and output (both have .ppng suffix)
        *
        *    .png -> .ppng is OK, do premultiplication.
        *    .ppng -> .ppng is OK, simply copy data.
        *    .ppng -> .ppng is not OK because colors are irretrievably lost.
        *    .ppng -> no output (pngcrush -n) is OK.
        *
        * To do: Implement this stuff!
        */

static int found_CgBI = 0;
static int found_any_chunk = 0;
static int save_apng_chunks = 0; /* 0: output not .apng 1: .apng 2: rejected */
static int found_acTL_chunk = 0; /* 0: not found, 1: found, 2: rejected */
static int image_is_immutable = 0;
static int pngcrush_must_exit = 0;
static int all_chunks_are_safe = 0;
static int number_of_open_files;
static int do_pplt = 0;
#ifdef PNGCRUSH_MULTIPLE_ROWS
static png_uint_32 max_rows_at_a_time = 1;
static png_uint_32 rows_at_a_time;
#endif
char pplt_string[STR_BUF_SIZE];
char *ip, *op, *dot;
char in_string[STR_BUF_SIZE];
char prog_string[STR_BUF_SIZE];
char out_string[STR_BUF_SIZE];
char in_extension[STR_BUF_SIZE];
static int text_inputs = 0;
int text_where[10];           /* 0: no text; 1: before PLTE; 2: after PLTE */
int text_compression[10];     /* -1: uncompressed tEXt; 0: compressed zTXt
                                  1: uncompressed iTXt; 2: compressed iTXt */
char text_text[11*STR_BUF_SIZE+1]; /* It would be nice to png_malloc this but we
                                    don't have a png_ptr yet when we need it. */
char text_keyword[11*80+1];

/* PNG_iTXt_SUPPORTED */
char text_lang[881];
char text_lang_key[881];

/* PNG_iCCP_SUPPORTED */
int iccp_length = 0;
char *iccp_text;
char *iccp_file;
char iccp_name[80];

int best;

char buffer[256];

/* Set up the "cexcept" Try/Throw/Catch exception handler. */
#include "cexcept.h"
define_exception_type(const char *);
extern struct exception_context the_exception_context[1];
struct exception_context the_exception_context[1];
png_const_charp msg;

static png_uint_32 input_length;
static png_uint_32 total_input_length = 0;
static png_uint_32 total_output_length = 0;
static int pngcrush_mode = DEFAULT_MODE;
static int resolution = 0;
static int remove_chunks = 0;
static int output_color_type;
static int output_bit_depth;
static int force_output_color_type = 8;
static int force_output_bit_depth = 0;
static int input_color_type;
static int input_bit_depth;
static int trial;
static int last_trial = 0;
static png_uint_32 pngcrush_write_byte_count;
static png_uint_32 pngcrush_best_byte_count=0xffffffff;

static int salvage = 0;
static int bail = 0;  /* if 0, bail out of trials early */
static int check_crc = 0;  /* if 0, skip CRC and ADLER32 checks */
                           /* otherwise check both */
static int force = 1; /* if 1, force output even if IDAT is larger */
static unsigned int benchmark_iterations = 0;

static int blacken = 0; /* if 0, or 2 after the first trial,
                           do not blacken color samples */

/* Delete these in pngcrush-1.8.0 */
#if 0
static int make_gray = 0; /* if 0, 2, or 3 after the first trial,
                           do not change color_type to gray */
static int make_opaque = 0; /* if 0, 2, or 3 after the first trial,
                           do not change color_type to opaque */
static int make_8_bit = 0; /* if 0, 2, or 3 after the first trial,
                           do not reduce bit_depth from 16 */
static int reduce_palette = 0;
#endif

/* Activate these in pngcrush-1.8.0 */
#if 1
static int noreduce = 1; /* if 0, "-reduce" was specified */
static int make_gray = 1; /* if 0, 2, or 3 after the first trial,
                           do not change color_type to gray */
static int make_opaque = 1; /* if 0, 2, or 3 after the first trial,
                           do not change color_type to opaque */
static int make_8_bit = 1; /* if 0, 2, or 3 after the first trial,
                           do not reduce bit_depth from 16 */
static int reduce_palette = 1;
#endif

static int compression_window;
static int default_compression_window = 15;
static int force_compression_window = 0;
static int compression_mem_level = 9;
static int final_method = 0;
static int brute_force = 0;
static int brute_force_level = 0;
static int brute_force_filter = 0;
static int brute_force_strategy = 0;
static int brute_force_levels[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
static int brute_force_filters[6] = { 1, 1, 1, 1, 1, 1 };
#ifdef Z_RLE
static int brute_force_strategies[NUM_STRATEGIES] = { 1, 1, 1, 1 };
#else
static int brute_force_strategies[NUM_STRATEGIES] = { 1, 1, 1 };
#endif
static int speed = 0;
static int method = 10;
static int pauses = 0;
static int nosave = 0;    /* 0: save; 1: do not save */
static int overwrite = 0; /* 1: overwrite the input file instead of
                                creating a new output file */
static int nofilecheck = 0;
static int no_limits = 0;
static int new_mng = 0;
static png_bytep row_buf;
#ifdef PNGCRUSH_MULTIPLE_ROWS
static png_bytepp row_pointers;
#endif
static int z_strategy;
static int best_of_three;
static int methods_specified = 0;
static int specified_intent = -1;
static int intent = -1;
static int ster_mode = -1;
static int new_time_stamp = 1;
static int plte_len = -1;
#ifdef PNG_FIXED_POINT_SUPPORTED
static int specified_gamma = 0;
static int image_specified_gamma = 0;
static int force_specified_gamma = 0;
#else
static double specified_gamma = 0.0;
static double image_specified_gamma = 0;
static double force_specified_gamma = 0.0;
#endif
static int double_gamma = 0;

static int names;
static int first_name;

static int have_trns = 0;
static png_uint_16 trns_index = 0;
static png_uint_16 trns_red = 0;
static png_uint_16 trns_green = 0;
static png_uint_16 trns_blue = 0;
static png_uint_16 trns_gray = 0;

static png_byte trns_array[256];
static png_byte trans_in[256];
static png_uint_16 num_trans_in;

#if defined(PNG_READ_tRNS_SUPPORTED) && defined(PNG_WRITE_tRNS_SUPPORTED)
       png_bytep trans;
       int num_trans;
       png_color_16p trans_values;
#endif

static int have_bkgd = 0;
static png_uint_16 bkgd_red = 0;
static png_uint_16 bkgd_green = 0;
static png_uint_16 bkgd_blue = 0;
static png_byte bkgd_index = 0;

static png_colorp palette;
static int num_palette;

#ifdef REORDER_PALETTE
static png_byte palette_reorder[256];
#endif

static png_structp read_ptr, write_ptr, mng_ptr;
static png_infop read_info_ptr, write_info_ptr;
static png_infop end_info_ptr;
static png_infop write_end_info_ptr;
static FILE *fpin, *fpout;
png_uint_32 measure_idats(FILE * fp);
#ifdef PNGCRUSH_LOCO
static FILE *mng_out;
static int do_loco = 0;
static int input_format = 0;    /* 0: PNG  1: MNG */
static int output_format = 0;
#endif
static int do_color_count;
png_uint_32 pngcrush_measure_idat(png_structp png_ptr);

static png_uint_32 idat_length[MAX_METHODSP1];
static int filter_type, zlib_level;
static png_bytep png_row_filters = NULL;

#if PNGCRUSH_TIMERS > 0
unsigned int pc_timer;
static float t_filter[PNGCRUSH_TIMERS] = {0};
static png_uint_32 filter_count[PNGCRUSH_TIMERS] = {0};
png_uint_32 t_sec;
png_uint_32 t_nsec;
#endif

static png_uint_32 max_idat_size = MAX_IDAT_SIZE; /* increases the IDAT size */

#if 0 /* disabled */
static png_uint_32 crushed_idat_size = 0x3ffffffL;
static int already_crushed = 0;
#endif

int ia;

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
static /* const */ png_byte chunks_to_ignore[] = {

     98,  75,  71 , 68, '\0',  /* bKGD */
     99,  72,  82,  77, '\0',  /* cHRM */
    103,  65,  77,  65, '\0',  /* gAMA */
    104,  73,  83,  84, '\0',  /* hIST */
    105,  67,  67,  80, '\0',  /* iCCP */
    105,  84,  88, 116, '\0',  /* iTXt */
    111,  70,  70, 115, '\0',  /* oFFs */
    112,  67,  65,  76, '\0',  /* pCAL */
    112,  72,  89, 115, '\0',  /* pHYs */
    115,  66,  73,  84, '\0',  /* sBIT */
    115,  67,  65,  76, '\0',  /* sCAL */
    115,  80,  76,  84, '\0',  /* sPLT */
    115,  82,  71,  66, '\0',  /* sRGB */
    115,  84,  69,  82, '\0',  /* sTER */
    116,  69,  88, 116, '\0',  /* tEXt */
    116,  73,  77,  69, '\0',  /* tIME */
    116,  82,  78,  83, '\0',  /* tRNS */
    122,  84,  88, 116, '\0'   /* zTXt */
};
#endif

/* Prototypes */
static void pngcrush_cexcept_error(png_structp png_ptr,
  png_const_charp message);

static void pngcrush_warning(png_structp png_ptr,
  png_const_charp message);

void PNGCBAPI pngcrush_default_read_data(png_structp png_ptr, png_bytep data,
  png_size_t length);

#ifdef PNGCRUSH_H
void png_read_transform_info(png_structp png_ptr, png_infop info_ptr);
#endif

void PNGCBAPI pngcrush_default_write_data(png_structp png_ptr, png_bytep data,
  png_size_t length);

void pngcrush_write_png(png_structp write_pointer, png_bytep data,
     png_size_t length);

#ifdef PNG_USER_MEM_SUPPORTED
png_voidp pngcrush_debug_malloc(png_structp png_ptr, png_uint_32 size);
void pngcrush_debug_free(png_structp png_ptr, png_voidp ptr);
#endif

void pngcrush_pause(void);

#ifdef __riscos
static int fileexists(const char *name)
static int filesize(const char *name)
static int mkdir(const char *name, int ignored)
static void setfiletype(const char *name)
#endif

int keep_unknown_chunk(png_const_charp name, char *argv[]);
int keep_chunk(png_const_charp name, char *argv[]);
void show_result(void);
png_uint_32 measure_idats(FILE * fp);
png_uint_32 pngcrush_measure_idat(png_structp png_ptr);

void print_version_info(void);
void print_usage(int retval);

#ifdef PNGCRUSH_H
/* Use unexported functions in the embedded libpng */
# define pngcrush_reset_crc(png_ptr) png_reset_crc(png_ptr)
# define pngcrush_calculate_crc(png_ptr, ptr, length) \
  png_calculate_crc(png_ptr, ptr, length)
# define pngcrush_crc_read(png_ptr, buf, length) \
  png_crc_read(png_ptr, buf, length)
# define pngcrush_crc_error(png_ptr) png_crc_error(png_ptr)
# define pngcrush_crc_finish(png_ptr, skip) png_crc_finish(png_ptr, skip)

#else
/*
 * ============================================================
 * We aren't using the bundled libpng functions, so we must
 * reproduce the libpng routines that aren't exported by libpng
 * ============================================================
 */

# if (PNGCRUSH_LIBPNG_VER >= 10209)
#   ifndef PNG_READ_BIG_ENDIAN_SUPPORTED
#     undef pngcrush_get_uint_32
png_uint_32 pngcrush_get_uint_32(png_bytep buf);
/* Grab an unsigned 32-bit integer from a buffer in big-endian format. */
png_uint_32 /* PRIVATE */
pngcrush_get_uint_32(png_bytep buf)
{
   png_uint_32 i = ((png_uint_32) (*buf & 0xff) << 24) +
       (*(buf + 1) << 16) + (*(buf + 2) <<  8) + (*(buf + 3)) ;

   return (i);
}
#   else /*! BIG_ENDIAN */
#      define pngcrush_get_uint_32(buf) ( *((png_uint_32p) (buf)))
#   endif /* BIG_ENDIAN */

#   undef pngcrush_get_uint_31
png_uint_32 pngcrush_get_uint_31(png_structp png_ptr, png_bytep buf);
png_uint_32 /* PRIVATE */
pngcrush_get_uint_31(png_structp png_ptr, png_bytep buf)
{
   png_uint_32 i = pngcrush_get_uint_32(buf);
   if (i > PNG_UINT_31_MAX)
   {
     i=0;
     png_error(png_ptr, "PNG unsigned integer out of range.\n");
   }
   return (i);
}

#   undef pngcrush_save_uint_32
void pngcrush_save_uint_32(png_bytep buf, png_uint_32 i);
void /* PRIVATE */
pngcrush_save_uint_32(png_bytep buf, png_uint_32 i)
{
   buf[0] = (png_byte)((i >> 24) & 0xff);
   buf[1] = (png_byte)((i >> 16) & 0xff);
   buf[2] = (png_byte)((i >> 8) & 0xff);
   buf[3] = (png_byte)(i & 0xff);
}
# endif  /* PNGCRUSH_LIBPNG_VER < 10209 */

/*
 * Reset the CRC variable to 32 bits of 1's.  Care must be taken
 * in case CRC is > 32 bits to leave the top bits 0.
 */
void PNGAPI
pngcrush_reset_crc(png_structp png_ptr)
{
   pngcrush_crc = crc32(0, Z_NULL, 0);
}
/*
 * Calculate the CRC over a section of data.  We can only pass as
 * much data to this routine as the largest single buffer size.  We
 * also check that this data will actually be used before going to the
 * trouble of calculating it.
 */
void PNGAPI
pngcrush_calculate_crc(png_structp png_ptr, png_bytep ptr, png_size_t length)
{
   pngcrush_crc = crc32(pngcrush_crc, ptr, (uInt)length);
}

/* Read data, and (optionally) run it through the CRC. */
void PNGAPI
pngcrush_crc_read(png_structp png_ptr, png_bytep buf, png_size_t length)
{
   pngcrush_default_read_data(png_ptr, buf, length);
   pngcrush_calculate_crc(png_ptr, buf, length);
}

/* Compare the CRC stored in the PNG file with that calculated by libpng from
 * the data it has read thus far.
 */
int PNGAPI
pngcrush_crc_error(png_structp png_ptr)
{
   png_byte crc_bytes[4];
   png_uint_32 crc;

   pngcrush_default_read_data(png_ptr, crc_bytes, 4);

   crc = pngcrush_get_uint_32(crc_bytes);
   return ((int)(crc != pngcrush_crc));
}

/*
 * Optionally skip data and then check the CRC.  Depending on whether we
 * are reading a ancillary or critical chunk, and how the program has set
 * things up, we may calculate the CRC on the data and print a message.
 * Returns '1' if there was a CRC error, '0' otherwise.
 */
int PNGAPI
pngcrush_crc_finish(png_structp png_ptr, png_uint_32 skip)
{
   png_size_t i;
   png_byte bytes[1024];
   png_size_t istop = 1024;

   for (i = (png_size_t)skip; i > istop; i -= istop)
   {
      pngcrush_crc_read(png_ptr, bytes, (png_size_t)1024);
   }
   if (i)
   {
      pngcrush_crc_read(png_ptr, bytes, i);
   }

   if (pngcrush_crc_error(png_ptr))
   {
      {
         png_chunk_error(png_ptr, "CRC error");
      }
      return (1);
   }

   return (0);
}
#endif /* PNGCRUSH_H */

#ifdef PNG_STDIO_SUPPORTED
/*
 * This is the function that does the actual reading of data.  If you are
 * not reading from a standard C stream, you should create a replacement
 * read_data function and use it at run time with png_set_read_fn(), rather
 * than changing the library.
 */
void PNGCBAPI pngcrush_default_read_data(png_structp png_ptr, png_bytep data,
   png_size_t length)
{
   png_FILE_p io_ptr;

   if (length == 0)
      png_error(png_ptr, "Read Error: invalid length requested");

   io_ptr = png_get_io_ptr(png_ptr);

   if (fileno(io_ptr) == -1)
      png_error(png_ptr, "Read Error: invalid io_ptr");

   /*
    * fread() returns 0 on error, so it is OK to store this in a png_size_t
    * instead of an int, which is what fread() actually returns.
    */
   if ((png_size_t)fread((void *)data, sizeof (png_byte), length,
        io_ptr) != length)
   {
      png_error(png_ptr, "Read Error: invalid length returned");
#if PNGCRUSH_LIBPNG_VER >= 10700
      PNG_ABORT
#else
      PNG_ABORT();
#endif
   }

   clearerr(io_ptr);

   if (ferror(io_ptr))
   {
      clearerr(io_ptr);
      png_error(png_ptr, "Read Error: error returned by fread()");
   }

   if (feof(io_ptr))
   {
      clearerr(io_ptr);
      png_error(png_ptr, "Read Error: unexpected end of file");
   }

   clearerr(io_ptr);
}
#endif /* PNG_STDIO_SUPPORTED */

#ifdef PNG_STDIO_SUPPORTED
/*
 * This is the function that does the actual writing of data.  If you are
 * not writing to a standard C stream, you should create a replacement
 * write_data function and use it at run time with png_set_write_fn(), rather
 * than changing the library.
 */
void PNGCBAPI pngcrush_default_write_data(png_structp png_ptr, png_bytep data,
   png_size_t length)
{
   png_uint_32 check;
   png_FILE_p io_ptr;

   io_ptr = png_get_io_ptr(png_ptr);

   check = fwrite(data, 1, length, io_ptr);
   if (check != length)
      png_error(png_ptr, "Write Error");
}
#endif /* PNG_STDIO_SUPPORTED */

static void pngcrush_warning(png_structp png_ptr,
   png_const_charp warning_msg)
{
#if (PNGCRUSH_LIBPNG_VER >= 10700)
   /* don't warn about damaged LZ stream due to bailing */
   if (bail == 0 && !strcmp(warning_msg, "damaged LZ stream"))
      return;
#endif

   if (verbose >= 0)
      fprintf(stderr, "pngcrush: %s\n", warning_msg);
   else
   {
      if (show_warnings)
         fprintf(stderr, "%s: %s\n", inname, warning_msg);
   }
   return;
}

/* cexcept interface */

static void pngcrush_cexcept_error(png_structp png_ptr,
   png_const_charp err_msg)
{

#if 0
/* Handle "bad adaptive filter value" error.  */
   if (!strcmp(err_msg, "bad adaptive filter value")) {
#ifdef PNG_CONSOLE_IO_SUPPORTED
        fprintf(stderr, "\nIn %s, correcting %s\n", inname,err_msg);
#else
        pngcrush_warning(png_ptr, err_msg);
#endif
     return;
    }

#if PNGCRUSH_LIBPNG_VER < 10400
/* Handle "Too many IDAT's found" error.  In libpng-1.4.x this became
 * a benign error, "Too many IDATs found".  This scheme will not work
 * in libpng-1.5.0 and later.
 */
#  ifdef PNGCRUSH_H  /* Why would this not work with the system library? */
    if (!strcmp(err_msg, "Too many IDAT's found")) {
#    ifdef PNG_CONSOLE_IO_SUPPORTED
        fprintf(stderr, "\nIn %s, correcting %s\n", inname,err_msg);
#    else
        pngcrush_warning(png_ptr, err_msg);
#    endif
     return;
    }
#  endif /* PNGCRUSH_H */
#endif /* PNGCRUSH_LIBPNG_VER */
#endif /* 0 */

    {
        Throw err_msg;
    }
}


/* START of code to validate memory allocation and deallocation */
#ifdef PNG_USER_MEM_SUPPORTED

/*
 * Allocate memory.  For reasonable files, size should never exceed
 * 64K.  However, zlib may allocate more then 64K if you don't tell
 * it not to.  See zconf.h and png.h for more information.  zlib does
 * need to allocate exactly 64K, so whatever you call here must
 * have the ability to do that.
 *
 * This piece of code can be compiled to validate max 64K allocations
 * by setting MAXSEG_64K in zlib zconf.h *or* PNG_MAX_MALLOC_64K.
 */
typedef struct memory_information {
    png_uint_32 size;
    png_voidp pointer;
    struct memory_information *next;
} memory_information;
typedef memory_information *memory_infop;

static memory_infop pinformation = NULL;
static int current_allocation = 0;
static int maximum_allocation = 0;


png_voidp pngcrush_debug_malloc(png_structp png_ptr, png_uint_32 size)
{

    /*
     * png_malloc has already tested for NULL; png_create_struct calls
     * pngcrush_debug_malloc directly (with png_ptr == NULL prior to
     * libpng-1.2.0 which is OK since we are not using a user mem_ptr)
     */

    if (size == 0)
        return (png_voidp) (NULL);

    /*
     * This calls the library allocator twice, once to get the requested
     * buffer and once to get a new free list entry.
     */
    {
        memory_infop pinfo = (memory_infop)malloc(sizeof *pinfo);
        if (pinfo == NULL)
           return (png_voidp) (NULL);
        pinfo->size = size;
        current_allocation += size;
        if (current_allocation > maximum_allocation)
            maximum_allocation = current_allocation;
        pinfo->pointer = malloc(size);
        pinfo->next = pinformation;
        pinformation = pinfo;
        /* Make sure the caller isn't assuming zeroed memory. */
        png_memset(pinfo->pointer, 0xdd, pinfo->size);
        if (verbose > 2)
            fprintf(STDERR, "Pointer %p allocated %lu bytes\n",
                    (png_voidp) pinfo->pointer, (unsigned long)size);
        return (png_voidp) (pinfo->pointer);
    }
}

/* Free a pointer.  It is removed from the list at the same time. */
void pngcrush_debug_free(png_structp png_ptr, png_voidp ptr)
{
    if (png_ptr == NULL)
        fprintf(STDERR, "NULL pointer to pngcrush_debug_free.\n");
    if (ptr == 0) {
#if 0 /* This happens all the time. */
        fprintf(STDERR, "WARNING: freeing NULL pointer\n");
#endif /* 0 */
        return;
    }

    /* Unlink the element from the list. */
    {
        memory_infop *ppinfo = &pinformation;
        for (;;) {
            memory_infop pinfo = *ppinfo;
            if (pinfo->pointer == ptr) {
                *ppinfo = pinfo->next;
                current_allocation -= pinfo->size;
                if (current_allocation < 0)
                    fprintf(STDERR, "Duplicate free of memory\n");
                /* We must free the list element too, but first kill
                   the memory that is to be freed. */
                memset(ptr, 0x55, pinfo->size);
                if (verbose > 2)
                    fprintf(STDERR, "Pointer %p freed %lu bytes\n",
                        (png_voidp) ptr, (unsigned long)pinfo->size);
                free(pinfo);
                break;
            }
            if (pinfo->next == NULL) {
                fprintf(STDERR, "Pointer %p not found\n",
                    (png_voidp) ptr);
                break;
            }
            ppinfo = &pinfo->next;
        }
    }

    /* Finally free the data. */
    free(ptr);
}

#endif /* PNG_USER_MEM_SUPPORTED */
/* END of code to test memory allocation/deallocation */




void pngcrush_pause(void)
{
    if (pauses > 0) {
        char keystroke;
        fprintf(STDERR, "Press [ENTER] key to continue.\n");
        keystroke = (char) getc(stdin);
        keystroke = keystroke;  /* stifle compiler warning */
    }
}


void png_skip_chunk(png_structp png_ptr)
{
  png_byte buff[4] = { 0, 0, 0, 0 };
  int ib;
  unsigned long length;

  /* read the length field */
  pngcrush_default_read_data(png_ptr, buff, 4);
  length = pngcrush_get_uint_31(png_ptr,buff);
  /* read the chunk name */
  pngcrush_default_read_data(png_ptr, buff, 4);
  if (verbose > 0)
    printf("Skipping %c%c%c%c chunk.\n",buff[0],buff[1],
      buff[2],buff[3]);
  /* skip the data and CRC */
  for (ib=0; ib<length+4; ib++)
  {
     png_byte junk[1] = { 0 };
     pngcrush_default_read_data(png_ptr, junk, 1);
  }
}

#ifndef __riscos
#  define setfiletype(x)

#else /* defined(__riscos) */
#  include <kernel.h>

/* The riscos/acorn support was contributed by Darren Salt. */
static int fileexists(const char *name)
{
# ifdef __acorn
    int ret;
    return _swix(8, 3 | 1 << 31, 17, name, &ret) ? 0 : ret;
# else
    _kernel_swi_regs r;
    r.r[0] = 17;
    r.r[1] = (int) name;
    return _kernel_swi(8, &r, &r) ? 0 : r.r[0];
# endif
}


static int filesize(const char *name)
{
# ifdef __acorn
    int ret;
    return _swix(8, 3 | 1 << 27, 17, name, &ret) ? 0 : ret;
# else
    _kernel_swi_regs r;
    r.r[0] = 17;
    r.r[1] = (int) name;
    return _kernel_swi(8, &r, &r) ? 0 : r.r[4];
# endif
}


static int mkdir(const char *name, int ignored)
{
# ifdef __acorn
    _swi(8, 0x13, 8, name, 0);
    return 0;
# else
    _kernel_swi_regs r;
    r.r[0] = 8;
    r.r[1] = (int) name;
    r.r[4] = r.r[3] = r.r[2] = 0;
    return (int) _kernel_swi(8 | 1 << 31, &r, &r);
# endif
}


static void setfiletype(const char *name)
{
# ifdef __acorn
    _swi(8, 7, 18, name, 0xB60);
# else
    _kernel_swi_regs r;
    r.r[0] = 18;
    r.r[1] = (int) name;
    r.r[2] = 0xB60;
    _kernel_swi(8 | 1 << 31, &r, &r);
# endif
}

#endif /* defined(__riscos) */




/*
 * GRR:  basically boolean; first arg is chunk name-string (e.g., "tIME" or
 *       "alla"); second is always full argv[] command line
 *     - remove_chunks is argv index of *last* -rem arg on command line
 *       (would be more efficient to build table at time of cmdline processing!)
 *       (i.e., build removal_list with names or unique IDs or whatever--skip
 *        excessive string-processing on every single one)
 *     - reprocesses command line _every_ time called, looking for -rem opts...
 *     - just like keep_chunk() except that latter sets things_have_changed
 *       variable and debug stmts say "Removed chunk" (but caller actually does
 *       so, by choosing not to copy chunk to new file)
 *     - for any given chunk name, "name" must either match exact command-line
 *       arg (e.g., -rem fOOb), OR it must match one of the official PNG chunk
 *       names explicitly listed below AND command-line arg either used all-
 *       lowercase form or one of "all[ab]" options
 */
int keep_unknown_chunk(png_const_charp name, char *argv[])
{
    int i;
    if (remove_chunks == 0)
        return 1;   /* no -rem options, so always keeping */
    for (i = 1; i <= remove_chunks; i++) {
        if (!strncmp(argv[i], "-rem", 4)) {
            int allb = 0;
            i++;
            if (!strncmp(argv[i], "all", 3)) {
                allb++;  /* all but gamma, but not doing gamma here */
            }
            if (!strncmp(argv[i], name, 4) /* exact chunk-name match in args */
                /* ...or exact match for one of known set, plus args included
                 * either "alla", "allb", or all-lowercase form of "name" */
                || (!strncmp(name, "cHRM", 4)
                    && (!strncmp(argv[i], "chrm", 4) || allb))
                || (!strncmp(name, "dSIG", 4)
                    && (!strncmp(argv[i], "dsig", 4) || allb))
                || (!strncmp(name, "gIFg", 4)
                    && (!strncmp(argv[i], "gifg", 4) || allb))
                || (!strncmp(name, "gIFt", 4)
                    && (!strncmp(argv[i], "gift", 4) || allb))
                || (!strncmp(name, "gIFx", 4)
                    && (!strncmp(argv[i], "gifx", 4) || allb))
                || (!strncmp(name, "hIST", 4)
                    && (!strncmp(argv[i], "hist", 4) || allb))
                || (!strncmp(name, "iCCP", 4)
                    && (!strncmp(argv[i], "iccp", 4) || allb))
                || (!strncmp(name, "pCAL", 4)
                    && (!strncmp(argv[i], "pcal", 4) || allb))
                || (!strncmp(name, "sCAL", 4)
                    && (!strncmp(argv[i], "scal", 4) || allb))
                || (!strncmp(name, "sPLT", 4)
                    && (!strncmp(argv[i], "splt", 4) || allb))
                || (!strncmp(name, "tIME", 4)
                    && (!strncmp(argv[i], "time", 4) || allb)))
            {
                return 0;
            }
        }
    }
    return 1;
}




int keep_chunk(png_const_charp name, char *argv[])
{
    int i;
    if (verbose > 2 && last_trial)
        fprintf(STDERR, "   Read the %s chunk.\n", name);
    if (remove_chunks == 0)
        return 1;
    if (verbose > 1 && last_trial)
        fprintf(STDERR, "     Check for removal of the %s chunk.\n", name);
    for (i = 1; i <= remove_chunks; i++) {
        if (!strncmp(argv[i], "-rem", 4)) {
            int alla = 0;
            int allb = 0;
            int allt = 0;
            i++;
            if (!strncmp(argv[i], "all", 3)) {
                allt++;         /* all forms of text chunk are ancillary */
                allb++;         /* all ancillaries but gamma... */
                if (!strncmp(argv[i], "alla", 4))
                    alla++;     /* ...no, all ancillaries, period */
            } else if (!strncmp(argv[i], "text", 4)) {
                allt++;         /* all forms of text chunk */
            }
            if (!strncmp(argv[i], name, 4)  /* exact chunk-name match in args
                * ...or exact match for one of known set, plus args included
                * either "alla", "allb", or all-lowercase form of "name": */
                || (!strncmp(name, "PLTE", 4)
                    && (!strncmp(argv[i], "plte", 4)        ))
                || (!strncmp(name, "bKGD", 4)
                    && (!strncmp(argv[i], "bkgd", 4) || allb))
                || (!strncmp(name, "cHRM", 4)
                    && (!strncmp(argv[i], "chrm", 4) || allb))
                || (!strncmp(name, "dSIG", 4)
                    && (!strncmp(argv[i], "dsig", 4) || allb))
                || (!strncmp(name, "gAMA", 4)
                    && (!strncmp(argv[i], "gama", 4) || alla))
                || (!strncmp(name, "gIFg", 4)
                    && (!strncmp(argv[i], "gifg", 4) || allb))
                || (!strncmp(name, "gIFt", 4)
                    && (!strncmp(argv[i], "gift", 4) || allb))
                || (!strncmp(name, "gIFx", 4)
                    && (!strncmp(argv[i], "gifx", 4) || allb))
                || (!strncmp(name, "hIST", 4)
                    && (!strncmp(argv[i], "hist", 4) || allb))
                || (!strncmp(name, "iCCP", 4)
                    && (!strncmp(argv[i], "iccp", 4) || allb))
                || (!strncmp(name, "iTXt", 4)
                    && (!strncmp(argv[i], "itxt", 4) || allt))
                || (!strncmp(name, "oFFs", 4)
                    && (!strncmp(argv[i], "offs", 4) || allb))
                || (!strncmp(name, "pHYs", 4)
                    && (!strncmp(argv[i], "phys", 4) || allb))
                || (!strncmp(name, "pCAL", 4)
                    && (!strncmp(argv[i], "pcal", 4) || allb))
                || (!strncmp(name, "sBIT", 4)
                    && (!strncmp(argv[i], "sbit", 4) || allb))
                || (!strncmp(name, "sCAL", 4)
                    && (!strncmp(argv[i], "scal", 4) || allb))
                || (!strncmp(name, "sRGB", 4)
                    && (!strncmp(argv[i], "srgb", 4) || allb))
                || (!strncmp(name, "sTER", 4)
                    && (!strncmp(argv[i], "ster", 4) || allb))
                || (!strncmp(name, "sPLT", 4)
                    && (!strncmp(argv[i], "splt", 4) || allb))
                || (!strncmp(name, "tEXt", 4)
                    && (                                allt))
                || (!strncmp(name, "tIME", 4)
                    && (!strncmp(argv[i], "time", 4) || allb))
                || (!strncmp(name, "tRNS", 4)
                    && (!strncmp(argv[i], "trns", 4)        ))
                || (!strncmp(name, "zTXt", 4)
                    && (!strncmp(argv[i], "ztxt", 4) || allt)) )
            {
                /* (caller actually does the removal--by failing to create
                 * copy) */
                if (verbose > 0 && last_trial)
                    fprintf(STDERR, "   Removed the %s chunk.\n", name);
                return 0;
            }
        }
    }
    if (verbose > 1 && last_trial)
        fprintf(STDERR, "   Preserving the %s chunk.\n", name);
    return 1;
}




void show_result(void)
{
    if (total_output_length) {
        if (total_input_length == total_output_length)
            fprintf(STDERR, "   Overall result: no change\n");
        else if (total_input_length > total_output_length)
            fprintf(STDERR,
                    "   Overall result: %4.2f%% reduction, %lu bytes\n",
                    (100.0 -
                     (100.0 * total_output_length) / total_input_length),
                    (unsigned long)(total_input_length-total_output_length));
        else
            fprintf(STDERR,
                    "   Overall result: %4.2f%% increase, %lu bytes\n",
                    -(100.0 -
                      (100.0 * total_output_length) / total_input_length),
                    (unsigned long)(total_output_length - total_input_length));
    }

#if PNGCRUSH_TIMERS > 0
    for (pc_timer=0;pc_timer < PNGCRUSH_TIMERS; pc_timer++)
    {
      filter_count[pc_timer]+=pngcrush_timer_get_hits(pc_timer);
      t_sec=pngcrush_timer_get_seconds(pc_timer);
      t_nsec=pngcrush_timer_get_nanoseconds(pc_timer);
      t_filter[pc_timer] = (float)t_nsec/1000000000.;
      if (t_sec)
        t_filter[pc_timer] += (float)t_sec;
    }

#  if PNGCRUSH_TIMERS >= 3
    if (benchmark_iterations > 0 && verbose >= 0)
    {
      fprintf(STDERR, "   CPU time decode %.4f,", t_filter[1]);
      fprintf(STDERR, " encode %.4f,", t_filter[2]);
      fprintf(STDERR, " other %.4f,", t_filter[3]);
      fprintf(STDERR, " total %.4f sec\n", t_filter[0]);
    }
#endif

   if (verbose <= 0)
     return;

#  if PNGCRUSH_TIMERS > 9
    {
    float total_t_filter=0;
    float total_filter_count=0;
    for (pc_timer = 5; pc_timer < 10; pc_timer++)
    {
      total_t_filter+=t_filter[pc_timer];
      total_filter_count+=filter_count[pc_timer];
    }
    if (total_filter_count > 0)
    {
      for (pc_timer = 5; pc_timer < 10; pc_timer++)
      {
        fprintf(STDERR, "     filter[%u] defilter time = %15.9f, count = %lu\n",
            pc_timer-5, t_filter[pc_timer],
           (unsigned long)filter_count[pc_timer]);
      }
      fprintf(STDERR, "     total defilter time     = %15.9f, count = %lu\n",
        total_t_filter, (unsigned long) total_filter_count);
      }
    }
#  endif
#  if PNGCRUSH_TIMERS > 4
    {
      if (filter_count[4] > 0)
        fprintf(STDERR, "     deinterlace time        = %15.9f, count = %lu\n",
             t_filter[4], (unsigned long)filter_count[4]);
    }
#  endif
#  if PNGCRUSH_USE_CLOCK_GETTIME != 0
#  if PNGCRUSH_TIMERS > 1
    {
      fprintf(STDERR, "     total decode time       = %15.9f, count = %lu\n",
           t_filter[1], (unsigned long)filter_count[1]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 10
    {
      if (filter_count[10] > 0)
        fprintf(STDERR, "     filter setup time       = %15.9f, count = %lu\n",
             t_filter[10], (unsigned long)filter_count[10]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 2
    {
      fprintf(STDERR, "     total encode time       = %15.9f, count = %lu\n",
           t_filter[2], (unsigned long)filter_count[2]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 3
    {
      fprintf(STDERR, "     total misc time         = %15.9f, count = %lu\n",
           t_filter[3], (unsigned long)filter_count[3]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 0
    {
      fprintf(STDERR, "     total time              = %15.9f, count = %lu\n",
           t_filter[0], (unsigned long)filter_count[0]);
    }
#  endif
#  endif
#else
#  ifdef PNGCRUSH_TIMERS
    fprintf(STDERR, "   CPU time defilter = 0.000000\n");
    fprintf(STDERR, "   (PNGCRUSH_TIMERS=%d)\n\n", (int)PNGCRUSH_TIMERS);
#  endif
#endif

#ifdef PNG_USER_MEM_SUPPORTED
    if (current_allocation) {
        memory_infop pinfo = pinformation;
        fprintf(STDERR, "MEMORY ERROR: %d bytes still allocated\n",
                current_allocation);
        while (pinfo != NULL) {
            fprintf(STDERR, "%10lu bytes at %p\n", (unsigned long)pinfo->size,
                    (png_voidp) pinfo->pointer);
            free(pinfo->pointer);
            pinfo = pinfo->next;
        }
    }
#endif /* PNG_USER_MEM_SUPPORTED */
    if (found_acTL_chunk == 2)
        fprintf(STDERR,
        "   **** Discarded APNG chunks. ****\n");
}

void pngcrush_write_png(png_structp write_pointer, png_bytep data,
     png_size_t length)
{
    pngcrush_write_byte_count += (int) length;
    if (nosave == 0 && last_trial == 1)
      pngcrush_default_write_data(write_pointer, data, length);
}

static void pngcrush_flush(png_structp png_ptr)
{
   /* Do nothing. */
   PNGCRUSH_UNUSED(png_ptr)
}


void pngcrush_examine_pixels_fn(png_structp png_ptr, png_row_infop
    row_info, png_bytep data)
{
   if (blacken == 1 || make_gray == 1 || make_opaque == 1)
   {
      /* Check if there are any fully transparent pixels.  If one is found,
       * without the underlying color already black, set blacken==2. If the
       * PNG colortype does not support an alpha channel, set blacken==3.
       *
       * Check if there are any transparent pixels.  If one is found,
       * set make_opaque==2. If the PNG colortype does not support an alpha
       * channel, set make_opaque==3.
       *
       * Check if there are any non-gray pixels.  If one is found,
       * set make_gray == 2. If the PNG colortype is already gray, set
       * make_gray = 3.
       *
       * Check if any 16-bit pixels do not have identical high and low
       * bytes.  If one is found, set make_8_bit == 2. If the PNG bit_depth
       * is not 16-bits, set make_8_bit = 3.
       *
       * Find the maximum palette entry present in the IDAT chunks of
       * an indexed PNG.
       *
       */

      int i;

      if (row_info->color_type < 4)
      {
        blacken = 3;  /* It doesn't have an alpha channel */
        /* To do: check if tRNS chunk can be removed from color type 0 or 2 */
        make_opaque = 3;
      }

      if (row_info->color_type == 0 || row_info->color_type == 4)
        make_gray = 3; /* It's already gray! */

      if (row_info->color_type == 3)
        make_gray = 3; /* Don't change indexed PNG */

      if (row_info->bit_depth < 16)
        make_8_bit = 3;

      i = (int) row_info->rowbytes-1;

      if ((row_info->color_type == 2 || row_info->color_type == 6) &&
          make_gray == 1) /* RGB */
      {
        if (row_info->bit_depth == 8)
          {
            int incr=3;
            if (row_info->color_type == 6)
            {
               incr=4;
               i--;
            }
            for ( ; i > 0 ; )
            {
               if (data[i] != data[i-1] || data[i] != data[i-2])
                 {
                   make_gray = 2;
                 }

               i-=incr;
            }
          }

        else /* bit depth == 16 */
          {
            int incr = 6;
            if (row_info->color_type == 6)
            {
               incr = 8;
               i-=2;
            }
            for ( ; i > 0 ; )
            {
               if (data[i] != data[i-2] || data[i] != data[i-4] ||
                   data[i-1] != data[i-3] || data[i-1] != data[i-5])
               {
                  make_gray = 2;
               }
               i-=incr;
            }
          }
      }

      else if (row_info->color_type == 4 && (blacken == 1 ||
               make_opaque == 1)) /* GA */
      {
        i = (int) row_info->rowbytes-1;

        if (row_info->bit_depth == 8)
          {
            for ( ; i > 0 ; )
            {
               if (blacken == 1 && data[i] == 0 &&  data[i-1] != 0)
               {
                   blacken = 2;
               }

               if (make_opaque == 1 && data[i] != 255)
               {
                   make_opaque = 2;
               }
               i-=2;
            }
          }

        else /* bit depth == 16 */
          {
            for ( ; i > 0 ; )
            {
               if (blacken == 1 && (data[i] == 0 && data[i-1] == 0) &&
                   (data[i-2] != 0 || data[i-3] != 0))
               {
                  blacken = 2;
               }

               if (make_opaque == 1 && (data[i] != 255 || data[i-1] != 255))
               {
                   make_opaque = 2;
               }
               i-=4;
            }
          }
      }

      /* color_type == 6, RGBA */
      if (row_info->color_type == 6 && (blacken == 1 || make_gray == 1 ||
               make_opaque == 1))
      {
        i = (int) row_info->rowbytes-1;

        if (row_info->bit_depth == 8)
          {
            for ( ; i > 0 ; )
            {
               if (blacken == 1 && data[i] == 0 &&
                   (data[i-1] != 0 || data[i-2] != 0 || data[i-3] != 0))
                 {
                   blacken = 2;
                 }

               if (make_gray == 1 &&
                  (data[i-1] != data[i-2] || data[i-1] != data[i-3]))
                 {
                   make_gray = 2;
                 }

               if (make_opaque == 1 && data[i] != 255)
                 {
                   make_opaque = 2;
                 }

               i-=4;
            }
          }

        else /* bit depth == 16 */
          {
            for ( ; i > 0 ; )
            {
               if (blacken == 1 && (data[i] == 0 && data[i-1]== 0) &&
                   (data[i-2] != 0 || data[i-3] != 0 || data[i-4] != 0 ||
                   data[i-5] != 0 || data[i-6] != 0 || data[i-7] != 0))
                 {
                   blacken = 2;
                 }

               if (make_gray == 1 &&
                   (data[i-2] != data[i-4] || data[i-2] != data[i-6] ||
                   data[i-3] != data[i-5] || data[i-3] != data[i-7]))
                 {
                   make_gray = 2;
                 }

               if (make_opaque == 1 && (data[i] != 255 || data[i-1] != 255))
                 {
                   make_opaque = 2;
                 }

               i-=8;
            }
          }
      }
   }

   if (make_8_bit == 1)
   {
      int i;
      i = (int) row_info->rowbytes-1;

      if (row_info->color_type == 0)
      {
         for ( ; i > 0 ; )
         {
               if (data[i] != data[i-1])
                  make_8_bit = 2;
               i-=2;
         }
      }

      if (row_info->color_type == 2)
      {
         for ( ; i > 0 ; )
         {
               if (data[i] != data[i-1] || data[i-2] != data[i-3] ||
                   data[i-4] != data [i-5])
                  make_8_bit = 2;
               i-=6;
         }
      }

      if (row_info->color_type == 4)
      {
         for ( ; i > 0 ; )
         {
               if (data[i] != data[i-1] || data[i-2] != data[i-3])
                  make_8_bit = 2;
               i-=4;
         }
      }

      if (row_info->color_type == 6)
      {
         for ( ; i > 0 ; )
         {
               if (data[i] != data[i-1] || data[i-2] != data[i-3] ||
                   data[i-4] != data [i-5] || data[i-6] != data[i-7])
                  make_8_bit = 2;
               i-=8;
         }
      }
   }

   if (reduce_palette == 1 && row_info->color_type == 3)
   {
      int i;
      i = (int) row_info->rowbytes-1;
      
      for ( ; i > 0 ; i--)
      {
         if (data[i] >= plte_len)
            plte_len = data[i] + 1;
      }
   }
}

void pngcrush_transform_pixels_fn(png_structp png_ptr, png_row_infop row_info,
     png_bytep data)
{

   int i;

   if (blacken == 2)
   {
   /* change the underlying color of any fully transparent pixels to black */
      i=(int) row_info->rowbytes-1;

      if (row_info->color_type == 4) /* GA */
      {
        if (row_info->bit_depth == 8)
          {
            for ( ; i > 0 ; )
            {
               if (data[i] == 0 &&  data[i-1] != 0)
                  {
                     data[i-1]=0;
                  }
               i-=2;
            }
          }

        else /* bit depth == 16 */
          {
            for ( ; i > 0 ; )
            {
               if (data[i] == 0 && data[i-1] == 0)
                  {
                    data[i-2]=0;
                    data[i-3]=0;
                  }
               i-=4;
            }
          }
      }

      else /* color_type == 6, RGBA */
      {
        if (row_info->bit_depth == 8)
          {
            for ( ; i > 0 ; )
            {
               if (data[i] == 0)
                 {
                   data[i-1]=0;
                   data[i-2]=0;
                   data[i-3]=0;
                 }
               i-=4;
            }
          }

        else /* bit depth == 16 */
          {
            for ( ; i > 0 ; )
            {
               if (data[i] == 0 && data[i-1] == 0)
                 {
                   data[i-2]=0;
                   data[i-3]=0;
                   data[i-4]=0;
                   data[i-5]=0;
                   data[i-6]=0;
                   data[i-7]=0;
                 }
               i-=8;
            }
          }
      }
   }
}


int main(int argc, char *argv[])
{
    unsigned int bench = 0;
    png_uint_32 y;
    int bit_depth = 0;
    int color_type = 0;
    int num_pass, pass;
    int num_methods;

    int try10 = 0;

    char *endptr = NULL;

    /* try_method[n]: 0 means try this method;
     *
     *              : 1 means do not try this method.
     */
    int try_method[MAX_METHODSP1];
    int methods_enabled = 0;
    int last_method = MAX_METHODS;

    int fm[MAX_METHODSP1];
    int lv[MAX_METHODSP1];
    int zs[MAX_METHODSP1];
    int lev, strat, filt;

#ifdef PNG_gAMA_SUPPORTED
#  ifdef PNG_FIXED_POINT_SUPPORTED
    png_fixed_point file_gamma = 0;
#  else
    double file_gamma = 0.;
#  endif
#endif
    char *cp;
    int i;

#if PNGCRUSH_TIMERS >= 0
    for (pc_timer=0;pc_timer< PNGCRUSH_TIMERS; pc_timer++)
    {
        pngcrush_timer_reset(pc_timer);
        pngcrush_timer_min_secs[pc_timer]=0xffffffff;
        pngcrush_timer_min_nsec[pc_timer]=0xffffffff;
    }
    pngcrush_timer_start(PNGCRUSH_TIMER_TOTAL);
    pngcrush_timer_start(PNGCRUSH_TIMER_MISC);
#endif

    if (strcmp(png_libpng_ver, PNG_LIBPNG_VER_STRING))
    {
        fprintf(STDERR,
                "Warning: versions are different between png.h and png.c\n");
        fprintf(STDERR, "  png.h version: %s\n", PNG_LIBPNG_VER_STRING);
        fprintf(STDERR, "  png.c version: %s\n\n", png_libpng_ver);
    }

    row_buf = (png_bytep) NULL;
    P2(" row_buf = %p\n",row_buf);
    number_of_open_files = 0;
    do_color_count = 0;
    PNGCRUSH_UNUSED(do_color_count) /* silence compiler warning */

    strncpy(prog_string, argv[0], STR_BUF_SIZE);
    prog_string[STR_BUF_SIZE-1] = '\0';
    progname = prog_string;
    for (i = 0, cp = prog_string; *cp != '\0'; i++, cp++)
    {
#ifdef __riscos
        if (*cp == '.' || *cp == ':')
            progname = ++cp;
#else
        if (*cp == '\\' || *cp == '/')
            progname = ++cp;
#endif
    }

    /*
     * Definition of methods ("canonical list" is methods 11 and up)
     */
    for (i = 0; i <= MAX_METHODS; i++)
    {
        try_method[i] = 1;  /* 1 means do not try this method */
        fm[i] = 6; lv[i] = 9; zs[i] = 1;  /* default:  method 136 */
    }


    fm[0] = 0;   lv[0] = 0;   zs[0] = 0;   /* method  0 == uncompressed */
    fm[1] = 0;   lv[1] = 4;   zs[1] = 0;   /* method  1 == method  53 */
    fm[2] = 1;   lv[2] = 4;   zs[2] = 0;   /* method  2 == method  54 */
 /* fm[3] = 6;*/ lv[3] = 4;/* zs[3] = 1;*/ /* method  3 == method 161 */
    fm[4] = 0;/* lv[4] = 9;   zs[4] = 1;*/ /* method  4 == method 119 */
    fm[5] = 1;/* lv[5] = 9;   zs[5] = 0;*/ /* method  5 == method 114 */
 /* fm[6] = 6;   lv[6] = 9;*/ zs[6] = 0;   /* method  6 == method 157 */
    fm[7] = 0;/* lv[7] = 9;*/ zs[7] = 0;   /* method  7 == method 113 */
    fm[8] = 1;/* lv[8] = 9;   zs[8] = 1;*/ /* method  8 == method 120 */
 /* fm[9] = 6;*/ lv[9] = 2;   zs[9] = 2;   /* method  9 == method xxx */
 /* fm[10]= 6;   lv[10]= 9;   zs[10]= 1;*/ /* method 10 == method 166 */

    /* methods 11 through 16
     *
     * [strategy 2 (Z_HUFFMAN_ONLY) is independent of zlib compression level]
     */
    method = 11;
    for (filt = 0; filt <= 5; filt++)
    {
        fm[method] = filt;
        lv[method] = 2;
        zs[method] = 2;
        method++;
    }

    /*
     * methods 17 through 136 (10*2*6 = 120)
     */
    for (lev = 1; lev <= 9; lev++)
    {
        for (strat = 0; strat <= 1; strat++)
        {
            for (filt = 0; filt <= 5; filt++)
            {
                fm[method] = filt;
                lv[method] = lev;
                zs[method] = strat;
                method++;
            }
        }
    }

#ifdef Z_RLE
    /* methods 125 through 136
     *
     * [strategy 3 (Z_RLE) is mostly independent of level; 1-3 and 4-9 are
     * same]
     */
    for (filt = 0; filt <= 5; filt++)
    {
        fm[method] = filt;
        lv[method] = 1;
        zs[method] = 3;
        method++;
    }
    for (filt = 0; filt <= 5; filt++)
    {
        fm[method] = filt;
        lv[method] = 4;
        zs[method] = 3;
        method++;
    }
#endif /* Z_RLE */

    /* methods 137 through 148 (12*1*1 = 12), level 0 */

    for (strat = 0; strat <= 1; strat++)
    {
        for (filt = 0; filt <= 5; filt++)
        {
            fm[method] = filt;
            lv[method] = 0;
            zs[method] = strat;
            method++;
        }
    }

    /*
     * methods 149 through 176 (9*3*1 + 1 = 28), speedy
     */
    for (strat = 0; strat <= 3; strat++)
    {
        for (lev = 1; lev <= 9; lev++)
        {
           lv[method] = lev;
           zs[method] = strat;
           fm[method] = 6;
           method++;
           if (strat == 2)
              break; /* HUFFMAN ONLY is independent of level */
        }
    }


    num_methods = method;   /* GRR */

    /* method 177 */
    fm[method] = 0; lv[method] = 0; zs[method] = 0;  /* copy_idat */
    method++;

#define pngcrush_get_long strtol(argv[i],&endptr,10)

#define pngcrush_check_long \
    {if (errno || endptr == argv[i] || *endptr != '\0') \
      { fprintf(STDERR, "pngcrush: malformed or missing argument\n"); \
        exit(1); \
      } \
    }

#define BUMP_I \
        { i++; \
        if(i >= argc) {fprintf(STDERR,"pngcrush: insufficient parameters\n");\
        exit(1);} }

    names = 1;

    /* ===================================================================== */
    /* FIXME:  move args-processing block into separate function (470 lines) */
    for (i = 1; i < argc; i++)
    {
        errno=0;

        if (!strncmp(argv[i], "--", 2))
            argv[i]++;

        if (!strncmp(argv[i], "-", 1))
            names++;

        /* GRR:  start of giant else-if block */

        if (!strncmp(argv[i], "-bail", 5))
            bail=0;

        else if (!strncmp(argv[i], "-bench", 6))
        {
            names++;
            BUMP_I;
            benchmark_iterations = (unsigned int) pngcrush_get_long;
            pngcrush_check_long;
        }

        else if (!strncmp(argv[i], "-bkgd", 5) ||
                 !strncmp(argv[i], "-bKGD", 5))
        {
            names += 3;
            have_bkgd = 1;
            BUMP_I;
            bkgd_red = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            BUMP_I;
            bkgd_green = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            BUMP_I;
            bkgd_blue = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            bkgd_index = 0;
        }

        else if (!strncmp(argv[i], "-blacken", 8))
            blacken=1;

        else if (!strncmp(argv[i], "-brute", 6))
            /* brute force:  try everything */
        {
            methods_specified = 1;
            brute_force++;
            for (method = 1; method < num_methods; method++)
                try_method[method] = 0;
            if (brute_force_filter == 0)
                for (filt = 0; filt < 6; filt++)
                    brute_force_filters[filt] = 0;
            if (brute_force_level == 0)
                for (lev = 0; lev < 10; lev++)
                    brute_force_levels[lev] = 0;
            if (brute_force_strategy == 0)
                for (strat = 0; strat < NUM_STRATEGIES; strat++)
                    brute_force_strategies[strat] = 0;
        }

        else if (!strncmp(argv[i], "-check", 6))
        {
            check_crc = 1;
        }

        else if (!strncmp(argv[i], "-c", 3) || !strncmp(argv[i], "-col", 4))
        {
            names++;
            BUMP_I;
            force_output_color_type = pngcrush_get_long;
            pngcrush_check_long;
        }

        else if (!strncmp(argv[i], "-d", 3) || !strncmp(argv[i], "-dir", 4))
        {
            BUMP_I;
            if (pngcrush_mode == EXTENSION_MODE)
                pngcrush_mode = DIREX_MODE;
            else
                pngcrush_mode = DIRECTORY_MODE;
            directory_name = argv[names++];
        }

        else if (!strncmp(argv[i], "-exit", 5))
        {
            pngcrush_must_exit = 1;
        }

        else if (!strncmp(argv[i], "-e", 3) || !strncmp(argv[i], "-ext", 4))
        {
            BUMP_I;
            if (pngcrush_mode == DIRECTORY_MODE)
                pngcrush_mode = DIREX_MODE;
            else
                pngcrush_mode = EXTENSION_MODE;
            extension = argv[names++];
        }

        else if (!strncmp(argv[i], "-fast", 5))
        {
            /* try two fast filters */
            methods_specified = 1;
            try_method[16] = 0;
            try_method[53] = 0;
        }

        else if (!strncmp(argv[i], "-force", 6))
        {
            force = 1;
        }

        else if (!strncmp(argv[i], "-fix", 4))
        {
            salvage++;
        }

        else if (!strncmp(argv[i], "-f", 3) || !strncmp(argv[i], "-fil", 4))
        {
            int specified_filter;
            BUMP_I;
            specified_filter = pngcrush_get_long;
            pngcrush_check_long;
            if (specified_filter > 5 || specified_filter < 0)
                specified_filter = 5;
            names++;
            if (brute_force == 0)
                fm[method] = specified_filter;
            else
            {
                if (brute_force_filter == 0)
                  for (filt = 0; filt < 6; filt++)
                    brute_force_filters[filt] = 1;
                brute_force_filters[specified_filter] = 0;
                brute_force_filter++;
            }
        }

#ifdef PNG_gAMA_SUPPORTED
        else if (!strncmp(argv[i], "-g", 3) || !strncmp(argv[i], "-gam", 4))
        {
            names++;
            BUMP_I;
            found_gAMA=1;
            if (specified_intent < 0)
            {
#ifdef PNG_FIXED_POINT_SUPPORTED
                int c;
                char number[16];
                char *n = number;
                int nzeroes = -1;
                int length = strlen(argv[i]);
                for (c = 0; c < length; c++)
                {
                    if (*(argv[i] + c) == '.')
                    {
                        nzeroes = 5;
                    }
                    else if (nzeroes)
                    {
                        *n++ = *(argv[i] + c);
                        nzeroes--;
                    }
                }
                for (c = 0; c < nzeroes; c++)
                    *n++ = '0';
                *n = '\0';
                specified_gamma = strtol(number,&endptr,10);
#else
                specified_gamma = strtof(argv[i],&endptr);
#endif
                pngcrush_check_long;
            }
        }
#endif /* PNG_gAMA_SUPPORTED */

        else if (!strncmp(argv[i], "-h", 3) || !strncmp(argv[i], "-hel", 4))
        {
            ++verbose;
            print_version_info();
            printed_version_info++;
            print_usage(0);   /* this exits */
        }

        else if (!strncmp(argv[i], "-huffman", 8))
        {
            /* try all filters with huffman */
            methods_specified = 1;
            for (method = 11; method <= 16; method++)
            {
                try_method[method] = 0;
            }
        }

#ifdef PNG_iCCP_SUPPORTED
        else if (!strncmp(argv[i], "-iccp", 5))
        {
            FILE *iccp_fn;
            if (iccp_length)
                free(iccp_text);
            BUMP_I;
            iccp_length = pngcrush_get_long;
            pngcrush_check_long;
            names += 3;
            BUMP_I;
            strncpy(iccp_name, argv[i], 80);
            iccp_name[79] = '\0';
            BUMP_I;
            iccp_file = argv[i];
            if ((iccp_fn = FOPEN(iccp_file, "rb")) == NULL) {
                fprintf(STDERR, "Could not find file: %s\n", iccp_file);
                iccp_length = 0;
            }
            else
            {
                int ic;
                number_of_open_files++;
                iccp_text = (char*)malloc(iccp_length);
                if (iccp_text == NULL)
                {
                   fprintf(STDERR, "malloc of iccp_text failed\n");
                   iccp_length = 0;
                }

                for (ic = 0; ic < iccp_length; ic++)
                {
                    png_size_t num_in;
                    num_in = fread((void *)buffer, 1, 1, iccp_fn);

                    if (!num_in)
                        break;

                    iccp_text[ic] = buffer[0];
                }

                FCLOSE(iccp_fn);
            }
        }
#endif /* PNG_iCCP_SUPPORTED */

        else if (!strncmp(argv[i], "-keep", 5))
        {
            names++;
            BUMP_I;
            if (!strncmp(argv[i], "-dSIG", 5)
                    && (!strncmp(argv[i], "-dsig", 5) ))
              found_any_chunk=1;
        }

        else if (!strncmp(argv[i], "-l", 3) || !strncmp(argv[i], "-lev", 4))
        {
            int specified_level;
            BUMP_I;
            specified_level = pngcrush_get_long;
            pngcrush_check_long;
            if (specified_level > 9 || specified_level < 0)
                specified_level = 9;
            names++;
            if (brute_force == 0)
                lv[method] = specified_level;
            else
            {
                if (brute_force_level == 0)
                    for (lev = 0; lev < 10; lev++)
                        brute_force_levels[lev] = 1;
                brute_force_levels[specified_level] = 0;
                brute_force_level++;
            }
        }

        else if (!strncmp(argv[i], "-loco", 5))
        {
#ifdef PNGCRUSH_LOCO
            do_loco = 1;
#else
            fprintf
                (STDERR,"Cannot do -loco because libpng was compiled"
                 " without MNG features");
#endif
        }

        else if (!strncmp(argv[i], "-max", 4))
        {
            names++;
            BUMP_I;
            max_idat_size = (png_uint_32) pngcrush_get_long;
            pngcrush_check_long;
            if (max_idat_size == 0 || max_idat_size > PNG_UINT_31_MAX)
                max_idat_size = PNG_ZBUF_SIZE;
        }

        else if (!strncmp(argv[i], "-m", 3) || !strncmp(argv[i], "-met", 4))
        {
            names++;
            BUMP_I;
            method = pngcrush_get_long;
            pngcrush_check_long;
            if (method >= 1 && method <= MAX_METHODS)
            {
              methods_specified = 1;
              brute_force = 0;
              try_method[method] = 0;
            }
            else
            {
              fprintf(STDERR, "\n  Ignoring invalid method: %d\n",
                      method);
              method = MAX_METHODS;
            }
        }

#ifdef PNGCRUSH_LOCO
        else if (!strncmp(argv[i], "-mng", 4))
        {
            names++;
            BUMP_I;
            mngname = argv[i];
            new_mng++;
        }
#endif

        else if (!strncmp(argv[i], "-new", 4))
        {
            make_opaque = 1;                 /* -reduce */
            make_gray = 1;                   /* -reduce */
            make_8_bit = 1;                  /* -reduce */
            reduce_palette = 1;              /* -reduce */
        }

        else if (!strncmp(argv[i], "-nobail", 7))
            bail=1;

        else if (!strncmp(argv[i], "-nocheck", 8))
        {
            check_crc = 0;
        }

        else if (!strncmp(argv[i], "-nofilecheck", 5))
        {
            nofilecheck++;
        }
        else if (!strncmp(argv[i], "-noforce", 6))
        {
            force = 0;
        }


        else if (!strncmp(argv[i], "-nolimits", 5))
        {
            no_limits++;
        }

        else if (!strncmp(argv[i], "-noreduce_pal", 13))
        {
            reduce_palette = 0;
        }

        else if (!strncmp(argv[i], "-noreduce", 9))
        {
            make_opaque = 0;
            make_gray = 0;
            make_8_bit = 0;
            reduce_palette = 0;
        }

        else if (!strncmp(argv[i], "-n", 3) || !strncmp(argv[i], "-nos", 4))
        {
            /* no save; I just use this for testing decode speed */
            /* also to avoid saving if a CgBI chunk was found */
            nosave++;
            pngcrush_mode = EXTENSION_MODE;
        }

        else if (!strncmp(argv[i], "-oldtimestamp", 5))
        {
            new_time_stamp=0;
        }

        else if (!strncmp(argv[i], "-old", 4))
        {
            make_opaque = 0;                 /* no -reduce */
            make_gray = 0;                   /* no -reduce */
            make_8_bit = 0;                  /* no -reduce */
            reduce_palette = 0;              /* no -reduce */
        }

        else if(!strncmp(argv[i], "-ow",3))
        {
            overwrite = 1;
        }

        else if (!strncmp(argv[i], "-pplt", 3))
        {
            names++;
            do_pplt++;
            BUMP_I;
            strncpy(pplt_string, argv[i], STR_BUF_SIZE);
            pplt_string[STR_BUF_SIZE-1] = '\0';
        }

        else if (!strncmp(argv[i], "-premultiply", 5))
        {
            premultiply=2;
        }

        else if (!strncmp(argv[i], "-p", 3) || !strncmp(argv[i], "-pau", 4))
        {
            pauses++;
        }

        else if (!strncmp(argv[i], "-q", 3) || !strncmp(argv[i], "-qui", 4))
        {
            /* quiet, does not suppress warnings or timing */
            verbose = 0;
        }

        else if (!strncmp(argv[i], "-reduce_pal", 11))
        {
            reduce_palette = 1;
        }

        else if (!strncmp(argv[i], "-reduce", 7))
        {
            noreduce = 0;
            make_opaque = 1;
            make_gray = 1;
            make_8_bit = 1;
            reduce_palette = 1;
        }

#ifdef PNG_gAMA_SUPPORTED
        else if (!strncmp(argv[i], "-replace_gamma", 4))
        {
            names++;
            BUMP_I;
            found_gAMA=1;
            {
#ifdef PNG_FIXED_POINT_SUPPORTED
                int c;
                char number[16];
                char *n = number;
                int nzeroes = -1;
                int length = strlen(argv[i]);
                for (c = 0; c < length; c++)
                {
                    if (*(argv[i] + c) == '.')
                    {
                        nzeroes = 5;
                    }
                    else if (nzeroes)
                    {
                        *n++ = *(argv[i] + c);
                        nzeroes--;
                    }
                }
                for (c = 0; c < nzeroes; c++)
                    *n++ = '0';
                *n = '\0';
                force_specified_gamma = strtol(number,&endptr,10);
#else
                force_specified_gamma = strtof(argv[i],&endptr);
#endif
                pngcrush_check_long;
            }
        }
#endif /* PNG_gAMA_SUPPORTED */

#ifdef PNG_pHYs_SUPPORTED
        else if (!strncmp(argv[i], "-res", 4))
        {
            names++;
            BUMP_I;
            resolution = pngcrush_get_long;
            pngcrush_check_long;
        }
#endif

#ifdef Z_RLE
        else if (!strncmp(argv[i], "-rle", 4))
        {
            /* try all filters with RLE */
            methods_specified = 1;
            for (method = 125; method <= 136; method++)
            {
                try_method[method] = 0;
            }
        }
#endif

#ifdef PNGCRUSH_MULTIPLE_ROWS
        else if (!strncmp(argv[i], "-rows", 5))
        {
            names++;
            BUMP_I;
            max_rows_at_a_time = pngcrush_get_long;
            pngcrush_check_long;
        }
#endif

        else if (!strncmp(argv[i], "-r", 3) || !strncmp(argv[i], "-rem", 4))
        {
            remove_chunks = i;
            names++;
            BUMP_I;
            if (!strncmp(argv[i], "-dSIG", 5)
                 && (!strncmp(argv[i], "-dsig", 5)))
               image_is_immutable=0;
        }

        else if (!strncmp(argv[i], "-save", 5))
        {
            all_chunks_are_safe++;
        }

        else if (!strncmp(argv[i], "-speed", 6))
        {
            speed = 1;
        }

        else if (!strncmp(argv[i], "-srgb", 5) ||
                   !strncmp(argv[i], "-sRGB", 5))
        {
#ifdef PNG_gAMA_SUPPORTED
#  ifdef PNG_FIXED_POINT_SUPPORTED
            specified_gamma = 45455L;
#  else
            specified_gamma = 0.45455;
#  endif
#endif
            specified_intent = 0;
            BUMP_I;
            if (!strncmp(argv[i], "0", 1) ||
                !strncmp(argv[i], "1", 1) ||
                !strncmp(argv[i], "2", 1) ||
                !strncmp(argv[i], "3", 1))
            {
                names++;
                specified_intent = (int) pngcrush_get_long;
                pngcrush_check_long;
            } else
                i--;
        }

        else if (!strncmp(argv[i], "-ster", 5) ||
                   !strncmp(argv[i], "-sTER", 5))
        {
            BUMP_I;
            ster_mode = -1;
            if (!strncmp(argv[i], "0", 1) ||
                !strncmp(argv[i], "1", 1))
            {
                names++;
                ster_mode = (int) pngcrush_get_long;
                pngcrush_check_long;
            }
            else
                i--;
        }

        else if (!strncmp(argv[i], "-s", 3) || !strncmp(argv[i], "-sil", 4))
        {
            /* silent, suppresses warnings, timing, and results */
            verbose = -1;
        }

        else if (!strncmp(argv[i], "-text", 5)
                 || !strncmp(argv[i], "-tEXt", 5) ||
#ifdef PNG_iTXt_SUPPORTED
                 !strncmp(argv[i], "-itxt", 5)
                 || !strncmp(argv[i], "-iTXt", 5)
                 || !strncmp(argv[i], "-zitxt", 6)
                 || !strncmp(argv[i], "-ziTXt", 6) ||
#endif
                 !strncmp(argv[i], "-ztxt", 5)
                 || !strncmp(argv[i], "-zTXt", 5))
        {
            i += 2;
            BUMP_I;
            i -= 3;
            if (strlen(argv[i + 2]) < 80 &&
                strlen(argv[i + 3]) < STR_BUF_SIZE &&
                text_inputs < 10)
            {
#ifdef PNG_iTXt_SUPPORTED
                if (!strncmp(argv[i], "-zi", 3))
                {
                    text_compression[text_inputs] =
                        PNG_ITXT_COMPRESSION_zTXt;
                    /* names += 2; */
                }
                else
#endif
                if (!strncmp(argv[i], "-z", 2))
                    text_compression[text_inputs] =
                        PNG_TEXT_COMPRESSION_zTXt;
                else if (!strncmp(argv[i], "-t", 2))
                    text_compression[text_inputs] =
                        PNG_TEXT_COMPRESSION_NONE;
#ifdef PNG_iTXt_SUPPORTED
                else
                {
                    text_compression[text_inputs] =
                        PNG_ITXT_COMPRESSION_NONE;
                    /* names += 2; */
                }
#endif
                names += 3;
                if (!strncmp(argv[++i], "b", 1))
                    text_where[text_inputs] = 1;
                if (!strncmp(argv[i], "a", 1))
                    text_where[text_inputs] = 2;
                strncpy(&text_keyword[text_inputs * 80], argv[++i],
                    80);
                text_keyword[text_inputs * 80 + 79] = '\0';
#ifdef PNG_iTXt_SUPPORTED
                if (text_compression[text_inputs] <= 0)
                {
                    text_lang[text_inputs * 80] = '\0';
                    text_lang_key[text_inputs * 80] = '\0';
                }
                else
                {
                    i += 2;
                    BUMP_I;
                    i -= 3;
                    names += 2;
                    strncpy(&text_lang[text_inputs * 80], argv[++i], 80);
                    text_lang[text_inputs * 80 + 79] = '\0';
                    /* libpng-1.0.5j and later */
                    strncpy(&text_lang_key[text_inputs * 80], argv[++i], 80);
                    text_lang_key[text_inputs * 80 + 79] = '\0';
                }
#endif
                strncpy(&text_text[text_inputs * STR_BUF_SIZE], argv[++i],
                    STR_BUF_SIZE);
                text_text[(text_inputs + 1) * STR_BUF_SIZE - 1] = '\0';
                text_inputs++;
            } else {
                if (text_inputs > 9)
                    fprintf(STDERR,
                            "too many text/zTXt inputs; only 10 allowed\n");
                else
                    fprintf(STDERR,
                            "keyword exceeds 79 characters or text"
                            " exceeds 2047 characters\n");
                i += 3;
                names += 3;
#ifdef PNG_iTXt_SUPPORTED
                if (!strncmp(argv[i], "-i", 3) || !strncmp(argv[i], "-itx", 4))
                {
                    i++;
                    BUMP_I;
                    names += 2;
                }
#endif
            }
        }

        else if (!strncmp(argv[i], "-time_stamp", 5) ||  /* legacy */
                 !strncmp(argv[i], "-newtimestamp", 5))
            new_time_stamp=1;

#ifdef PNG_tRNS_SUPPORTED
        else if (!strncmp(argv[i], "-trns_a", 7) ||
                 !strncmp(argv[i], "-tRNS_a", 7))
        {
            num_trans_in = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            if (num_trans_in > 256)
               num_trans_in = 256;
            trns_index=num_trans_in-1;
            have_trns = 1;
            for (ia = 0; ia < num_trans_in; ia++)
            {
                BUMP_I;
                trans_in[ia] = (png_byte) pngcrush_get_long;
                pngcrush_check_long;
            }
            names += 1 + num_trans_in;
        }

        else if (!strncmp(argv[i], "-trns", 5) ||
                   !strncmp(argv[i], "-tRNS", 5))
        {
            names += 5;
            have_trns = 1;
            BUMP_I;
            trns_index = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            BUMP_I;
            trns_red = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            BUMP_I;
            trns_green = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            BUMP_I;
            trns_blue = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
            BUMP_I;
            trns_gray = (png_uint_16) pngcrush_get_long;
            pngcrush_check_long;
        }
#endif /* tRNS */

        else if(!strncmp(argv[i], "-try10",6))
        {
            try10 = 1;
        }

        else if (!strncmp(argv[i], "-version", 8))
        {
            fprintf(STDERR, " pngcrush ");
            fprintf(STDERR, PNGCRUSH_VERSION);
            fprintf(STDERR, ", uses libpng ");
            fprintf(STDERR, PNG_LIBPNG_VER_STRING);
            fprintf(STDERR, " and zlib ");
            fprintf(STDERR, ZLIB_VERSION);
            fprintf(STDERR, "\n Check http://pmt.sf.net/\n");
            fprintf(STDERR, " for the most recent version.\n");
            verbose = 0;
            exit(0);
        }

        else if (!strncmp(argv[i], "-v", 3) || !strncmp(argv[i], "-ver", 4))
        {
            verbose++;
        }

        else if (!strncmp(argv[i], "-warn", 5))
        {
            show_warnings++;
            verbose = -1;
        }

        else if (!strncmp(argv[i], "-w", 3) || !strncmp(argv[i], "-win", 4))
        {
            BUMP_I;
            default_compression_window = pngcrush_get_long;
            pngcrush_check_long;
            force_compression_window++;
            names++;
        }

        else if (!strncmp(argv[i], "-zm", 4) || !strncmp(argv[i], "-zmem", 5))
        {
            BUMP_I;
            compression_mem_level = pngcrush_get_long;
            pngcrush_check_long;
            names++;
        }

        else if (!strncmp(argv[i], "-z", 3))
        {
            int specified_strategy;
            BUMP_I;
            specified_strategy = pngcrush_get_long;
            pngcrush_check_long;
            if (specified_strategy > 2 || specified_strategy < 0)
                specified_strategy = 0;
            names++;
            if (brute_force == 0)
                zs[method] = specified_strategy;
            else
            {
                if (brute_force_strategy == 0)
                    for (strat = 0; strat < 2; strat++)
                        brute_force_strategies[strat] = 1;
                brute_force_strategies[specified_strategy] = 0;
                brute_force_strategy++;
            }
        }

        else if (!strncmp(argv[i], "-", 1))
        {
            if (verbose > 0 && printed_version_info == 0)
            {
              print_version_info();
              printed_version_info++;
            }
            fprintf(STDERR, "\n  Ignoring invalid option: %s\n",
                    argv[i]);
        } /* GRR:  end of giant if-else block */
    } /* end of loop over args ============================================ */

    if (verbose > 0 && printed_version_info == 0)
        print_version_info();

    if (default_compression_window == 32)
        default_compression_window = 15;
    else if (default_compression_window == 16)
        default_compression_window = 14;
    else if (default_compression_window == 8)
        default_compression_window = 13;
    else if (default_compression_window == 4)
        default_compression_window = 12;
    else if (default_compression_window == 2)
        default_compression_window = 11;
    else if (default_compression_window == 1)
        default_compression_window = 10;
    else if (default_compression_window == 512)
        default_compression_window = 9;
    /* Use of compression window size 256 is not recommended. */
    else if (default_compression_window == 256)
        default_compression_window = 8;
    else if (default_compression_window == 0)
        /* do nothing */;
    else if (default_compression_window != 15) {
        fprintf(STDERR, "Invalid window size (%d); using window size=4\n",
                default_compression_window);
        default_compression_window = 12;
    }

    if (pngcrush_mode == DEFAULT_MODE)
    {
        if (argc - names == 2)
        {
            inname = argv[names];
            outname = argv[names + 1];
        }

        else if (overwrite)
        {
            inname = argv[names];
        }

        else
        {
            if ((argc - names == 1 || nosave))
            {
                inname = argv[names];
            }
            if (verbose > 0 && !nosave)
            {
                print_usage(1);   /* this exits */
            }
        }
    }

    first_name = names;

    if (benchmark_iterations)
        bench=1;
    else
        bench=0;
    
    for (; bench <= benchmark_iterations; bench++)
    {
        if (benchmark_iterations > 0)
        {
            P1("  Pngcrush benchmark iteration %d\n",bench);
            names = first_name;
        }

#if PNGCRUSH_TIMERS > 0
        for (pc_timer = 0; pc_timer < PNGCRUSH_TIMERS; pc_timer++)
        {
            pngcrush_timer_reset(pc_timer);
            filter_count[pc_timer]=0;
        }
        pngcrush_timer_start(PNGCRUSH_TIMER_TOTAL);
        pngcrush_timer_start(PNGCRUSH_TIMER_MISC);
#endif

    for (ia = 0; ia < 256; ia++)
        trns_array[ia]=255;

    for (;;)  /* loop on input files */
    {
        methods_enabled = 0;
        last_trial = 0;

        if (png_row_filters != NULL)
        {
            free(png_row_filters);
            png_row_filters = NULL;
        }

        image_specified_gamma = 0;
        intent=specified_intent;
        
        inname = argv[names++];

        if (inname == NULL)
        {
#if PNGCRUSH_TIMERS > 0
            pngcrush_timer_stop(PNGCRUSH_TIMER_MISC);
            pngcrush_timer_stop(PNGCRUSH_TIMER_TOTAL);
#endif
            if (verbose >= 0)
            {
                show_result();
            }
            break;
        }

        if (pngcrush_mode == DIRECTORY_MODE || pngcrush_mode == DIREX_MODE) {
            int inlen, outlen;
#ifndef __riscos
            struct stat stat_buf;
            if (stat(directory_name, &stat_buf))
#else
            if (fileexists(directory_name) & 2)
#endif
            {
#if defined(_MBCS) || defined(WIN32) || defined(__WIN32__)
                if (_mkdir(directory_name))
#else
                if (mkdir(directory_name, 0755))
#endif
                {
                    fprintf(STDERR,
                      "pngcrush: could not create directory %s\n",
                      directory_name);
                    exit(1);
                }
                nofilecheck = 1;
            }
            outlen = strlen(directory_name);
            if (outlen >= STR_BUF_SIZE-1)
            {
                fprintf(STDERR,
                  "pngcrush: directory %s is too long for buffer\n",
                  directory_name);
                exit(1);
            }

            strcpy(out_string, directory_name);
            /* Append a slash if it hasn't already got one at the end. */
            if (out_string[outlen-1] != SLASH[0] &&
                out_string[outlen-1] != FWD_SLASH[0] &&
                out_string[outlen-1] != BACK_SLASH[0])
              out_string[outlen++] = SLASH[0];   /* (assume SLASH is 1 byte) */
            out_string[outlen] = '\0';

            inlen = strlen(inname);
            if (inlen >= STR_BUF_SIZE)
            {
                fprintf(STDERR,
                   "pngcrush: filename %s is too long for buffer\n", inname);
                exit(1);
            }
            strcpy(in_string, inname);
            in_string[inlen] = '\0';
#ifdef __riscos
            op = strrchr(in_string, '.');
            if (!op)
                op = in_string;
            else
                op++;
#else
            op = in_string;
            ip = in_string + inlen - 1;   /* start at last char in string */
            while (ip > in_string)
            {
                if (*ip == '\\' || *ip == '/')
                {
                    op = ip + 1;
                    break;
                }
                --ip;
            }
#endif

            if (outlen + (inlen - (op - in_string)) >= STR_BUF_SIZE)
            {
                fprintf(STDERR,
                   "pngcrush: full path is too long for buffer\n");
                exit(1);
            }
            strcpy(out_string+outlen, op);
            /*outlen += inlen - (op - in_string); */
            outname = out_string;
        }

        if (overwrite && (pngcrush_mode == EXTENSION_MODE ||
            pngcrush_mode == DIRECTORY_MODE ||
            pngcrush_mode == DIREX_MODE))
        {
            if (overwrite > 0)
            {
               P1( "Ignoring \"-ow\"; cannot use it with \"-d\" or \"-e\"");
               overwrite=0;
            }
        }

        /*
         * FIXME:  need same input-validation fixes (as above) here, too
         *
         * FIXME:  what was the point of setting in_string and out_string in
         *         DIREX_MODE above if going to do all over again here?
         */
        if (pngcrush_mode == EXTENSION_MODE || pngcrush_mode == DIREX_MODE)
        {
            ip = in_string;
            in_string[0] = '\0';
            if (pngcrush_mode == EXTENSION_MODE)
                strncat(in_string, inname, STR_BUF_SIZE-1);
            else
                strncat(in_string, outname, STR_BUF_SIZE-1);
            ip = in_string;
            op = dot = out_string;
            while (*ip != '\0')
            {
                *op++ = *ip++;
#ifdef __riscos
                if (*ip == '/')
                    dot = op;
#else
                if (*ip == '.')
                    dot = op;
#endif
            }
            *op = '\0';

            if (dot != out_string)
                *dot = '\0';

            in_extension[0] = '\0';
            if (dot != out_string)
            {
                strncat(in_extension, ++dot, STR_BUF_SIZE - 1);
            }

            strncat(out_string, extension, STR_BUF_SIZE - 1);
            outname = out_string;
        }

        if ((outname[strlen(outname) - 4] == 'p') &&
            (outname[strlen(outname) - 3] == 'p') &&
            (outname[strlen(outname) - 2] == 'n') &&
            (outname[strlen(outname) - 1] == 'g'))
        {
           /* Writing a *.ppng (png with premultiplied alpha) */
            premultiply=2;
#ifndef PNG_READ_PREMULTIPLY_ALPHA_SUPPORTED
            png_error(read_ptr, "Premultiplied alpha is not supported");
#endif
        }

        if ((outname[strlen(outname) - 4] == 'a') &&
            (outname[strlen(outname) - 3] == 'p') &&
            (outname[strlen(outname) - 2] == 'n') &&
            (outname[strlen(outname) - 1] == 'g'))
        {
           /* Writing an APNG */
           save_apng_chunks=1;
        }

        if (nosave < 2)
        {
            P1( "Opening file %s for length measurement\n",
                       inname);

            if ((fpin = FOPEN(inname, "rb")) == NULL)
            {
                fprintf(STDERR, "Could not find file: %s\n", inname);
                continue;
            }
            number_of_open_files++;

#ifdef PNGCRUSH_LOCO
            if (new_mng)
            {

#  ifdef PNG_USER_MEM_SUPPORTED
               if (verbose > 0)
                  mng_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING,
                    (png_voidp) NULL, (png_error_ptr) pngcrush_cexcept_error,
                    (png_error_ptr) pngcrush_warning, (png_voidp) NULL, 
                    (png_malloc_ptr) pngcrush_debug_malloc,
                    (png_free_ptr) pngcrush_debug_free);
               else
#  endif
                  mng_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                    (png_voidp) NULL,
                    (png_error_ptr) pngcrush_cexcept_error,
                    (png_error_ptr) pngcrush_warning);
               if (mng_ptr == NULL)
                  fprintf(STDERR, "pngcrush could not create mng_ptr");

               if ((mng_out = FOPEN(mngname, "wb")) == NULL)
               {
                  fprintf(STDERR,
                      "pngcrush: could not open output file %s\n",
                      mngname);
                  FCLOSE(fpin);
                  exit(1);
               }
               number_of_open_files++;
               png_init_io(mng_ptr, mng_out);
               png_set_write_fn(mng_ptr, (png_voidp) mng_out,
                                (png_rw_ptr) pngcrush_write_png,
                                pngcrush_flush);
            }
#endif /* PNGCRUSH_LOCO */

            idat_length[0] = measure_idats(fpin);

#ifdef PNGCRUSH_LOCO
            if (new_mng)
            {
                    png_destroy_write_struct(&mng_ptr, NULL);
                    FCLOSE(mng_out);
            }
#endif

            FCLOSE(fpin);


            if (verbose >= 0 && bench < 2)
            {
                if (nosave)
                {
                  fprintf(STDERR, "  %s:\n", inname);
                }
                else if (overwrite)
                {
                  fprintf(STDERR,
                    "  Recompressing IDAT chunks in %s\n", inname);
                }
                else
                {
                  fprintf(STDERR,
                    "  Recompressing IDAT chunks in %s to %s\n",
                    inname, outname);
                }
                fprintf(STDERR,
                  "   Total length of data found in critical chunks        "
                  "    =%10lu\n", (unsigned long)idat_length[0]);
                fflush(STDERR);
            }

            if (idat_length[0] == 0)
                continue;

        }

        else
            idat_length[0] = 1;

        if (image_is_immutable)
        {
            fprintf(STDERR,
              "   Image %s has a dSIG chunk and is immutable.\n", inname);
        }

        if (!image_is_immutable)
        {

        if (force_output_color_type != 8 &&
            force_output_color_type != 0 &&
            force_output_color_type != 2 &&
            force_output_color_type != 3 &&
            force_output_color_type != 4 &&
            force_output_color_type != 6)
        {
            fprintf(STDERR, "\n  Ignoring invalid color_type: %d\n",
              force_output_color_type);
            force_output_color_type=8;
        }
        output_color_type = force_output_color_type;

        output_bit_depth = force_output_bit_depth;

        best_of_three = 1;
        pngcrush_best_byte_count=0xffffffff;

        if (blacken == 1 || make_gray == 1 || make_opaque == 1 ||
            reduce_palette == 1)
        {
           try_method[0] = 0;
        }

        /*
         * From the PNG spec, various dependencies among chunk types
         *   must be accounted for during any reduction of color type
         *   or bit depth:
         *
         * IHDR valid bit depth depends on color type
         *   valid filter type depends on color type (for MNG extensions)
         * tRNS depends on color type
         *   depends on num_palette for color type 3 (palette)
         * iCCP valid profile depends on color type
         * sBIT depends on color type and bit depth
         * bKGD depends on color type and bit depth
         * hIST depends on num_palette
         *
         * Chunk types present have been detected in the first pass over
         *   the file, in the measure_idat() function.
         */

        if (make_gray)
        {
           if ((found_iCCP && keep_unknown_chunk("iCCP", argv)) ||
               (found_color_bKGD && keep_unknown_chunk("bKGD", argv)) ||
               found_acTL_chunk == 1 ||
               (found_sBIT_different_RGB_bits &&
               keep_unknown_chunk("sBIT", argv)))
           {
              P1 ("Cannot change colortype to gray when iCCP,"
                  " acTL, bKGD with color, or sBIT chunk is present\n");
              make_gray = 0;
           }
           else
           {
              make_gray = 1;
              try_method[0] = 0;
           }
        }

        if (make_opaque)
        {
           if (found_tRNS || found_acTL_chunk == 1)
           {
             P1("Cannot remove the alpha channel when tRNS"
                  " or acTL chunk is present\n");
             make_opaque = 0;
           }
           else
           {
             make_opaque = 1;
             try_method[0] = 0;
           }
        }

        if (make_8_bit)
        {
           if ((found_bKGD && keep_unknown_chunk("bKGD", argv)) ||
              found_acTL_chunk == 1 ||
              (found_sBIT_max > 8 && keep_unknown_chunk("sBIT", argv)))
           {
              P1 ("Cannot reduce bit depth to 8 when bKGD,"
                  " sBIT or acTL chunk is present\n");
              make_8_bit = 0;
           }
           else
           {
             make_8_bit = 1;
             try_method[0] = 0;
           }
        }

        if (input_color_type == 3 && reduce_palette)
        {
           if ((found_hIST && keep_unknown_chunk("hIST", argv)) ||
              found_acTL_chunk == 1)
           {
             P1("Cannot reduce palette length when hIST"
                 " or acTL chunk is present\n");
             reduce_palette = 0;
             plte_len = -1;
           }
           else
           {
             try_method[0] = 0;
             plte_len = 0;
           }
        }

        /* Handle specified brute_force options */
        if (brute_force_level || brute_force_filter || brute_force_strategy)
        {
           for (method = 1; method < num_methods; method++)
           {
             int option;

             try_method[method]=1;
             if (brute_force_level)
             {
                for (option = 0; option < 10; option++)
                   if (option == lv[method])
                      try_method[method]=brute_force_levels[option];
             }

             if ((try_method[method] == 0) && brute_force_filter)
             {
                for (option = 0; option < 6; option++)
                   if (option == fm[method])
                      try_method[method]=brute_force_filters[option];
             }

             if ((try_method[method] == 0) && brute_force_strategy)
             {
                for (option = 0; option < NUM_STRATEGIES; option++)
                   if (option == zs[method])
                      try_method[method]=brute_force_strategies[option];
             }

             if (method && method < 11)
                try_method[method] = 1;
           }

           if (speed)
           {
             /* Do not try AVG or PAETH */
             for (method = 1; method < num_methods; method++)
             {
               if (try_method[method] == 0 && (fm[method] == 3 ||
                   fm[method] == 4 || fm[method] == 5))
                     try_method[method] = 1;
             }
           }
         }

        if (methods_specified == 0 || try10 != 0)
        {
            for (i = 0; i <= DEFAULT_METHODS; i++)
                try_method[i] = 0;

            try_method[6] = try10;
        }

        for (i = 1; i <= MAX_METHODS; i++)
        {
           methods_enabled += (1 - try_method[i]);
        }
        P1("%d methods enabled\n",methods_enabled);
/* Skip trial 0 when a single method was specified and -reduce was not */
        if (methods_specified != 0 && noreduce != 0)
        {
          if (methods_enabled == 1)
          {
             try_method[0] = 1;
             make_opaque = 0;
             make_gray = 0;
             make_8_bit = 0;
             reduce_palette = 0;
          }
        }

        last_method = 0;
        for (i = 1; i <= MAX_METHODS; i++)
        {
           if (try_method[i] == 0)
              last_method = i;
        }

        if (methods_enabled > 1)
           last_method++;

        P1("   pngcrush: methods     = %d\n",methods_enabled);
        P1("   pngcrush: last_method = %d\n",last_method);
        
        if (methods_enabled == 1 && last_method == 176)
           copy_idat = 1;

        best_of_three = 1;

#ifndef __riscos
        {
            /* COVERITY complains about TOCTOU when inname is used later */
            struct stat stat_buf;
            stat(inname, &stat_buf);
            input_length = (unsigned long) stat_buf.st_size;
        }
#else
        input_length = (unsigned long) filesize(inname);
#endif

        /* ////////////////////////////////////////////////////////////////////
        ////////////////                                   ////////////////////
        ////////////////  START OF MAIN LOOP OVER METHODS  ////////////////////
        ////////////////                                   ////////////////////
        //////////////////////////////////////////////////////////////////// */

        /* MAX_METHODS is 177 */
        P1("\n\nENTERING MAIN LOOP OVER %d METHODS\n", MAX_METHODS);
        for (trial = 0; trial <= last_method; trial++)
        {
            if (nosave || trial == last_method)
               last_trial = 1;

            if (verbose > 1)
            fprintf(STDERR, "pngcrush: trial = %d\n",trial);

            pngcrush_write_byte_count=0;
#ifdef PNGCRUSH_H
# if ZLIB_VERNUM > 0x1240
            if (last_trial == 0)
               pngcrush_write_byte_count=6; /* zlib header that isn't written */
# endif
#endif

            found_IDAT = 0;

            if (trial != 0)
               idat_length[trial] = (png_uint_32) 0xffffffff;

            /* this part of if-block is for final write-the-best-file
               iteration */
            if (trial == last_method)
            {
                png_uint_32 best_length;

                if (methods_enabled == 1)
                {
                   best = trial;
                   best_length = idat_length[trial];
                }
                else
                {
                   int j;

                   /* check lengths */
                   best = 0;  /* i.e., input file */
                   best_length = (png_uint_32) 0xffffffff;
                   for (j = 0; j <= last_method; j++)
                   {
                       if (best == 0 && best_length == idat_length[j])
                       {
                           /* If no change, report the first match */
                           best = j;
                       }
                       if ((force == 0 || j != 0) &&
                            best_length > idat_length[j])
                       {
                           best_length = idat_length[j];
                           best = j;
                       }
                       if (j > 148 && j < 176 && best_length == idat_length[j])
                       {
                           /* break ties in favor of method 6 */
                           best = j;
                       }
                   }
                }

                if (image_is_immutable ||
                     (idat_length[best] == idat_length[0] &&
                     force == 0 && nosave == 0))
                {
                    /* just copy input to output */

                    P2("prepare to copy input to output\n");
                    pngcrush_pause();

                    if ((fpin = FOPEN(inname, "rb")) == NULL)
                    {
                        fprintf(STDERR, "Could not find input file %s\n",
                                inname);
                        continue;
                    }

                    number_of_open_files++;
                    if ((fpout = FOPEN(outname, "wb")) == NULL)
                    {
                        fprintf(STDERR,
                           "pngcrush: could not open output file %s\n",
                           outname);
                        FCLOSE(fpin);
                        exit(1);
                    }

                    number_of_open_files++;
                    P2("copying input to output...");

                    for (;;)
                    {
                        png_size_t num_in, num_out;

                        num_in = fread((void *)buffer, 1, 1, fpin);
                        if (!num_in)
                            break;
                        num_out = fwrite(buffer, 1, 1, fpout);
                        if (num_out != num_in)
                            P2("copy error.\n");
                    }
                    P2("copy complete.\n");
                    pngcrush_pause();
                    FCLOSE(fpin);
                    FCLOSE(fpout);
                    setfiletype(outname);
                    break;
                }

                filter_type = fm[best];
                zlib_level = lv[best];
                if (zs[best] == 1)
                    z_strategy = Z_FILTERED;
                else if (zs[best] == 2)
                    z_strategy = Z_HUFFMAN_ONLY;
#ifdef Z_RLE
                else if (zs[best] == 3)
                    z_strategy = Z_RLE;
#endif
                else /* if (zs[best] == 0) */
                    z_strategy = Z_DEFAULT_STRATEGY;
            }

            else /* Trial < last_method */
            {
                if (trial > 2 && trial < 5 && idat_length[trial - 1]
                    < idat_length[best_of_three])
                    best_of_three = trial - 1;

                if (try_method[trial])
                {
                    P2("skipping \"late\" trial %d\n", trial);
                    continue;
                }

                /* default behavior is to do the heuristics (6 of the first
                 * 10 methods). try10 means try all of 1-10.
                 */
                if (!methods_specified && try10 == 0)
                {
                    if ((trial == 4 || trial == 7) && best_of_three != 1)
                    {
                        P2("skipping \"early\" trial %d\n", trial);
                        continue;
                    }
                    if ((trial == 5 || trial == 8) && best_of_three != 2)
                    {
                        P2("skipping \"early\" trial %d\n", trial);
                        continue;
                    }
                    if ((trial == 6 || trial == 9 || trial == 10)
                        && best_of_three != 3)
                    {
                        P2("skipping \"early\" trial %d\n", trial);
                        continue;
                    }
                }
                filter_type = fm[trial];
                zlib_level = lv[trial];
                if (zs[trial] == 1)
                    z_strategy = Z_FILTERED;
                else if (zs[trial] == 2)
                    z_strategy = Z_HUFFMAN_ONLY;
#ifdef Z_RLE
                else if (zs[trial] == 3)
                    z_strategy = Z_RLE;
#endif
                else /* if (zs[trial] == 0) */
                    z_strategy = Z_DEFAULT_STRATEGY;
                final_method = trial;
                if (!nosave)
                {
                    P2("\n\n------------------------------------------------\n"
                       "Begin trial %d, filter %d, strategy %d, level %d\n",
                       trial, filter_type, z_strategy, zlib_level);
                }
            }

            P2("prepare to open files.\n");
            pngcrush_pause();

            if ((fpin = FOPEN(inname, "rb")) == NULL)
            {
                fprintf(STDERR, "Could not find input file %s\n", inname);
                continue;
            }
            number_of_open_files++;

            if (last_trial && nosave == 0)
            {
#ifndef __riscos
                /* Can't sensibly check this on RISC OS without opening a file
                   for update or output
                 */
                struct stat stat_in, stat_out;
                if (last_trial && !nofilecheck
                    && (stat(inname, &stat_in) == 0)
                    && (stat(outname, &stat_out) == 0) &&
#if defined(_MSC_VER) || defined(__MINGW32__)   /* maybe others? */
                    /* MSVC++6.0 will erroneously return 0 for both files, so
                       we simply check the size instead.  It is possible that
                       we will erroneously reject the attempt when inputsize
                       and outputsize are equal, for different files
                     */
                    (stat_in.st_size == stat_out.st_size) &&
#else
                    (stat_in.st_ino == stat_out.st_ino) &&
#endif
                    (stat_in.st_dev == stat_out.st_dev))
                {
                    fprintf(STDERR,
                            "\n   pngcrush: cannot overwrite input file %s\n",
                            outname);
                    P1("   st_ino=%d, st_size=%d\n\n",
                       (int) stat_in.st_ino, (int) stat_in.st_size);
                    FCLOSE(fpin);
                    exit(1);
                }
#endif
                if ((fpout = FOPEN(outname, "wb")) == NULL)
                {
                    fprintf(STDERR,
                            "pngcrush: could not open output file %s\n",
                            outname);
                    FCLOSE(fpin);
                    exit(1);
                }

                number_of_open_files++;
            }

            P2("files are opened.\n");
            pngcrush_pause();

/* OK to ignore any warning about the address of exception__prev in "Try" */
            Try {
                png_uint_32 row_length;
                P1( "Allocating read and write structures\n");
#ifdef PNG_USER_MEM_SUPPORTED
                if (verbose > 0)
                   read_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
                     (png_voidp) NULL,
                     (png_error_ptr) pngcrush_cexcept_error,
                     (png_error_ptr) pngcrush_warning,
                     (png_voidp) NULL,
                     (png_malloc_ptr) pngcrush_debug_malloc,
                     (png_free_ptr) pngcrush_debug_free);
                else
#endif /* PNG_USER_MEM_SUPPORTED */
                   read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                     (png_voidp) NULL,
                     (png_error_ptr) pngcrush_cexcept_error,
                     (png_error_ptr) pngcrush_warning);
                if (read_ptr == NULL)
                    Throw "pngcrush could not create read_ptr";

#ifdef PNG_BENIGN_ERRORS_SUPPORTED
# if PNGCRUSH_LIBPNG_VER >= 10400
                /* Allow certain errors in the input file to be handled
                 * as warnings.
                 */
                png_set_benign_errors(read_ptr, 1);
# endif
#endif

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
                if (no_limits == 0)
                {
# if PNGCRUSH_LIBPNG_VER >= 10400
                   png_set_user_limits(read_ptr, 500000L, 500000L);
                   png_set_chunk_cache_max(read_ptr, 500);
# endif
# if PNGCRUSH_LIBPNG_VER >= 10401
                   png_set_chunk_malloc_max(read_ptr, 2000000L);
# endif
                }
#endif /* PNG_SET_USER_LIMITS_SUPPORTED */

#if defined(PNG_MAXIMUM_INFLATE_WINDOW) && defined(PNG_OPTION_ON)
                if (salvage)
                {
                   P1(" Setting MAXIMUM_INFLATE_WINDOW\n");
                   png_set_option(read_ptr, PNG_MAXIMUM_INFLATE_WINDOW,
                       PNG_OPTION_ON);
                }
#endif

#if 0
                /* Use a smaller decompression buffer for speed */
                png_set_compression_buffer_size(read_ptr,
                    (png_size_t)256);
#endif /* 0 */

    /* Change the underlying color of any fully transparent pixel to black.
     * Remove the alpha channel from any fully-opaque image.
     * Change any all-gray image to a gray colortype.
     * Reduce 16-bit image to 8-bit if possible without loss.
     */
    if (trial == 0 &&
        (blacken == 1 || make_gray == 1 || make_opaque == 1 ||
        make_8_bit == 1 || reduce_palette == 1))
    {
      P1(" Examine image for possible lossless reductions\n");
      png_set_read_user_transform_fn(read_ptr, pngcrush_examine_pixels_fn);
    }
#ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
                if (last_trial == 0)
                {
                   png_set_keep_unknown_chunks(read_ptr,
                        PNG_HANDLE_CHUNK_NEVER, (png_bytep) NULL, 0);

                   png_set_keep_unknown_chunks(read_ptr,
                        PNG_HANDLE_CHUNK_NEVER, chunks_to_ignore,
                        sizeof (chunks_to_ignore)/5);
                }
#endif

                if (nosave == 0)
                {
#ifdef PNG_USER_MEM_SUPPORTED
                   if (verbose > 0)
                       write_ptr = png_create_write_struct_2(
                         PNG_LIBPNG_VER_STRING,
                         (png_voidp) NULL,
                         (png_error_ptr) pngcrush_cexcept_error,
                         (png_error_ptr) pngcrush_warning,
                         (png_voidp) NULL,
                         (png_malloc_ptr) pngcrush_debug_malloc,
                         (png_free_ptr) pngcrush_debug_free);
                    else
#endif
                       write_ptr = png_create_write_struct(
                         PNG_LIBPNG_VER_STRING,
                         (png_voidp) NULL,
                         (png_error_ptr) pngcrush_cexcept_error,
                         (png_error_ptr) NULL);
                    if (write_ptr == NULL)
                        Throw "pngcrush could not create write_ptr";

                }
                P1("Allocating read_info, write_info, end_info structures\n");
                read_info_ptr = png_create_info_struct(read_ptr);

                if (read_info_ptr == NULL)
                    Throw "pngcrush could not create read_info_ptr";

                end_info_ptr = png_create_info_struct(read_ptr);
                if (end_info_ptr == NULL)
                    Throw "pngcrush could not create end_info_ptr";

                if (nosave == 0)
                {
                    write_info_ptr = png_create_info_struct(write_ptr);
                    if (write_info_ptr == NULL)
                        Throw "pngcrush could not create write_info_ptr";

                    write_end_info_ptr = png_create_info_struct(write_ptr);
                    if (write_end_info_ptr == NULL)
                        Throw
                            "pngcrush could not create write_end_info_ptr";
                }

                P2("structures created.\n");
                pngcrush_pause();

                P1( "Initializing input and output streams\n");
#ifdef PNG_STDIO_SUPPORTED
                png_init_io(read_ptr, fpin);
#else
                png_set_read_fn(read_ptr, (png_voidp) fpin,
                                (png_rw_ptr) NULL);
#endif /* PNG_STDIO_SUPPORTED */

                if (nosave == 0)
                    png_set_write_fn(write_ptr, (png_voidp) fpout,
                        (png_rw_ptr) pngcrush_write_png, pngcrush_flush);

                P2("io has been initialized.\n");
                pngcrush_pause();

#ifdef PNG_CRC_QUIET_USE
                if (check_crc == 0)
                {
                    /* We don't need to check IDAT CRC's and ADLER32 because
                     * they were already checked in the pngcrush_measure_idat
                     * function
                     */
# ifdef PNG_IGNORE_ADLER32
                    png_set_option(read_ptr, PNG_IGNORE_ADLER32,
                        PNG_OPTION_ON);
# endif
                    png_set_crc_action(read_ptr, PNG_CRC_QUIET_USE,
                                       PNG_CRC_QUIET_USE);
                }
#endif

#ifndef PNGCRUSH_CHECK_ADLER32
# ifdef PNG_IGNORE_ADLER32
                if (last_trial == 0)
                {
                    /* During trials other than the final output, avoid
                     * calculating CRC and ADLER32 checksums (just write
                     * a DEFLATE datastream)
                     */
                    png_set_option(write_ptr, PNG_IGNORE_ADLER32,
                        PNG_OPTION_ON);
                }
# endif
#endif

#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
                /* Only run this test (new in libpng-1.5.10) during the
                 * 0th and last trial
                 */
                if ((input_color_type == 3) &&
                    ((last_trial && reduce_palette == 0) ||
                    (trial == 0 && reduce_palette == 1)))
                {
                   P1(" Check the read palette\n");
                   png_set_check_for_invalid_index (read_ptr, 1);
                }
#endif
#ifdef PNG_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED
                if (last_trial && nosave == 0 && output_color_type == 3)
                {
                   P1(" Check the written palette\n");
                   png_set_check_for_invalid_index (write_ptr, 1);
                }
#endif

            if (last_trial == 1)
            {

#ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
                png_set_keep_unknown_chunks(read_ptr, PNG_HANDLE_CHUNK_ALWAYS,
                                            (png_bytep) NULL, 0);
#endif

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
                if (nosave == 0)
                {
                    if (save_apng_chunks == 1)
                    {
                       /* To do: Why use write_ptr not read_ptr here? */
                       png_set_keep_unknown_chunks(write_ptr,
                                                PNG_HANDLE_CHUNK_ALWAYS,
                                                (png_bytep) "acTL", 1);
                       png_set_keep_unknown_chunks(write_ptr,
                                                PNG_HANDLE_CHUNK_ALWAYS,
                                                (png_bytep) "fcTL", 1);
                       png_set_keep_unknown_chunks(write_ptr,
                                                PNG_HANDLE_CHUNK_ALWAYS,
                                                (png_bytep) "fdAT", 1);
                    }

                    if (found_any_chunk == 1)
                       png_set_keep_unknown_chunks(write_ptr,
                                                PNG_HANDLE_CHUNK_ALWAYS,
                                                (png_bytep) "dSIG", 1);
                    if (all_chunks_are_safe)
                    {
                        png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    (png_bytep) NULL, 0);
    printf("while saving, all_chunks_are_safe=\n");
                        if (save_apng_chunks == 0)
                        {
                           png_set_keep_unknown_chunks(write_ptr,
                                                PNG_HANDLE_CHUNK_ALWAYS,
                                                (png_bytep) "acTL", 1);
                           png_set_keep_unknown_chunks(write_ptr,
                                                PNG_HANDLE_CHUNK_ALWAYS,
                                                (png_bytep) "fcTL", 1);
                           png_set_keep_unknown_chunks(write_ptr,
                                                PNG_HANDLE_CHUNK_ALWAYS,
                                                (png_bytep) "fdAT", 1);
                        }
                    }

                    else
                    {
#if !defined(PNG_cHRM_SUPPORTED) || !defined(PNG_hIST_SUPPORTED) || \
    !defined(PNG_iCCP_SUPPORTED) || !defined(PNG_sCAL_SUPPORTED) || \
    !defined(PNG_pCAL_SUPPORTED) || !defined(PNG_sPLT_SUPPORTED) || \
    !defined(PNG_sTER_SUPPORTED) || !defined(PNG_tIME_SUPPORTED)
                        png_byte chunk_name[5];
                        chunk_name[4] = '\0';
#endif

                        /* To do: Why use write_ptr not read_ptr here? */

                        if (keep_unknown_chunk("alla", argv) &&
                            keep_unknown_chunk("allb", argv))
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_IF_SAFE,
                                                    (png_bytep) NULL,
                                                    0);
                        else
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_NEVER,
                                                    (png_bytep) NULL,
                                                    0);

#ifndef PNG_cHRM_SUPPORTED
                        if (keep_unknown_chunk("cHRM", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_cHRM);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_hIST_SUPPORTED
                        if (keep_unknown_chunk("hIST", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_hIST);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_iCCP_SUPPORTED
                        if (keep_unknown_chunk("iCCP", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_iCCP);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_iTXt_SUPPORTED
                        if (keep_unknown_chunk("iTXt", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_iTXt);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_sCAL_SUPPORTED
                        if (keep_unknown_chunk("sCAL", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_sCAL);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_pCAL_SUPPORTED
                        if (keep_unknown_chunk("pCAL", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_pCAL);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_sPLT_SUPPORTED
                        if (keep_unknown_chunk("sPLT", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_sPLT);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_sTER_SUPPORTED
                        if (keep_unknown_chunk("sTER", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_sTER);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
#ifndef PNG_tIME_SUPPORTED
                        if (keep_unknown_chunk("tIME", argv))
                        {
                            pngcrush_save_uint_32(chunk_name, PNG_UINT_tIME);
                            png_set_keep_unknown_chunks(write_ptr,
                                                    PNG_HANDLE_CHUNK_ALWAYS,
                                                    chunk_name, 1);
                        }
#endif
                    }
                } /* nosave == 0 */
#endif /* PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED */
            } /* last trial */

/* To do: Remove this? We already did this in measure_idats */
            P1("   Reading signature bytes\n");
            {
#ifdef PNGCRUSH_LOCO
              png_byte mng_signature[8] =
                  { 138, 77, 78, 71, 13, 10, 26, 10 };
#endif
              png_byte png_signature[8] =
                  { 137, 80, 78, 71, 13, 10, 26, 10 };

              pngcrush_default_read_data(read_ptr, png_signature, 8);
              png_set_sig_bytes(read_ptr, 8);

#ifdef PNGCRUSH_LOCO
              if (!(int)(png_memcmp(mng_signature, png_signature, 8)))
              {
                  /* Skip the MHDR */
                  png_permit_mng_features(read_ptr,
                                      PNG_FLAG_MNG_FILTER_64);
                  png_skip_chunk(read_ptr);
                  input_format = 1;
              }

              else
#endif
              if (png_sig_cmp(png_signature, 0, 8))
              {
                  if (png_sig_cmp(png_signature, 0, 4))
                      png_error(read_ptr, "Not a PNG file!");
              else
                  png_error(read_ptr,
                      "PNG file corrupted by ASCII conversion");
              }
              if (salvage && found_CgBI)
              {
                  /* Skip the CgBI chunk */

                  png_skip_chunk(read_ptr);

                  /* iCCP and zTXt are probably unreadable
                   * because of the nonstandard deflate */

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
                  png_set_keep_unknown_chunks(read_ptr,
                      PNG_HANDLE_CHUNK_NEVER,
                      (png_bytep)"iCCP", 1);
                  png_set_keep_unknown_chunks(read_ptr,
                      PNG_HANDLE_CHUNK_NEVER,
                      (png_bytep)"zTXt", 1);
#endif
              }
            }

#ifndef PNG_READ_PREMULTIPLY_ALPHA_SUPPORTED
            if (premultiply)
                 png_error(read_ptr, "Premultiplied alpha is not supported");
#endif

            P1( "   Reading info struct\n");
            png_read_info(read_ptr, read_info_ptr);

            if (trial != 0)
            {
               if (make_opaque == 1)
               {
                  P1(" Remove all-opaque alpha channel\n");
                  if (output_color_type == 4)
                     output_color_type = 0;
                  if (output_color_type == 6)
                     output_color_type = 2;
               }
               P1(" make_opaque=    %d\n",make_opaque);

               if (make_gray == 1)
               {
                  /* Note: Take care that iCCP, sBIT, and bKGD data are not
                   * lost or become invalid when reducing images from
                   * truecolor to grayscale or when reducing the bit depth.
                   * (To do: test this; it's probably OK)
                   */
                  /* To do: check bKGD color */

                  P1(" Encode all-gray image with a gray colortype\n");
                  if (output_color_type == 6)
                     output_color_type = 4;
                  if (output_color_type == 2)
                     output_color_type = 0;
               }
               P1(" make_gray=      %d\n",make_gray);

               if (make_8_bit == 1)
               {
                  /* Note: Take care that sBIT and bKGD data are not
                   * lost or become invalid when reducing the bit depth.
                   * (To do: test this; it's probably OK)
                   */
                  P1(" Reduce 16-bit image losslessly to 8-bit\n");
               }
               P1(" make_8_bit=     %d\n",make_8_bit);

               if (make_opaque != 1 && blacken == 2)
               {
                  P1(" Blacken the fully transparent pixels\n");
                  png_set_read_user_transform_fn(read_ptr,
                      pngcrush_transform_pixels_fn);
               }
               P1(" make_opaque=    %d\n",make_opaque);
               P1(" blacken=        %d\n",blacken);

               if (reduce_palette == 1)
               {
                  /* Note: Take care that sBIT and bKGD data are not
                   * lost or become invalid when reducing the bit depth.
                   * (To do: test this; it's probably OK)
                   */
                  P1(" Reduce palette by truncating unused entries\n");
               }
               P1(" reduce_palette= %d\n",reduce_palette);
               P1("  new plte_len = %d\n",plte_len);
            }

            /* { GRR added for quick %-navigation (1) */

            /* Start of chunk-copying/removal code, in order:
             *  - IHDR
             *  - bKGD
             *  - cHRM
             *  - gAMA
             *  - sRGB
             *  - iCCP
             *  - oFFs
             *  - pCAL
             *  - pHYs
             *  - hIST
             *  - tRNS
             *  - PLTE
             *  - sBIT
             *  - sCAL
             *  - sPLT
             *  - sTER
             *  - tEXt/zTXt/iTXt
             *  - tIME
             *  - unknown chunks
             */
            {
                int compression_method,
                    filter_method;

                P1( "Transferring info struct\n");

                if (png_get_IHDR(read_ptr, read_info_ptr, &width, &height,
                     &bit_depth, &color_type, &interlace_method,
                     &compression_method, &filter_method))
                {
                    int need_expand = 0;
                    input_color_type = color_type;
                    input_bit_depth = bit_depth;

                    if (output_color_type > 7)
                    {
                        output_color_type = input_color_type;
                    }

                    /* if (verbose > 1 && last_trial) */
                    if (verbose > 1 && trial == 0)
                    {
                        fprintf(STDERR, "   IHDR chunk data:\n");
                        fprintf(STDERR,
                                "      Width=%lu, height=%lu\n",
                                (unsigned long)width,
                                (unsigned long)height);
                        fprintf(STDERR, "      Bit depth =%d\n",
                                bit_depth);
                        fprintf(STDERR, "      Color type=%d\n",
                                color_type);
                        if (output_color_type != color_type)
                            fprintf(STDERR,
                                    "      Output color type=%d\n",
                                    output_color_type);
                        fprintf(STDERR, "      Interlace =%d\n",
                                interlace_method);
                    }

                    output_bit_depth = input_bit_depth;

                    if ((output_color_type != 3 || output_bit_depth > 8)
                       && output_bit_depth >= 8
                       && output_bit_depth > input_bit_depth)
                      need_expand = 1;

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
                    if ((color_type == 2 ||
                         color_type == 6 ||
                         color_type == 3) &&
                         (output_color_type == 0 ||
                         output_color_type == 4))
                    {
                        if (verbose > 0 && last_trial)
                        {
                                fprintf(STDERR, "   Reducing truecolor "
                                  "image to grayscale.\n");
                        }
#ifdef PNG_FIXED_POINT_SUPPORTED
                        png_set_rgb_to_gray_fixed(read_ptr, 1,
                           21260, 71520);
#else
                        png_set_rgb_to_gray(read_ptr, 1,
                           0.21260, 0.71520);
#endif
                        if (output_bit_depth < 8)
                            output_bit_depth = 8;
                        if (color_type == 3)
                            need_expand = 1;
                    }
#endif /* PNG_READ_RGB_TO_GRAY_SUPPORTED */

                    if (color_type != 3 && output_color_type == 3)
                    {
                        fprintf(STDERR,"  Cannot change to indexed color "
                          "(color_type 3)\n");
                        output_color_type = input_color_type;
                    }

                    if ((color_type == 0 || color_type == 4) &&
                        (output_color_type == 2
                         || output_color_type == 6))
                    {
                        png_set_gray_to_rgb(read_ptr);
                    }

                    if ((color_type == 4 || color_type == 6) &&
                        (output_color_type != 4
                         && output_color_type != 6))
                    {
                        if (verbose > 0 && last_trial)
                        {
                            fprintf(STDERR,
                              "   Stripping existing alpha channel.\n");
                        }
#ifdef PNG_READ_STRIP_ALPHA_SUPPORTED
                        png_set_strip_alpha(read_ptr);
#endif
                    }

                    if ((output_color_type == 4
                         || output_color_type == 6) && (color_type != 4
                                                        && color_type != 6))
                    {
                        if (verbose > 0 && last_trial)
                            fprintf(STDERR,
                              "   Adding an opaque alpha channel.\n");
#ifdef PNG_READ_FILLER_SUPPORTED
                        png_set_filler(read_ptr, (png_uint_32) 65535L,
                                       PNG_FILLER_AFTER);
#endif
                        need_expand = 1;
                    }

                    if (output_color_type != 0 && output_color_type != 3 &&
                        output_bit_depth < 8)
                       output_bit_depth = 8;

                    if ((output_color_type == 2
                         || output_color_type == 6)
                        && color_type == 3)
                    {
                        if (verbose > 0 && last_trial)
                            fprintf(STDERR,
                              "   Expanding indexed color file.\n");
                        need_expand = 1;
                    }
#ifdef PNG_READ_EXPAND_SUPPORTED
                    if (need_expand == 1)
                        png_set_expand(read_ptr);
#endif

#ifdef PNG_READ_PACK_SUPPORTED
                    if (input_bit_depth < 8)
                    {
                        png_set_packing(read_ptr);
                    }

                    if (output_color_type == 0 && output_bit_depth < 8)
                    {
                        png_color_8 true_bits;
                        true_bits.gray = (png_byte) (output_bit_depth);
                        png_set_shift(read_ptr, &true_bits);
                    }
#endif
 
                    if (trial > 0)
                    {
                    /* To do: have we got the right plte_len now? */
                      if (plte_len > 0 && output_color_type == 3 &&
                          force_output_bit_depth == 0)
                      {
                        if (plte_len <= 2)
                          force_output_bit_depth = 1;
                        else if (plte_len <= 4)
                          force_output_bit_depth = 2;
                        else if (plte_len <= 16)
                          force_output_bit_depth = 4;
                        else
                          force_output_bit_depth = 8;
                      }

                    if (make_8_bit == 1)
                    {
#if defined(PNG_READ_SCALE_16_TO_8_SUPPORTED) || \
defined(PNG_READ_STRIP_16_TO_8_SUPPORTED)
                       output_bit_depth = 8;
                       force_output_bit_depth = 8;
                       if (verbose > 0 && last_trial)
                         fprintf(STDERR,
                         "   Stripping 16-bit depth to 8, trial = %d\n",
                         trial);

#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
                       png_set_strip_16(read_ptr);
#else
#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
                       png_set_scale_16(read_ptr);
#else
                       fprintf(STDERR,
                       "   PNG_READ_STRIP_16_TO_8 NOT SUPPORTED\n");
#endif
#endif
#endif
                    }
                      P1("force_output_bit_depth=%d\n",force_output_bit_depth);
                    }


                    if (last_trial == 1)
                    {
                    if (save_apng_chunks == 1 || found_acTL_chunk == 1)
                    {
                       if (save_apng_chunks == 0)
                       {
                          if (verbose > 0) fprintf(STDERR,
                          "   pngcrush will only save APNG chunks in an\n");
                          if (verbose > 0) fprintf(STDERR,
                          "   output file with the \".apng\" extension\n");
                       }
                       if (input_color_type != output_color_type)
                       {
                          if (verbose > 0) fprintf(STDERR,
                          "   Cannot save APNG chunks with a color_type\n");
                          if (verbose > 0) fprintf(STDERR,
                          "   different from that of the main image.\n");
                          save_apng_chunks = 2;
                       }
                       if (input_bit_depth != output_bit_depth)
                       {
                          if (verbose > 0) fprintf(STDERR,
                          "   Cannot save APNG chunks with a bit_depth\n");
                          if (verbose > 0) fprintf(STDERR,
                          "   different from that of the main image.\n");
                          save_apng_chunks = 2;
                       }
                       if (save_apng_chunks != 1 && found_acTL_chunk == 1)
                          found_acTL_chunk = 2;
                    }
                    }

                    if (verbose > 1)
                        fprintf(STDERR, "   Setting IHDR\n");

#ifdef PNGCRUSH_LOCO
                    output_format = 0;
                    if (do_loco)
                    {
                        if (output_color_type == 2
                            || output_color_type == 6)
                        {
                            output_format = 1;
                            filter_method = 64;
                            if (nosave == 0 && last_trial == 1)
                               png_permit_mng_features(write_ptr,
                                   PNG_FLAG_MNG_FILTER_64);

                        }
                    } else
                        filter_method = 0;
#endif

                    png_set_IHDR(write_ptr, write_info_ptr, width,
                                 height, output_bit_depth,
                                 output_color_type, interlace_method,
                                 compression_method, filter_method);

                } /* IHDR */
            }

            if (premultiply == 1 || premultiply == 2)
            {

#ifdef PNG_READ_PREMULTIPLY_ALPHA_SUPPORTED
              /* 0: not premultipled
               * 1: premultiplied input (input has .pngp suffix and the
               *    PNGP chunk is present)
               * 2: premultiplied output (output has .pngp suffix and the
               *    -premultiply option is present; PNGP chunk is added)
               * 3: premultiplied input and output (both have .pngp suffix)
               */
               P1("Calling png_set_premultiply_alpha\n");
               png_set_premultiply_alpha(read_ptr,output_bit_depth);
#endif
            }


            /* Handle ancillary chunks */
            if (last_trial == 1)
            {
#if defined(PNG_READ_bKGD_SUPPORTED) && defined(PNG_WRITE_bKGD_SUPPORTED)
                {
                    png_color_16p background;
                    if (!have_bkgd
                        && png_get_bKGD(read_ptr, read_info_ptr,
                                        &background))
                    {
                        if (keep_chunk("bKGD", argv))
                        {
                            if ((input_color_type == 2 ||
                                input_color_type == 6) &&
                                (output_color_type == 0 ||
                                output_color_type == 4))
                              background->gray = background->green;

                            png_set_bKGD(write_ptr, write_info_ptr,
                                         background);
                        }
                    }
                    if (have_bkgd)
                    {
                        /* If we are reducing an RGB image to grayscale, but
                         * the background color isn't gray, the green channel
                         * is written.  That's not spec-compliant.  We should
                         * really check for a non-gray bKGD and refuse to do
                         * the reduction if one is present.
                         */
                        png_color_16 backgd;
                        png_color_16p backgrnd = &backgd;
                        backgrnd->red = bkgd_red;
                        backgrnd->green = bkgd_green;
                        backgrnd->blue = bkgd_blue;
                        backgrnd->gray = backgrnd->green;
                        backgrnd->index = bkgd_index;
                        png_set_bKGD(write_ptr, write_info_ptr,
                                     backgrnd);
                    }
                }
#endif /* PNG_READ_bKGD_SUPPORTED && PNG_WRITE_bKGD_SUPPORTED */

#if defined(PNG_READ_cHRM_SUPPORTED) && defined(PNG_WRITE_cHRM_SUPPORTED)
#ifdef PNG_FIXED_POINT_SUPPORTED
                {
                    png_fixed_point white_x, white_y, red_x, red_y,
                        green_x, green_y, blue_x, blue_y;

                    if (found_cHRM && png_get_cHRM_fixed
                        (read_ptr, read_info_ptr, &white_x, &white_y,
                         &red_x, &red_y, &green_x, &green_y, &blue_x,
                         &blue_y))
                        {
                          if (keep_chunk("cHRM", argv))
                          {
                                png_set_cHRM_fixed(write_ptr,
                                                   write_info_ptr, white_x,
                                                   white_y, red_x, red_y,
                                                   green_x, green_y,
                                                   blue_x, blue_y);
                          }
                        }
                }
#else
                {
                    double white_x, white_y, red_x, red_y, green_x,
                        green_y, blue_x, blue_y;

                    if (found_cHRM && png_get_cHRM
                        (read_ptr, read_info_ptr, &white_x, &white_y,
                         &red_x, &red_y, &green_x, &green_y, &blue_x,
                         &blue_y)) {
                        if (keep_chunk("cHRM", argv))
                        {
                                png_set_cHRM(write_ptr, write_info_ptr,
                                             white_x, white_y, red_x,
                                             red_y, green_x, green_y,
                                             blue_x, blue_y);
                        }
                    }
                }
#endif /* PNG_FIXED_POINT_SUPPORTED */
#endif /* PNG_READ_cHRM_SUPPORTED && PNG_WRITE_cHRM_SUPPORTED */

#if defined(PNG_READ_gAMA_SUPPORTED) && defined(PNG_WRITE_gAMA_SUPPORTED)
                {
                    if (force_specified_gamma)
                    {
                        if (last_trial)
                        {
                            if (verbose > 0)
                                fprintf(STDERR, "   Inserting gAMA chunk with "
#ifdef PNG_FIXED_POINT_SUPPORTED
                                  "gamma=(%d/100000)\n",
#else
                                  "gamma=%f\n",
#endif
                                  force_specified_gamma);
                        }
#ifdef PNG_FIXED_POINT_SUPPORTED
                        png_set_gAMA_fixed(write_ptr, write_info_ptr,
                                           (png_fixed_point)
                                           force_specified_gamma);
                        file_gamma =
                            (png_fixed_point) force_specified_gamma;
#else
                        png_set_gAMA(write_ptr, write_info_ptr,
                                     force_specified_gamma);
                        file_gamma = force_specified_gamma;
#endif
                    }
#ifdef PNG_FIXED_POINT_SUPPORTED
                    else if (found_gAMA && png_get_gAMA_fixed
                             (read_ptr, read_info_ptr, &file_gamma))
#else
                    else if (found_gAMA && png_get_gAMA
                             (read_ptr, read_info_ptr, &file_gamma))
#endif
                    {
                        if (keep_chunk("gAMA", argv))
                        {
                            if (image_specified_gamma)
                                file_gamma = image_specified_gamma;
                            if (verbose > 1 && last_trial)
#ifdef PNG_FIXED_POINT_SUPPORTED
                                fprintf(STDERR, "   gamma=(%d/100000)\n",
                                        (int) file_gamma);
                            if (double_gamma)
                                file_gamma += file_gamma;
                            png_set_gAMA_fixed(write_ptr, write_info_ptr,
                                               file_gamma);
#else
                                fprintf(STDERR, "   gamma=%f\n",
                                        file_gamma);
                            if (double_gamma)
                                file_gamma += file_gamma;
                            png_set_gAMA(write_ptr, write_info_ptr,
                                         file_gamma);
#endif
                        }
                    }
                    else if (specified_gamma)
                    {
                        if (last_trial)
                        {
                            if (verbose > 0)
                                fprintf(STDERR, "   Inserting gAMA chunk with "
#ifdef PNG_FIXED_POINT_SUPPORTED
                                  "gamma=(%d/100000)\n",
#else
                                  "gamma=%f\n",
#endif
                                  specified_gamma);
                        }
#ifdef PNG_FIXED_POINT_SUPPORTED
                        png_set_gAMA_fixed(write_ptr, write_info_ptr,
                                           specified_gamma);
                        file_gamma = (png_fixed_point) specified_gamma;
#else
                        png_set_gAMA(write_ptr, write_info_ptr,
                                     specified_gamma);
                        file_gamma = specified_gamma;
#endif
                    }
                }
#endif /* PNG_READ_gAMA_SUPPORTED && PNG_WRITE_gAMA_SUPPORTED */

#if defined(PNG_READ_sRGB_SUPPORTED) && defined(PNG_WRITE_sRGB_SUPPORTED)
                {
                    int file_intent;

                    if (png_get_sRGB
                        (read_ptr, read_info_ptr, &file_intent))
                    {
                        if (keep_chunk("sRGB", argv))
                        {
                            png_set_sRGB(write_ptr, write_info_ptr,
                                         file_intent);
                            intent = file_intent;
                        }
                    }
                    else if (found_sRGB)
                    {
#ifdef PNG_gAMA_SUPPORTED
#  ifdef PNG_FIXED_POINT_SUPPORTED
                        if (file_gamma >= 45000L && file_gamma <= 46000L)
#  else
                        if (file_gamma >= 0.45000 && file_gamma <= 0.46000)
#  endif
                        {
                            if (verbose > 0 && last_trial)
                                fprintf(STDERR,
                                  "   Inserting sRGB chunk with intent=%d\n",
                                  intent);
                            png_set_sRGB(write_ptr, write_info_ptr,
                                         intent);
                        }
                        else if (file_gamma != 0)
                        {
                            if (verbose > 0 && last_trial)
                            {
                                fprintf(STDERR, "   Ignoring sRGB request; "
#  ifdef PNG_FIXED_POINT_SUPPORTED
                                  "gamma=(%lu/100000)"
                                  " is not approx. 0.455\n",
                                  (unsigned long)file_gamma);
#  else
                                  "gamma=%f"
                                  " is not approx. 0.455\n",
                                  file_gamma);
#  endif
                            }
                        }
#endif /* PNG_gAMA_SUPPORTED */
                    }
                }
#endif /* PNG_READ_sRGB_SUPPORTED && PNG_WRITE_sRGB_SUPPORTED */

#if defined(PNG_READ_iCCP_SUPPORTED) && defined(PNG_WRITE_iCCP_SUPPORTED)
                if (intent < 0) {  /* ignore iCCP if sRGB is being written */
                    png_charp name;
#if PNGCRUSH_LIBPNG_VER < 10500
                    png_charp profile;
#else
                    png_bytep profile;
#endif
                    png_uint_32 proflen;
                    int compression_method;

                    if (found_iCCP && png_get_iCCP
                        (read_ptr, read_info_ptr, &name,
                         &compression_method, &profile, &proflen))
                    {
                        P1("Got iCCP chunk, proflen=%lu\n",
                            (unsigned long)proflen);
                        if (iccp_length)
                            P0("Will not replace existing iCCP chunk.\n");
                        if (keep_chunk("iCCP", argv))
                            png_set_iCCP(write_ptr, write_info_ptr,
#if PNGCRUSH_LIBPNG_VER < 10500
                                                           name,
#else
                                         (png_const_charp) name,
#endif
                                         compression_method, profile,
                                         proflen);

                    }
#ifdef PNG_iCCP_SUPPORTED
                    else if (iccp_length)
                    {
                        png_set_iCCP(write_ptr, write_info_ptr, iccp_name,
                                     0,
#if PNGCRUSH_LIBPNG_VER < 10500
                                     iccp_text,
#else
                                     (png_const_bytep) iccp_text,
#endif
                                     iccp_length);
                        P1("Wrote iCCP chunk, proflen=%d\n", iccp_length);
                    }
#endif /* PNG_iCCP_SUPPORTED */

                }
#endif /* PNG_READ_iCCP_SUPPORTED && PNG_WRITE_iCCP_SUPPORTED */

#if defined(PNG_READ_oFFs_SUPPORTED) && defined(PNG_WRITE_oFFs_SUPPORTED)
                {
                    png_int_32 offset_x, offset_y;
                    int unit_type;

                    if (png_get_oFFs
                        (read_ptr, read_info_ptr, &offset_x, &offset_y,
                         &unit_type)) {
                        if (offset_x == 0 && offset_y == 0)
                        {
                            if (verbose > 0 && last_trial)
                                fprintf(STDERR,
                                  "   Deleting useless oFFs 0 0 chunk\n");
                        }
                        else
                        {
                            if (keep_chunk("oFFs", argv))
                                png_set_oFFs(write_ptr, write_info_ptr,
                                             offset_x, offset_y,
                                             unit_type);
                        }
                    }
                }
#endif /* PNG_READ_oFFs_SUPPORTED && PNG_WRITE_oFFs_SUPPORTED */

#if defined(PNG_READ_pCAL_SUPPORTED) && defined(PNG_WRITE_pCAL_SUPPORTED)
                {
                    png_charp purpose, units;
                    png_charpp params;
                    png_int_32 X0, X1;
                    int type, nparams;

                    if (png_get_pCAL
                        (read_ptr, read_info_ptr, &purpose, &X0, &X1,
                         &type, &nparams, &units, &params))
                    {
                        if (keep_chunk("pCAL", argv))
                            png_set_pCAL(write_ptr, write_info_ptr,
                                         purpose, X0, X1, type, nparams,
                                         units, params);
                    }
                }
#endif /* pCAL_SUPPORTED */

#if defined(PNG_READ_pHYs_SUPPORTED) && defined(PNG_WRITE_pHYs_SUPPORTED)
                {
                    png_uint_32 res_x, res_y;
                    int unit_type;

                    if (resolution == 0)
                    {
                        if (png_get_pHYs
                            (read_ptr, read_info_ptr, &res_x, &res_y,
                             &unit_type))
                        {
                            if (res_x == 0 && res_y == 0)
                            {
                                if (verbose > 0 && last_trial)
                                    fprintf(STDERR,
                                      "   Deleting useless pHYs 0 0 chunk\n");
                            }
                            else
                            {
                                if (keep_chunk("pHYs", argv))
                                    png_set_pHYs(write_ptr, write_info_ptr,
                                                 res_x, res_y, unit_type);
                            }
                        }
                    } else {
                        unit_type = 1;
                        res_x = res_y =
                            (png_uint_32) ((resolution / .0254 + 0.5));
                        png_set_pHYs(write_ptr, write_info_ptr, res_x,
                                     res_y, unit_type);
                        if (verbose > 0 && last_trial)
                            fprintf(STDERR, "   Added pHYs %lu %lu 1 chunk\n",
                            (unsigned long)res_x,
                            (unsigned long)res_y);
                    }
                }
#endif

#if defined(PNG_READ_hIST_SUPPORTED) && defined(PNG_WRITE_hIST_SUPPORTED)
                {
                    png_uint_16p hist;

                    if (png_get_hIST(read_ptr, read_info_ptr, &hist))
                    {
                        if (keep_chunk("hIST", argv))
                            png_set_hIST(write_ptr, write_info_ptr, hist);
                    }
                }
#endif /* hIST_SUPPORTED */

#if defined(PNG_READ_tRNS_SUPPORTED) && defined(PNG_WRITE_tRNS_SUPPORTED)
                {
                    if (png_get_tRNS
                        (read_ptr, read_info_ptr, &trans, &num_trans,
                         &trans_values))
                    {
                        if (verbose > 1 && last_trial)
                            fprintf(STDERR,
                              "  Found tRNS chunk in input file.\n");
                        if (have_trns == 1)
                        {
                            P0("  Will not overwrite existing tRNS chunk.\n");
                        }
                        if (keep_chunk("tRNS", argv))
                        {
                            int last_nonmax = -1;
                            trns_red = trans_values->red;
                            trns_green = trans_values->green;
                            trns_blue = trans_values->blue;
                            trns_gray = trans_values->gray;
                            if (output_color_type == 3)
                            {
                                for (ia = 0; ia < num_trans; ia++)
                                    trns_array[ia] = trans[ia];
                                for (; ia < 256; ia++)
                                    trns_array[ia] = 255;
                                for (ia = 0; ia < 256; ia++)
                                {
                                    if (trns_array[ia] != 255)
                                        last_nonmax = ia;
                                }
                                if (last_trial && verbose > 0)
                                {
                                    if (last_nonmax < 0)
                                        fprintf(STDERR, "   Deleting "
                                          "all-opaque tRNS chunk.\n");
                                    else if (last_nonmax + 1 < num_trans)
                                        fprintf(STDERR,
                                          "   Truncating trailing opaque "
                                          "entries from tRNS chunk.\n");
                                }
                                num_trans = last_nonmax + 1;
                            }
                            else
                            {
                               /* color type is 0 or 2 */
                               if (input_bit_depth == 16 &&
                                   output_bit_depth == 8)
                               {
                                 /* zero out the high byte */
                                 trans_values->red &= 0x00ff;
                                 trans_values->green &= 0x00ff;
                                 trans_values->blue &= 0x00ff;
                                 trans_values->gray &= 0x00ff;
                               }

                            }
                            if (verbose > 1)
                                fprintf(STDERR,
                                  "   png_set_tRNS, num_trans=%d\n",
                                  num_trans);
                            if (output_color_type < 3 || num_trans)
                                png_set_tRNS(write_ptr, write_info_ptr,
                                             trans, num_trans,
                                             trans_values);
                        }
                    }
                    else if (have_trns == 1)
                    {
                        /* will not overwrite existing trns data */
                        png_color_16 trans_data;
                        png_byte index_data = (png_byte) trns_index;
                        num_trans = index_data + 1;
                        if (verbose > 1)
                            fprintf(STDERR, "Have_tRNS, num_trans=%d\n",
                                    num_trans);
                        if (output_color_type == 3)
                        {
                            trans_values = NULL;
                            for (ia = 0; ia < num_trans; ia++)
                                trns_array[ia] = trans_in[ia];
                            for (; ia < 256; ia++)
                                trns_array[ia] = 255;
                        } else {
                            for (ia = 0; ia < 256; ia++)
                                trns_array[ia] = 255;
                            trns_array[index_data] = 0;

                            trans_data.index = index_data;
                            trans_data.red = trns_red;
                            trans_data.green = trns_green;
                            trans_data.blue = trns_blue;
                            trans_data.gray = trns_gray;
                            trans_values = &trans_data;
                        }

                        P0("  Adding a tRNS chunk\n");
                        png_set_tRNS(write_ptr, write_info_ptr, trns_array,
                                     num_trans, trans_values);
                    }
                    else
                    {
                        for (ia = 0; ia < 256; ia++)
                            trns_array[ia] = 255;
                    }
                    if (verbose > 1 && last_trial)
                    {
                        int last = -1;
                        for (ia = 0; ia < num_palette; ia++)
                            if (trns_array[ia] != 255)
                                last = ia;
                        if (last >= 0) {
                            fprintf(STDERR, "   Transparency:\n");
                            if (output_color_type == 3)
                                for (ia = 0; ia < num_palette; ia++)
                                    fprintf(STDERR, "      %4d %4d\n", ia,
                                            trns_array[ia]);
                            else if (output_color_type == 0)
                                fprintf(STDERR, "      %d\n", trns_gray);
                            else if (output_color_type == 2)
                                fprintf(STDERR, "      %d %d %d\n",
                                        trns_red, trns_green, trns_blue);
                        }
                    }
                }
#endif /* PNG_READ_tRNS_SUPPORTED && PNG_WRITE_tRNS_SUPPORTED */
            }  /* End of ancillary chunk handling */

                if (png_get_PLTE
                    (read_ptr, read_info_ptr, &palette, &num_palette))
                {
                    if (plte_len > 0)
                        num_palette = plte_len;
                    if (do_pplt)
                    {
                        fprintf(STDERR,"PPLT: %s\n", pplt_string);
                        fprintf(STDERR,"Sorry, PPLT is not implemented yet.\n");
                    }

                    if (nosave == 0)
                    {
                    if (output_color_type == 3)
                        png_set_PLTE(write_ptr, write_info_ptr, palette,
                                     num_palette);
                    else if (keep_chunk("PLTE", argv))
                        png_set_PLTE(write_ptr, write_info_ptr, palette,
                                     num_palette);

                    }
                    if (verbose > 1 && last_trial)
                    {
                        png_colorp p = palette;
                        fprintf(STDERR, "   Palette:\n");
                        fprintf(STDERR,
                          "      I    R    G    B ( color )    A\n");
                        for (i = 0; i < num_palette; i++)
                        {
                            fprintf(STDERR,
                              "   %4d %4d %4d %4d (#%2.2x%2.2x%2.2x) %4d\n",
                              i, p->red, p->green, p->blue, p->red,
                              p->green, p->blue, trns_array[i]);
                            p++;
                        }
                    }
                }


            /* Handle ancillary chunks */
            if (last_trial == 1)
            {
#if defined(PNG_READ_sBIT_SUPPORTED) && defined(PNG_WRITE_sBIT_SUPPORTED)
                {
                    png_color_8p sig_bit;

                    /* If we are reducing a truecolor PNG to grayscale, and the
                     * RGB sBIT values aren't identical, we'll lose sBIT info.
                     */
                    if (png_get_sBIT(read_ptr, read_info_ptr, &sig_bit))
                    {
                        if (keep_chunk("sBIT", argv))
                        {
                            if ((input_color_type == 0
                                 || input_color_type == 4)
                                && (output_color_type == 2
                                    || output_color_type == 6
                                    || output_color_type == 3))
                                sig_bit->red = sig_bit->green =
                                    sig_bit->blue = sig_bit->gray;
                            if ((input_color_type == 2
                                 || input_color_type == 6
                                 || output_color_type == 3)
                                && (output_color_type == 0
                                    || output_color_type == 4))
                                sig_bit->gray = sig_bit->green;

                            if ((input_color_type == 0
                                 || input_color_type == 2)
                                && (output_color_type == 4
                                    || output_color_type == 6))
                                sig_bit->alpha = 1;

                            if (nosave == 0)
                               png_set_sBIT(write_ptr, write_info_ptr,
                                         sig_bit);
                        }
                    }
                }
#endif /* PNG_READ_sBIT_SUPPORTED)&& PNG_WRITE_sBIT_SUPPORTED */

#ifdef PNG_sCAL_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED
                {
                    int unit;
                    double scal_width, scal_height;

                    if (png_get_sCAL
                        (read_ptr, read_info_ptr, &unit, &scal_width,
                         &scal_height))
                    {
                        png_set_sCAL(write_ptr, write_info_ptr, unit,
                                     scal_width, scal_height);
                    }
                }
#  else
#    ifdef PNG_FIXED_POINT_SUPPORTED
                {
                    int unit;
                    png_charp scal_width, scal_height;

                    if (png_get_sCAL_s
                        (read_ptr, read_info_ptr, &unit, &scal_width,
                         &scal_height))
                    {
                        if (keep_chunk("sCAL", argv))
                            png_set_sCAL_s(write_ptr, write_info_ptr, unit,
                                           scal_width, scal_height);
                    }
                }
#    endif
#  endif /* PNG_FLOATING_POINT_SUPPORTED */
#endif /* PNG_sCAL_SUPPORTED */

#ifdef PNG_sPLT_SUPPORTED
                {
                    png_sPLT_tp entries;
                    int num_entries;

                    num_entries =
                        (int) png_get_sPLT(read_ptr, read_info_ptr,
                                           &entries);
                    if (num_entries)
                    {
                        if (keep_chunk("sPLT", argv))
                            png_set_sPLT(write_ptr, write_info_ptr,
                                         entries, num_entries);
                    }
                }
#endif

#ifdef PNG_TEXT_SUPPORTED
                {
                    png_textp text_ptr;
                    int num_text = 0;

                    if (png_get_text
                        (read_ptr, read_info_ptr, &text_ptr, &num_text) > 0
                        || text_inputs)
                    {
                        int ntext;
                        P1( "Handling %d tEXt/zTXt chunks before IDAT\n",
                                   num_text);

                        if (verbose > 1 && last_trial && num_text > 0)
                        {
                            for (ntext = 0; ntext < num_text; ntext++)
                            {
                                fprintf(STDERR, "%d  %s", ntext,
                                        text_ptr[ntext].key);
                                if (text_ptr[ntext].text_length)
                                    fprintf(STDERR, ": %s\n",
                                            text_ptr[ntext].text);
#ifdef PNG_iTXt_SUPPORTED
                                else if (text_ptr[ntext].itxt_length)
                                {
                                    fprintf(STDERR, " (%s: %s): \n",
                                            text_ptr[ntext].lang,
                                            text_ptr[ntext].lang_key);
                                    fprintf(STDERR, "%s\n",
                                            text_ptr[ntext].text);
                                }
#endif
                                else
                                    fprintf(STDERR, "\n");
                            }
                        }

                        if (num_text > 0)
                        {
                            if (keep_chunk("text", argv))
                            {
                                int num_to_write = num_text;
                                for (ntext = 0; ntext < num_text; ntext++)
                                {
                                    if (last_trial)
                                        P2("Text chunk before IDAT, "
                                          "compression=%d\n",
                                          text_ptr[ntext].compression);
                                    if (text_ptr[ntext].compression ==
                                        PNG_TEXT_COMPRESSION_NONE) {
                                        if (!keep_chunk("tEXt", argv))
                                        {
                                            text_ptr[ntext].key[0] = '\0';
                                            num_to_write--;
                                        }
                                    }
                                    if (text_ptr[ntext].compression ==
                                        PNG_TEXT_COMPRESSION_zTXt)
                                    {
                                        if (!keep_chunk("zTXt", argv))
                                        {
                                            text_ptr[ntext].key[0] = '\0';
                                            num_to_write--;
                                        }
                                    }
#ifdef PNG_iTXt_SUPPORTED
                                    if (text_ptr[ntext].compression ==
                                        PNG_ITXT_COMPRESSION_NONE
                                        || text_ptr[ntext].compression ==
                                        PNG_ITXT_COMPRESSION_zTXt)
                                    {
                                        if (!keep_chunk("iTXt", argv))
                                        {
                                            text_ptr[ntext].key[0] = '\0';
                                            num_to_write--;
                                        }
                                    }
#endif
                                }
                                if (num_to_write > 0)
                                    png_set_text(write_ptr, write_info_ptr,
                                                 text_ptr, num_text);
                            }
                        }
                        for (ntext = 0; ntext < text_inputs; ntext++)
                        {
                            if (text_where[ntext] == 1)
                            {
                                png_textp added_text;
                                added_text = (png_textp) png_malloc(write_ptr,
                                               (png_uint_32) sizeof(png_text));
                                added_text[0].key = &text_keyword[ntext * 80];
#ifdef PNG_iTXt_SUPPORTED
                                added_text[0].lang = &text_lang[ntext * 80];
                                added_text[0].lang_key =
                                    &text_lang_key[ntext * 80];
#endif
                                added_text[0].text = 
                                    &text_text[ntext * STR_BUF_SIZE];
                                added_text[0].compression =
                                    text_compression[ntext];
                                png_set_text(write_ptr, write_info_ptr,
                                             added_text, 1);
                                if (verbose > 0 && last_trial)
                                {
                                  if (added_text[0].compression < 0)
                                      fprintf(STDERR,
                                         "   Added a tEXt chunk.\n");
                                  else if (added_text[0].compression == 0)
                                      fprintf(STDERR,
                                         "   Added a zTXt chunk.\n");
#ifdef PNG_iTXt_SUPPORTED
                                  else
                                      fprintf(STDERR,
                                         "   Added a%scompressed iTXt chunk"
                                        ".\n", (added_text[0].compression == 1)?
                                        "n un" : " ");
#endif
                                }
                                png_free(write_ptr, added_text);
                                added_text = (png_textp) NULL;
                            }
                        }
                    }
                }
#endif /* defined(PNG_TEXT_SUPPORTED) */

#if defined(PNG_READ_tIME_SUPPORTED) && defined(PNG_WRITE_tIME_SUPPORTED)
                {
                    png_timep mod_time;

                    if (png_get_tIME(read_ptr, read_info_ptr, &mod_time))
                    {
                        if (keep_chunk("tIME", argv))
                            png_set_tIME(write_ptr, write_info_ptr, mod_time);
                    }
                }
#endif

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
                /* This section handles pCAL and tIME (at least, in default
                 * build), gIFx/gIFg/gIFt, private Fireworks chunks, etc. */
                {
                    png_unknown_chunkp unknowns;   /* allocated by libpng */
                    int num_unknowns;

                    num_unknowns = (int)png_get_unknown_chunks(read_ptr,
                      read_info_ptr, &unknowns);

                    if (nosave == 0 && ster_mode >= 0)
                    {
                        /* Add sTER chunk */
                        png_unknown_chunkp ster;
                        P1("Handling sTER as unknown chunk %d\n", num_unknowns);
                        ster = (png_unknown_chunk*)png_malloc(read_ptr,
                            (png_uint_32) sizeof(png_unknown_chunk));
                        png_memcpy((char *)ster[0].name, "sTER",5);
                        ster[0].size = 1;
                        ster[0].data = (png_byte*)png_malloc(read_ptr, 1);
                        ster[0].data[0] = (png_byte)ster_mode;
                        png_set_unknown_chunks(read_ptr, read_info_ptr,
                         ster, 1);
                        png_free(read_ptr,ster[0].data);
                        png_free(read_ptr,ster);
                        num_unknowns++;
                    }

#ifndef PNG_HAVE_IHDR
#define PNG_HAVE_IHDR 0x01
#endif
                    if (ster_mode >= 0)
                        png_set_unknown_chunk_location(read_ptr, read_info_ptr,
                           num_unknowns - 1, (int)PNG_HAVE_IHDR);

                    P1("Found %d unknown chunks\n", num_unknowns);

                    if (nosave == 0 && num_unknowns)
                    {
                        png_unknown_chunkp unknowns_keep; /* allocated by us */
                        int num_unknowns_keep;

                        unknowns_keep = (png_unknown_chunk*)png_malloc(
                          write_ptr, (png_uint_32) num_unknowns
                          *sizeof(png_unknown_chunk));

                        P1("malloc for %d unknown chunks\n", num_unknowns);
                        num_unknowns_keep = 0;

                        /* make an array of only those chunks we want to keep */
                        for (i = 0; i < num_unknowns; i++)
                        {
                            P1("Handling unknown chunk %d %s\n", i,
                               (char *)unknowns[i].name);
                            /* not EBCDIC-safe, but neither is keep_chunks(): */
                            P2("   unknown[%d] = %s (%lu bytes, location %d)\n",
                              i, unknowns[i].name,
                              (unsigned long)unknowns[i].size,
                              unknowns[i].location);
                            if (keep_chunk((char *)unknowns[i].name, argv))
                            {
                                png_memcpy(&unknowns_keep[num_unknowns_keep],
                                  &unknowns[i], sizeof(png_unknown_chunk));
                                ++num_unknowns_keep;
                            }
                        }

                        P1("Keeping %d unknown chunks\n", num_unknowns_keep);
                        png_set_unknown_chunks(write_ptr, write_info_ptr,
                          unknowns_keep, num_unknowns_keep);

                        /* relevant location bits:
                         *   (1) !PNG_HAVE_PLTE && !PNG_HAVE_IDAT (before PLTE)
                         *   (2)  PNG_HAVE_PLTE && !PNG_HAVE_IDAT (between)
                         *   (3)  PNG_AFTER_IDAT                  (after IDAT)
                         * PNG_HAVE_PLTE  = 0x02
                         * PNG_HAVE_IDAT  = 0x04
                         * PNG_AFTER_IDAT = 0x08
                         */
                        for (i = 0; i < num_unknowns_keep; i++)
                        {
                            png_set_unknown_chunk_location(write_ptr,
                              write_info_ptr, i,
                              (int)unknowns_keep[i].location);
                        }

                        /* png_set_unknown_chunks() makes own copy, so nuke
                         * ours */
                        png_free(write_ptr, unknowns_keep);
                    }
                }
              P1( "Finished handling ancillary chunks after IDAT\n");
#endif /* PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED */
            }  /* End of ancillary chunk handling */

                /* } GRR added for quick %-navigation (1) */

                /*
                 * Would be useful to have a libpng fix, that either exports
                 * png_read_transform_info() or provides a generic API that
                 * can set any png_ptr->flag, or, simpler, provides an API
                 * to set the PNG_FLAG_ROW_INIT flag.
                 *
                 * Starting in libpng-1.5.6beta06, png_read_update_info()
                 * does not check the PNG_FLAG_ROW_INIT flag and does not
                 * initialize the row or issue a warning.
                 *
                 */
#ifdef PNGCRUSH_H
                png_read_transform_info(read_ptr, read_info_ptr);
#else
                /* Some pngcrush capabilities are lacking when the system
                 * libpng is used instead of the one bundled with pngcrush
                 *
                 * To do: list those capabilities here
                 */

                /* pngcrush fails to read interlaced PNGs properly
                 * when png_read_update_info() is called here.
                 */

                /* png_read_update_info(read_ptr, read_info_ptr); */

                /* Workaround is to multiply rowbytes by 8/bit_depth
                 * to allow for unpacking, wherever we allocate a
                 * row buffer.  This results in too large a value for
                 * rowbytes under some conditions, e.g., when increasing
                 * a sub-8-bit file to RGBA, but this is rare and harmless.
                 */
#endif

                /* This is the default case (nosave == 1 -> perf-testing
                   only) */
                if (nosave == 0)
                {
                    if (filter_type == 0)
                        png_set_filter(write_ptr, 0, PNG_FILTER_NONE);
                    else if (filter_type == 1)
                        png_set_filter(write_ptr, 0, PNG_FILTER_SUB);
                    else if (filter_type == 2)
                        png_set_filter(write_ptr, 0, PNG_FILTER_UP);
                    else if (filter_type == 3)
                        png_set_filter(write_ptr, 0, PNG_FILTER_AVG);
                    else if (filter_type == 4)
                        png_set_filter(write_ptr, 0, PNG_FILTER_PAETH);
                    else if (filter_type == 5)
                            png_set_filter(write_ptr, 0, PNG_ALL_FILTERS);
                    else if (filter_type == 6) /* speedy */
                            png_set_filter(write_ptr, 0, PNG_FILTER_NONE |
                               PNG_FILTER_SUB | PNG_FILTER_UP);
                    else
                        png_set_filter(write_ptr, 0, PNG_FILTER_NONE);

#ifdef PNGCRUSH_LOCO
                    if (do_loco) {
                        png_byte buff[30];
                        const png_byte png_MHDR[5] = { 77, 72, 68, 82, '\0' };
                        png_byte mng_signature[8] =
                            { 138, 77, 78, 71, 13, 10, 26, 10 };
                        /* write the MNG 8-byte signature */
                        if (outname[strlen(outname) - 3] == 'p')
                            pngcrush_warning(read_ptr,
                              "  Writing a MNG file with a .png extension");
                        pngcrush_write_png(write_ptr, &mng_signature[0],
                                  (png_size_t) 8);
                        png_set_sig_bytes(write_ptr, 8);

                        /* Write a MHDR chunk */

                        buff[0] = (png_byte) ((width >> 24) & 0xff);
                        buff[1] = (png_byte) ((width >> 16) & 0xff);
                        buff[2] = (png_byte) ((width >> 8) & 0xff);
                        buff[3] = (png_byte) ((width) & 0xff);
                        buff[4] = (png_byte) ((height >> 24) & 0xff);
                        buff[5] = (png_byte) ((height >> 16) & 0xff);
                        buff[6] = (png_byte) ((height >> 8) & 0xff);
                        buff[7] = (png_byte) ((height) & 0xff);
                        for (i = 8; i < 27; i++)
                            buff[i] = 0x00;
                        buff[15] = 2; /* layer count */
                        buff[19] = 1; /* frame count */
                        if (output_color_type == 6)
                            buff[27] = 0x09; /* profile: MNG-VLC with trans. */
                        else
                            buff[27] = 0x01;  /* profile: MNG-VLC */
                        png_write_chunk(write_ptr, (png_bytep) png_MHDR,
                                        buff, (png_size_t) 28);
                    }
#endif /* PNGCRUSH_LOCO */

                    pngcrush_pause();

                    if (found_CgBI)
                    {
                        pngcrush_warning(read_ptr,
                            "Cannot read Xcode CgBI PNG");
                    }
                    P1( "\nWriting info struct\n");

                    if (copy_idat == 0)
                    {

#if 0 /* doesn't work; compression level has to be the same as in IDAT */
                    /* if zTXt other compressed chunk */
                    png_set_compression_level(write_ptr, 9);
                    png_set_compression_window_bits(write_ptr, 15);
#endif /* 0 */

                    pngcrush_pause();
                      {
                        png_uint_32 zbuf_size;
                        png_uint_32 required_window;
                        int channels = 0;
                        png_set_compression_strategy(write_ptr,
                                                     z_strategy);
                        png_set_compression_mem_level(write_ptr,
                                                      compression_mem_level);

                        if (output_color_type == 0)
                            channels = 1;
                        if (output_color_type == 2)
                            channels = 3;
                        if (output_color_type == 3)
                            channels = 1;
                        if (output_color_type == 4)
                            channels = 2;
                        if (output_color_type == 6)
                            channels = 4;

                        /* This method was copied from libpng to find maximum
                         * data size.  Only return sizes up to the maximum of
                         * a png_uint_32, by limiting the width and height used
                         * to 15 bits.
                         */
                        png_uint_32 rowbytes;
                        png_uint_32 h = height;

                        /* FIX THIS, does not work with libpng-1.7.0
                         * because info_ptr->bit_depth has not been
                         * updated yet.
                         */
                        rowbytes = png_get_rowbytes(read_ptr, read_info_ptr);
#ifndef PNGCRUSH_H
                        /* We must do this because we could not call
                         * png_read_update_info().
                         */
                        if (input_bit_depth < 8)
                          rowbytes *= 8/input_bit_depth;
#endif
                        if (rowbytes < 16384 && h < 16384)
                        {
                          if (rowbytes * h < 16384)
                          {
                             if (interlace_method)
                             {
                                /* Interlacing makes the uncompressed data
                                 * larger because of the replication of both
                                 * the filter byte and the padding to a byte
                                 * boundary.
                                 */
                                png_uint_32 w = width;
                                unsigned int pd = channels * output_bit_depth;
#if (PNGCRUSH_LIBPNG_VER < 10400)
                                png_size_t cb_base;
#else
                                png_alloc_size_t cb_base;
#endif
                                int interlace_pass;

                                for (cb_base=0, interlace_pass=0;
                                     interlace_pass<=6; ++interlace_pass)
                                {
                                   png_uint_32 pw = PNG_PASS_COLS(w,
                                     interlace_pass);

                                   if (pw > 0)
                                      cb_base += (PNGCRUSH_ROWBYTES(pd, pw)+1) *
                                          PNG_PASS_ROWS(h, interlace_pass);
                                }

                                max_bytes = cb_base;
                             }

                             else
                                max_bytes = (rowbytes+1) * h;
                          }
                          else
                             max_bytes = 0x3fffffffU;
                        }

                        else
                           max_bytes = 0x3fffffffU;

                        required_window = (png_uint_32) (max_bytes + 262);

                        zbuf_size =
                            png_get_compression_buffer_size(write_ptr);

                        /* reinitialize zbuf - compression buffer */
                        if (zbuf_size != max_idat_size)
                        {
                            png_uint_32 max_possible_size =
                                required_window;
                            if (max_possible_size > max_idat_size)
                                max_possible_size = max_idat_size;
                            P2("reinitializing write zbuf to %lu.\n",
                               (unsigned long)max_possible_size);
                            png_set_compression_buffer_size(write_ptr,
                                                            max_possible_size);
                        }

                        if (required_window <= 512)
                            compression_window = 9;
                        else if (required_window <= 1024)
                            compression_window = 10;
                        else if (required_window <= 2048)
                            compression_window = 11;
                        else if (required_window <= 4096)
                            compression_window = 12;
                        else if (required_window <= 8192)
                            compression_window = 13;
                        else if (required_window <= 16384)
                            compression_window = 14;
                        else
                            compression_window = 15;
                        if (compression_window > default_compression_window
                            || force_compression_window)
                            compression_window =
                                default_compression_window;

                        if (verbose > 1 && last_trial
                            && (compression_window != 15
                                || force_compression_window))
                            fprintf(STDERR,
                                    "   Compression window for output= %d\n",
                                    1 << compression_window);

                        png_set_compression_window_bits(write_ptr,
                                                        compression_window);
                      }

                    png_set_compression_level(write_ptr, zlib_level);
                    } /* copy_idat */

                    png_write_info(write_ptr, write_info_ptr);
                    P1( "\nWrote info struct\n");

                    if (copy_idat == 1)
                    {
                       /* No recompress, just copy IDATs. */
                       png_set_compression_buffer_size(write_ptr,
                                                       MAX_IDAT_SIZE);
                    }

#ifdef PNG_WRITE_PACK_SUPPORTED
                    if (output_bit_depth < 8)
                    {
                        if (output_color_type == 0)
                        {
                            png_color_8 true_bits;
                            true_bits.gray = (png_byte) (output_bit_depth);
                            png_set_shift(write_ptr, &true_bits);
                        }
                        png_set_packing(write_ptr);
                    }
#endif
                } /* no save */

#ifdef PNGCRUSH_MULTIPLE_ROWS
                rows_at_a_time = max_rows_at_a_time;
                if (rows_at_a_time == 0 || rows_at_a_time < height)
                    rows_at_a_time = height;
#endif

#ifndef PNGCRUSH_LARGE
                {
                    png_uint_32 rowbytes_s;
                    png_uint_32 rowbytes;

                    rowbytes = png_get_rowbytes(read_ptr, read_info_ptr);
#ifndef PNGCRUSH_H
                    if (input_bit_depth < 8)
                      rowbytes *= 8/input_bit_depth;
#endif

                    P2("rowbytes = %lud\n",rowbytes;

                    rowbytes_s = (png_size_t) rowbytes;
                    if (rowbytes == (png_uint_32) rowbytes_s)
#  ifdef PNGCRUSH_MULTIPLE_ROWS
                        row_buf =
                            png_malloc(read_ptr,
                                       rows_at_a_time * rowbytes + 64);
#  else
                        row_buf = png_malloc(read_ptr, rowbytes + 64);
#  endif
                    else
                        row_buf = NULL;
                }
#else /* PNGCRUSH_LARGE */
                {
                    png_uint_32 read_row_length, write_row_length;
                    read_row_length =
                        (png_uint_32) (png_get_rowbytes
                                       (read_ptr, read_info_ptr));
                    write_row_length =
                        (png_uint_32) (png_get_rowbytes
                                       (write_ptr, write_info_ptr));
                    row_length =
                        read_row_length >
                        write_row_length ? read_row_length :
                        write_row_length;
#ifndef PNGCRUSH_H
                    /* We must do this because we could not call
                     * png_read_update_info().
                     */
                    if (input_bit_depth < 8)
                          row_length *= 8/input_bit_depth;
#endif
#  ifdef PNGCRUSH_MULTIPLE_ROWS
                    row_buf =
                        (png_bytep) png_malloc(read_ptr,
                                               rows_at_a_time *
                                               row_length + 64);
#  else
                    row_buf =
                        (png_bytep) png_malloc(read_ptr, row_length + 64);
#  endif
                }
#endif /* PNGCRUSH_LARGE */

                P2(" row_buf = %p\n",row_buf);

                if (row_buf == NULL)
                    png_error(read_ptr,
                              "Insufficient memory to allocate row buffer");

                {
                    /* Check for sufficient memory: we need 2*zlib_window and,
                     * if filter_type == 5, 4*rowbytes in separate allocations.
                     * If it's not enough we can drop the "average" filter and
                     * we can reduce the zlib_window for writing.  We can't
                     * change the input zlib_window because the input file
                     * might have used the full 32K sliding window. (To do)
                     */
                }

                P2("allocated rowbuf.\n");

#ifdef PNGCRUSH_MULTIPLE_ROWS
                row_pointers = (png_bytepp) png_malloc(read_ptr,
                                                       rows_at_a_time *
                                                       sizeof(png_bytepp));
                P2("allocated row_pointers.\n");

                for (i = 0; i < rows_at_a_time; i++)
                    row_pointers[i] = row_buf + i * row_length;
#endif

                pngcrush_pause();

                num_pass = png_set_interlace_handling(read_ptr);
                if (nosave == 0)
                    png_set_interlace_handling(write_ptr);

/* START_STOP */

                for (pass = 0; pass < num_pass; pass++)
                {
#ifdef PNGCRUSH_MULTIPLE_ROWS
                    png_uint_32 num_rows;
#endif
                    P1( "\nBegin interlace pass %d\n", pass);
#ifdef PNGCRUSH_MULTIPLE_ROWS
                    num_rows = rows_at_a_time;
                    for (y = 0; y < height; y += rows_at_a_time)
#else
                    for (y = 0; y < height; y++)
#endif
                    {
#if PNGCRUSH_TIMERS > 0
                        if (verbose >= 0)
                        {
                           pngcrush_timer_stop(PNGCRUSH_TIMER_MISC);
                           pngcrush_timer_start(PNGCRUSH_TIMER_DECODE);
                        }
#endif
#ifdef PNGCRUSH_MULTIPLE_ROWS
                        if (y + num_rows > height)
                            num_rows = height - y;
#  ifdef BLOCKY_DEINTERLACE
                        png_read_rows(read_ptr, (png_bytepp) NULL,
                                      row_pointers, num_rows);
#  else /* SPARKLE */
                        png_read_rows(read_ptr, row_pointers,
                                      (png_bytepp) NULL, num_rows);
#  endif
#else
#  ifdef BLOCKY_DEINTERLACE
                        png_read_row(read_ptr, row_buf, (png_bytep) NULL);
#  else /* SPARKLE */
                        png_read_row(read_ptr, (png_bytep) NULL, row_buf);
#  endif
#endif
                        if (nosave == 0)
                        {
#if PNGCRUSH_TIMERS > 0
                        if (verbose >= 0)
                        {
                            pngcrush_timer_stop(PNGCRUSH_TIMER_DECODE);
                            pngcrush_timer_start(PNGCRUSH_TIMER_ENCODE);
                        }
#endif
#ifdef PNGCRUSH_MULTIPLE_ROWS
                            /* To do: zero the padding bits */
                            png_write_rows(write_ptr, row_pointers,
                                           num_rows);
#else
                            /* To do: zero the padding bits */
                            png_write_row(write_ptr, row_buf);
#endif
                        }
#if PNGCRUSH_TIMERS > 0
                        if (verbose >= 0)
                        {
                            if (nosave == 0)
                                pngcrush_timer_stop(PNGCRUSH_TIMER_ENCODE);
                            else
                                pngcrush_timer_stop(PNGCRUSH_TIMER_DECODE);
                            pngcrush_timer_start(PNGCRUSH_TIMER_MISC);
                        }
#endif
                        /* Bail if byte count exceeds best so far */
                        if (bail == 0 && trial != last_method &&
                            pngcrush_write_byte_count >
                            pngcrush_best_byte_count)
                        {
                           png_write_flush(write_ptr);
                           break;
                        }
                    }
                    P2( "End interlace pass %d\n\n", pass);
                    if (bail == 0 && trial != last_method &&
                        pngcrush_write_byte_count >
                        pngcrush_best_byte_count)
                       break;
                }

                if (color_type == 3)
                  {
                  if (trial == 0 && reduce_palette == 1)
                     {
                       int palette_length;

                       palette_length = plte_len;
                       P1("Measured palette length = %d\n", palette_length);

#ifdef PNG_READ_bKGD_SUPPORTED
                       {
                         png_color_16p bkgd;
                         if (png_get_bKGD(read_ptr, read_info_ptr, &bkgd))
                         {
                           bkgd_index = bkgd->index;
                           P1("bKGD index = %d\n", bkgd_index);
#if 1
                           if (bkgd_index > palette_length)
                           {
                              bkgd_index = palette_length;
                              have_bkgd = 1;
                              fprintf(STDERR,
                                  "   New bKGD index = %d\n", bkgd_index);
                           }
#endif
                           if (bkgd->index >= palette_length)
                              palette_length = (int) bkgd_index+1;
                           P1("Total    palette length = %d\n", palette_length);
                         }
                       }
#endif
                       plte_len = palette_length;
                       if (num_trans > plte_len)
                       {
                          num_trans = plte_len;
                          png_set_tRNS(write_ptr, write_info_ptr, trns_array,
                                     num_trans, trans_values);
                        }
                     }
                  }

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED) && \
    defined(PNG_FLOATING_POINT_SUPPORTED)
                if ((color_type == 2 || color_type == 6 || color_type == 3)
                    && (output_color_type == 0 || output_color_type == 4))
                {
                    png_byte rgb_error =
                        png_get_rgb_to_gray_status(read_ptr);
                    if (last_trial && verbose > 0 && rgb_error)
                        fprintf(STDERR,
                          "   **** Converted non-gray image to gray. **** \n");
                }
#endif

#ifdef PNG_FREE_UNKN
#  if defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED)
                png_free_data(read_ptr, read_info_ptr, PNG_FREE_UNKN, -1);
#  endif
#  if defined(PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED)
                png_free_data(write_ptr, write_info_ptr, PNG_FREE_UNKN, -1);
#  endif
#else
#  if defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED)
                png_free_unknown_chunks(read_ptr, read_info_ptr, -1);
#  endif
#  if defined(PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED)
                png_free_unknown_chunks(write_ptr, write_info_ptr, -1);
#  endif
#endif /* PNG_FREE_UNKN */

                /* { GRR:  added for %-navigation (2) */
                if (!(bail == 0 && trial != last_method &&
                    pngcrush_write_byte_count >
                    pngcrush_best_byte_count))
                {
                   P1("   Reading and writing end_info data\n");
                   png_read_end(read_ptr, end_info_ptr);

            /* Handle ancillary chunks */
            if (last_trial == 1)
            {
#if (defined(PNG_READ_tEXt_SUPPORTED) && defined(PNG_WRITE_tEXt_SUPPORTED)) \
 || (defined(PNG_READ_iTXt_SUPPORTED) && defined(PNG_WRITE_iTXt_SUPPORTED)) \
 || (defined(PNG_READ_zTXt_SUPPORTED) && defined(PNG_WRITE_zTXt_SUPPORTED))

                P1( "Check for iTXt/tEXt/zTXt chunks after IDAT\n");
                {
                    png_textp text_ptr;
                    int num_text = 0;

                    if (png_get_text (read_ptr,
                        end_info_ptr, &text_ptr, &num_text) > 0
                        || text_inputs)
                    {
                        int ntext;

#ifdef PNG_iTXt_SUPPORTED
                        P1( "Handling %d tEXt/zTXt/iTXt chunks after IDAT\n",
#else
                        P1( "Handling %d tEXt/zTXt chunks after IDAT\n",
#endif
                                   num_text);

                        if (verbose > 1 && last_trial && num_text > 0)
                        {
                            for (ntext = 0; ntext < num_text; ntext++)
                            {
                                fprintf(STDERR, "%d  %s", ntext,
                                        text_ptr[ntext].key);
                                if (text_ptr[ntext].text_length)
                                    fprintf(STDERR, ": %s\n",
                                            text_ptr[ntext].text);
#ifdef PNG_iTXt_SUPPORTED
                                else if (text_ptr[ntext].itxt_length)
                                {
                                    fprintf(STDERR, " (%s: %s): \n",
                                            text_ptr[ntext].lang,
                                            text_ptr[ntext].lang_key);
                                    fprintf(STDERR, "%s\n",
                                            text_ptr[ntext].text);
                                }
#endif
                                else
                                    fprintf(STDERR, "\n");
                            }
                        }

                        if (last_trial)
                        {
                          if (num_text > 0)
                          {
                            if (keep_chunk("text", argv))
                            {
                                int num_to_write = num_text;
                                for (ntext = 0; ntext < num_text; ntext++)
                                {
                                    if (verbose > 1)
                                        P2("Text chunk after IDAT, "
                                          "compression=%d\n",
                                          text_ptr[ntext].compression);
                                    if (text_ptr[ntext].compression ==
                                        PNG_TEXT_COMPRESSION_NONE)
                                    {
                                        if (!keep_chunk("tEXt", argv))
                                        {
                                            text_ptr[ntext].key[0] = '\0';
                                            num_to_write--;
                                        }
                                    }
                                    if (text_ptr[ntext].compression ==
                                        PNG_TEXT_COMPRESSION_zTXt)
                                    {
                                        if (!keep_chunk("zTXt", argv))
                                        {
                                            text_ptr[ntext].key[0] = '\0';
                                            num_to_write--;
                                        }
                                    }
#ifdef PNG_iTXt_SUPPORTED
                                    if (text_ptr[ntext].compression ==
                                        PNG_ITXT_COMPRESSION_NONE
                                        || text_ptr[ntext].compression ==
                                        PNG_ITXT_COMPRESSION_zTXt)
                                    {
                                        if (!keep_chunk("iTXt", argv))
                                        {
                                            text_ptr[ntext].key[0] = '\0';
                                            num_to_write--;
                                        }
                                    }
#endif
                                }
                                if (num_to_write > 0)
                                    png_set_text(write_ptr,
                                                 write_end_info_ptr,
                                                 text_ptr, num_text);
                            }
                          }

                          P1( "text_inputs=%d\n",text_inputs);
                          for (ntext = 0; ntext < text_inputs; ntext++)
                          {
                            P1( "  ntext=%d\n",ntext);
                            if (text_where[ntext] == 2)
                            {
                                png_textp added_text;
                                added_text = (png_textp)
                                    png_malloc(write_ptr,
                                               (png_uint_32)
                                               sizeof(png_text));
                                added_text[0].key =
                                    &text_keyword[ntext * 80];
#ifdef PNG_iTXt_SUPPORTED
                                added_text[0].lang =
                                    &text_lang[ntext * 80];
                                added_text[0].lang_key =
                                    &text_lang_key[ntext * 80];
#endif
                                added_text[0].text =
                                    &text_text[ntext * STR_BUF_SIZE];
                                added_text[0].compression =
                                    text_compression[ntext];
                                png_set_text(write_ptr, write_end_info_ptr,
                                             added_text, 1);

                                if (verbose > 0 && last_trial)
                                {
                                  if (added_text[0].compression < 0)
                                      fprintf(STDERR,
                                          "   Added a tEXt chunk.\n");
                                  else if (added_text[0].compression == 0)
                                      fprintf(STDERR,
                                          "   Added a zTXt chunk.\n");
#ifdef PNG_iTXt_SUPPORTED
                                  else if (added_text[0].compression == 1)
                                      fprintf(STDERR,
                                          "   Added an uncompressed iTXt "
                                        "chunk.\n");
                                  else
                                      fprintf(STDERR,
                                          "   Added a compressed iTXt "
                                        "chunk.\n");
#endif
                                }
                                png_free(write_ptr, added_text);
                                added_text = (png_textp) NULL;
                            }
                          }
                        } /* end of nosave block */
                    }
                }
#endif /* (PNG_READ_tEXt_SUPPORTED and PNG_WRITE_tEXt_SUPPORTED) or */
       /* (PNG_READ_iTXt_SUPPORTED and PNG_WRITE_iTXt_SUPPORTED) or */
       /* (PNG_READ_zTXt_SUPPORTED and PNG_WRITE_zTXt_SUPPORTED) */
#if defined(PNG_READ_tIME_SUPPORTED) && defined(PNG_WRITE_tIME_SUPPORTED)
                {
                    png_timep mod_time;

                    if (png_get_tIME(read_ptr, end_info_ptr, &mod_time))
                    {
                        P1( "Handling tIME chunk after IDAT\n");
                        if (keep_chunk("tIME", argv))
                            png_set_tIME(write_ptr, write_end_info_ptr,
                                         mod_time);
                    }
                }
#endif

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
                /* GRR FIXME?  this block may need same fix as above */
                {
                    png_unknown_chunkp unknowns;
                    int num_unknowns =
                        (int) png_get_unknown_chunks(read_ptr,
                                                     end_info_ptr,
                                                     &unknowns);
                    if (num_unknowns && nosave == 0)
                    {
                        fprintf(STDERR,
                               "Handling %d unknown chunks after IDAT\n",
                               num_unknowns);
                        png_set_unknown_chunks(write_ptr,
                                               write_end_info_ptr,
                                               unknowns, num_unknowns);
                        for (i = 0; i < num_unknowns; i++)
                            png_set_unknown_chunk_location(write_ptr,
                                                           write_end_info_ptr,
                                                           i,
                                                           (int)
                                                           unknowns[i].
                                                           location);
                    }
                }
#endif
            }  /* End of ancillary chunk handling */

                if (nosave == 0) {
#if 0 /* doesn't work; compression level has to be the same as in IDAT */
                    /* if zTXt other compressed chunk */
                    png_set_compression_level(write_ptr, 9);
                    png_set_compression_window_bits(write_ptr, 15);
                    png_set_compression_buffer_size(write_ptr,
                                                    PNG_ZBUF_SIZE);
                    png_set_compression_strategy(write_ptr, 0);
#endif /* 0 */
                    png_write_end(write_ptr, write_end_info_ptr);
                }
                }
                /* } GRR:  added for %-navigation (2) */

                P1( "Destroying read data structs\n");
                if (row_buf != (png_bytep) NULL)
                {
                    P2(" row_buf = %p\n",row_buf);
                    P2( "Destroying row_buf\n");
                    /* Crash has been observed here when using the
                     * system libpng instead of the bundled one.
                     * It seems to have to do with the workaround for
                     * png_read_transform_info() not being exported
                     * in libpng-1.5.6 and later.
                     */
                    png_free(read_ptr, row_buf);
                    row_buf = (png_bytep) NULL;
                }
                P2( "Destroyed data row_buf\n");
#ifdef PNGCRUSH_MULTIPLE_ROWS
                if (row_pointers != (png_bytepp) NULL)
                {
                    P1( "Destroying row_pointers\n");
                    png_free(read_ptr, row_pointers);
                    row_pointers = (png_bytepp) NULL;
                }
                P2( "Destroyed data row_pointers\n");
#endif
                png_destroy_read_struct(&read_ptr, &read_info_ptr,
                                        &end_info_ptr);
                P2( "Destroyed data structs\n");
                if (nosave == 0)
                {
#ifdef PNGCRUSH_LOCO
                    if (do_loco)
                    {
                        const png_byte png_MEND[5] =
                            { 77, 69, 78, 68, '\0' };
                        /* write the MNG MEND chunk */
                        png_write_chunk(write_ptr, (png_bytep) png_MEND,
                                        NULL, (png_size_t) 0);
                    }
#endif
                    png_destroy_info_struct(write_ptr,
                                            &write_end_info_ptr);
                    png_destroy_write_struct(&write_ptr, &write_info_ptr);
                }
            }
            Catch(msg) {
                if (nosave == 0)
                    fprintf(stderr, "While converting %s to %s:\n", inname,
                      outname);
                else
                    fprintf(stderr, "While reading %s:\n", inname);
                fprintf(stderr,
                  "  pngcrush caught libpng error:\n   %s\n\n", msg);
                if (row_buf)
                {
                    png_free(read_ptr, row_buf);
                    row_buf = (png_bytep) NULL;
                }
#ifdef PNGCRUSH_MULTIPLE_ROWS
                if (row_pointers != (png_bytepp) NULL)
                {
                    png_free(read_ptr, row_pointers);
                    row_pointers = (png_bytepp) NULL;
                }
#endif
                if (nosave == 0)
                {
                    png_destroy_info_struct(write_ptr,
                                            &write_end_info_ptr);
                    png_destroy_write_struct(&write_ptr, &write_info_ptr);
                    setfiletype(outname);
                }
                png_destroy_read_struct(&read_ptr, &read_info_ptr,
                                        &end_info_ptr);
                if (verbose > 1)
                    fprintf(stderr, "returning after cleanup\n");
                trial = last_method + 1;
            }

            read_ptr = NULL;
            write_ptr = NULL;
            FCLOSE(fpin);
            if (last_trial && nosave == 0)
            {
                FCLOSE(fpout);
                setfiletype(outname);
            }

            if (nosave)
                break;

            if (trial == 0)
                continue;

            idat_length[trial] = pngcrush_write_byte_count;

            if (pngcrush_write_byte_count < pngcrush_best_byte_count)
               pngcrush_best_byte_count = pngcrush_write_byte_count;

            if (verbose > 0 && trial != last_method)
            {
                if (bail == 0 &&
                    pngcrush_write_byte_count > pngcrush_best_byte_count)
                   fprintf(STDERR,
                     "   Critical chunk length, method %3d"
                     " (ws %d fm %d zl %d zs %d) >%10lu\n",
                     trial, compression_window,
                     filter_type, zlib_level, z_strategy,
                     (unsigned long)pngcrush_best_byte_count);
                else
                   fprintf(STDERR,
                     "   Critical chunk length, method %3d"
                     " (ws %d fm %d zl %d zs %d) =%10lu\n",
                     trial, compression_window,
                     filter_type, zlib_level, z_strategy,
                     (unsigned long)idat_length[trial]);
                fflush(STDERR);
            }

        } /* end of trial-loop */

        P1("\n\nFINISHED MAIN LOOP OVER %d METHODS\n\n\n", last_method);

        /* ////////////////////////////////////////////////////////////////////
        //////////////////                                 ////////////////////
        //////////////////  END OF MAIN LOOP OVER METHODS  ////////////////////
        //////////////////                                 ////////////////////
        //////////////////////////////////////////////////////////////////// */
        }

        if (fpin)
        {
            FCLOSE(fpin);
        }
        if (last_trial && nosave == 0 && fpout)
        {
            FCLOSE(fpout);
            setfiletype(outname);
        }

        if (last_trial && nosave == 0 && overwrite != 0)
        {
            /* rename the new file , outname = inname */
            if (
#if (defined(_Windows) || defined(_WINDOWS) || defined(WIN32) ||  \
   defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__) || \
   defined(__DJGPP__) || defined(CYGWIN))
              remove(inname) != 0 ||
#endif
              rename(outname, inname) != 0 )
            {
                fprintf(STDERR,
                    "error while renaming \"%s\" to \"%s\" \n",outname,inname);
                exit (1);
            }
            else
                P2("rename %s to %s complete.\n",outname,inname);
        }

        if (last_trial && nosave == 0)
        {
            png_uint_32 output_length;
#ifndef __riscos
            struct stat stat_buf;
            struct utimbuf utim;

            stat(inname, &stat_buf);
            utim.actime  = stat_buf.st_atime;
            utim.modtime = stat_buf.st_mtime;
            stat(outname, &stat_buf);
            output_length = (unsigned long) stat_buf.st_size;
            if (new_time_stamp == 0)
            {
              /* set file timestamp (no big deal if fails) */
              utime(outname, &utim);
            }
#else
            output_length = (unsigned long) filesize(outname);
#endif
            if (verbose >= 0 && bench < 2)
            {
                total_input_length += input_length + output_length;

                if (best == 0)
                {
                  fprintf(STDERR,
                  "   Best pngcrush method = 0 (settings undetermined)\n");
                }

#if 0 /* disabled */
                else if (!already_crushed && !image_is_immutable)
#else
                else if (!image_is_immutable)
#endif
                {
                fprintf(STDERR,
                  "   Best pngcrush method        = %3d "
                  "(ws %d fm %d zl %d zs %d) =%10lu\n",
                  best, compression_window, fm[best], lv[best], zs[best],
                  (unsigned long)idat_length[best]);
                }

                if (verbose > 0)
                {
                if (idat_length[0] == idat_length[best])
                    fprintf(STDERR, "     (no critical chunk change)\n");
                else if (idat_length[0] > idat_length[best])
                    fprintf(STDERR, "     (%4.2f%% critical chunk reduction)\n",
                      (100.0 - (100.0 * idat_length[best]) / idat_length[0]));
                else
                    fprintf(STDERR, "     (%4.2f%% critical chunk increase)\n",
                      -(100.0 - (100.0 * idat_length[best]) / idat_length[0]));
                if (input_length == output_length)
                    fprintf(STDERR, "     (no filesize change)\n\n");
                else if (input_length > output_length)
                    fprintf(STDERR, "     (%4.2f%% filesize reduction)\n\n",
                      (100.0 - (100.0 * output_length) / input_length));
                else
                    fprintf(STDERR, "     (%4.2f%% filesize increase)\n\n",
                      -(100.0 - (100.0 * output_length) / input_length));

                if (verbose > 2)
                    fprintf(STDERR, "   Number of open files=%d\n",
                      number_of_open_files);

                }
            }
        }

        if (pngcrush_mode == DEFAULT_MODE || pngcrush_mode == OVERWRITE_MODE)
        {
            if (png_row_filters != NULL)
            {
                free(png_row_filters);
                png_row_filters = NULL;
            }
#if PNGCRUSH_TIMERS > 0
            pngcrush_timer_stop(PNGCRUSH_TIMER_MISC);
            pngcrush_timer_stop(PNGCRUSH_TIMER_TOTAL);
#endif
            if (verbose >= 0)
            {
                show_result();
            }
#ifdef PNG_iCCP_SUPPORTED
            if (iccp_length)
            {
                free(iccp_text);
                iccp_text = NULL;
                iccp_length = 0;
            }
#endif
            break;
        }
    } /* end of loop on input files */

#if PNGCRUSH_TIMERS > 0
    for (pc_timer = 0; pc_timer < PNGCRUSH_TIMERS; pc_timer++)
    {
        unsigned long ts,tn;

        ts=pngcrush_timer_get_seconds(pc_timer);
        tn=pngcrush_timer_get_nanoseconds(pc_timer);
        if (ts < pngcrush_timer_min_secs[pc_timer])
        {
            pngcrush_timer_min_secs[pc_timer] = ts;
            pngcrush_timer_min_nsec[pc_timer] = tn;
        }
        else
        {
        if (tn < pngcrush_timer_min_nsec[pc_timer])
            pngcrush_timer_min_nsec[pc_timer] = tn;
        }

        pngcrush_timer_reset(pc_timer);
    }
#endif

    } /* end of benchmark_iteration loop */
    if (verbose >= 0)
    {
#if (PNGCRUSH_TIMERS > 0)
    for (pc_timer=0;pc_timer < PNGCRUSH_TIMERS; pc_timer++)
    {
      filter_count[pc_timer]+=pngcrush_timer_get_hits(pc_timer);
      t_sec=pngcrush_timer_min_secs[pc_timer];
      t_nsec=pngcrush_timer_min_nsec[pc_timer];
      t_filter[pc_timer] = (float)t_nsec/1000000000.;
      if (t_sec)
        t_filter[pc_timer] += (float)t_sec;
    }

#  if PNGCRUSH_TIMERS >= 3
    fprintf(STDERR, "CPU time decode %.6f,", t_filter[1]);
    fprintf(STDERR, " encode %.6f,", t_filter[2]);
    fprintf(STDERR, " other %.6f,", t_filter[3]);
    fprintf(STDERR, " total %.6f sec\n", t_filter[0]);
#  endif

#  if PNGCRUSH_USE_CLOCK_GETTIME != 0
   if (benchmark_iterations > 0)
   {
#  if PNGCRUSH_TIMERS > 9
    {
    float total_t_filter=0;
    float total_filter_count=0;
    for (pc_timer = 5; pc_timer < 10; pc_timer++)
    {
      total_t_filter+=t_filter[pc_timer];
      total_filter_count+=filter_count[pc_timer];
    }
    if (total_filter_count > 0)
    {
      for (pc_timer = 5; pc_timer < 10; pc_timer++)
      {
        fprintf(STDERR, "     filter[%u] defilter time = %15.9f, count = %lu\n",
            pc_timer-5, t_filter[pc_timer],
           (unsigned long)filter_count[pc_timer]);
      }
      fprintf(STDERR, "     total defilter time     = %15.9f, count = %lu\n",
        total_t_filter, (unsigned long) total_filter_count);
      }
    }
#  endif
#  if PNGCRUSH_TIMERS > 4
    {
      if (filter_count[4] > 0)
        fprintf(STDERR, "     deinterlace time        = %15.9f, count = %lu\n",
             t_filter[4], (unsigned long)filter_count[4]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 1
    {
      fprintf(STDERR, "     total decode time       = %15.9f, count = %lu\n",
           t_filter[1], (unsigned long)filter_count[1]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 10
    {
      if (filter_count[10] > 0)
        fprintf(STDERR, "     filter setup time       = %15.9f, count = %lu\n",
             t_filter[10], (unsigned long)filter_count[10]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 2
    {
      fprintf(STDERR, "     total encode time       = %15.9f, count = %lu\n",
           t_filter[2], (unsigned long)filter_count[2]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 3
    {
      fprintf(STDERR, "     total misc time         = %15.9f, count = %lu\n",
           t_filter[3], (unsigned long)filter_count[3]);
    }
#  endif
#  if PNGCRUSH_TIMERS > 0
    {
      fprintf(STDERR, "     total time              = %15.9f, count = %lu\n",
           t_filter[0], (unsigned long)filter_count[0]);
    }
#  endif
    }
#endif
#endif
    }

    if (pngcrush_must_exit)
       exit(0);
    return 0;  /* just in case */

} /* end of main() */




png_uint_32 measure_idats(FILE * fp_in)
{
    /* Copyright (C) 1999-2002, 2006-2015 Glenn Randers-Pehrson
       (glennrp at users.sf.net).  See notice in pngcrush.c for conditions of
       use and distribution */
    P2("\nmeasure_idats:\n");
    P1( "Allocating read structure\n");
/* OK to ignore any warning about the address of exception__prev in "Try" */
    Try {
        read_ptr =
            png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL,
                                   (png_error_ptr) pngcrush_cexcept_error,
                                   (png_error_ptr) NULL);
        P1( "Allocating read_info,  end_info structures\n");
        read_info_ptr = png_create_info_struct(read_ptr);
        end_info_ptr = png_create_info_struct(read_ptr);

#ifdef PNG_STDIO_SUPPORTED
        png_init_io(read_ptr, fp_in);
#else
        png_set_read_fn(read_ptr, (png_voidp) fp_in, (png_rw_ptr) NULL);
#endif

        png_set_sig_bytes(read_ptr, 0);
        measured_idat_length = pngcrush_measure_idat(read_ptr);
        P2("measure_idats: IDAT length=%lu\n",
          (unsigned long)measured_idat_length);
        P1( "Destroying data structs\n");
        png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
    }
    Catch(msg) {
        fprintf(STDERR, "\nWhile measuring IDATs in %s ", inname);
        fprintf(STDERR, "pngcrush caught libpng error:\n   %s\n\n", msg);
        png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
        P1( "Destroyed data structs\n");
        measured_idat_length = 0;
    }
    return measured_idat_length;
}




#define PNGCRUSH_a  97
#define PNGCRUSH_z 122
#define PNGCRUSH_A  65
#define PNGCRUSH_Z  90 

png_uint_32 pngcrush_measure_idat(png_structp png_ptr)
{
    /* Copyright (C) 1999-2002, 2006-2015 Glenn Randers-Pehrson
       (glennrp at users.sf.net)
       See notice in pngcrush.c for conditions of use and distribution */

    /* Signature + IHDR + IEND; we'll add PLTE + IDAT lengths */
    png_uint_32 sum_idat_length = 45;

    png_byte *bb = NULL;
    png_uint_32 malloced_length=0;

    {
        png_byte png_signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
#ifdef PNGCRUSH_LOCO
        png_byte mng_signature[8] = { 138, 77, 78, 71, 13, 10, 26, 10 };
#endif

        pngcrush_default_read_data(png_ptr, png_signature, 8);
        png_set_sig_bytes(png_ptr, 8);

#ifdef PNGCRUSH_LOCO
        if (!(int) (png_memcmp(mng_signature, png_signature, 8)))
        {
            const png_byte png_MHDR[5] = { 77, 72, 68, 82, '\0' };

            int ib;
            png_byte buff[28] = { 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0,  0, 0, 0 };
            unsigned long length;
            /* read the MHDR */
            pngcrush_default_read_data(png_ptr, buff, 4);

            length = pngcrush_get_uint_31(png_ptr,buff);
            if (length > 28)
              png_error(png_ptr, "MHDR length too long");

            pngcrush_default_read_data(png_ptr, buff, 4);

            /* To do: combine with checking for valid PNG chunk_name, below */
            /* Check for valid chunk name [A-Za-z][A-Za-z][A-Z][A-Za-z] */
            if (!(((buff[0] >= PNGCRUSH_a && buff[0] <= PNGCRUSH_z)  ||
                   (buff[0] >= PNGCRUSH_A && buff[0] <= PNGCRUSH_Z)) &&
                  ((buff[1] >= PNGCRUSH_a && buff[1] <= PNGCRUSH_z)  ||
                   (buff[1] >= PNGCRUSH_A && buff[1] <= PNGCRUSH_Z)) &&
                  ((buff[2] >= PNGCRUSH_A && buff[2] <= PNGCRUSH_Z)) &&
                  ((buff[3] >= PNGCRUSH_a && buff[3] <= PNGCRUSH_z)  ||
                   (buff[3] >= PNGCRUSH_A && buff[3] <= PNGCRUSH_Z))))
            {
               int i;
               fprintf (STDERR,"Invalid MNG chunk name: \"");
               for (i=0; i<4; i++)
               {
                 if ((buff[i] >= PNGCRUSH_a && buff[i] <= PNGCRUSH_z) ||
                     (buff[i] >= PNGCRUSH_A && buff[i] <= PNGCRUSH_Z) ||
                     (buff[i] >= '0' && buff[i] <= '9'))
                    fprintf(STDERR,"%c",buff[i]);
                 else
                    fprintf(STDERR,"?");
               }
               fprintf(STDERR,"\" (0x%2x 0x%2x 0x%2x 0x%2x)\n",
                  buff[0],buff[1],buff[2],buff[3]);
            }
            else
               if (verbose > 1)
                 printf("   Reading %c%c%c%c chunk.\n",buff[0],buff[1],buff[2],
                   buff[3]);

            pngcrush_default_read_data(png_ptr, buff, length);

            if (length < 28)
              for (ib=27; ib >= length; ib--) 
                 buff[ib] = 0;

            if (verbose > 0) {
              printf("  width=%lu\n",
                (unsigned long) pngcrush_get_uint_31(png_ptr,buff));
              printf("  height=%lu\n",
                (unsigned long) pngcrush_get_uint_31(png_ptr,&buff[4]));
              printf("  ticksps=%lu\n",
                (unsigned long) pngcrush_get_uint_31(png_ptr,&buff[8]));
              printf("  nomlayc=%lu\n",
                (unsigned long) pngcrush_get_uint_31(png_ptr,&buff[12]));
              printf("  nomfram=%lu\n",
                (unsigned long) pngcrush_get_uint_31(png_ptr,&buff[16]));
              printf("  nomplay=%lu\n",
                (unsigned long) pngcrush_get_uint_31(png_ptr,&buff[20]));
              printf("  profile=%lu\n",
                (unsigned long) pngcrush_get_uint_31(png_ptr,&buff[24]));
            }

            if (new_mng)
            {
                /* write the MNG 8-byte signature */
                pngcrush_write_png(mng_ptr, &mng_signature[0],
                                  (png_size_t) 8);

                /* Write a MHDR chunk */
                png_write_chunk(mng_ptr, (png_bytep) png_MHDR,
                            buff, (png_size_t) 28);
            }

            pngcrush_default_read_data(png_ptr, buff, 4);
            input_format = 1;

        }
        else
#endif
        if (png_sig_cmp(png_signature, 0, 8))
        {
            if (png_sig_cmp(png_signature, 0, 4))
                png_error(png_ptr, "Not a PNG file..");
            else
                png_error(png_ptr,
                          "PNG file corrupted by ASCII conversion");
        }
    }

    if (salvage)
    {
#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
        /* The warning here about deprecated access to png_ptr->zstream
         * is unavoidable.  This will not work with libpng-1.5.x and later.
         */
        inflateUndermine(&png_ptr->zstream, 1);
#endif
    }

    for (;;)
    {
#ifndef PNG_UINT_IDAT
#  ifdef PNG_USE_LOCAL_ARRAYS
        PNG_IDAT;
        PNG_IEND;
        PNG_IHDR;
        PNG_acTL;
#    ifdef PNG_iCCP_SUPPORTED
        PNG_iCCP;
#    endif
#  else
#    ifdef PNG_iCCP_SUPPORTED
        const png_byte png_iCCP[5] = { 105, 67, 67, 80, '\0' };
#    endif
        const png_byte png_acTL[5] = {  97, 99, 84, 76, '\0' };
#  endif
#endif

        png_byte chunk_name[5];
        png_byte chunk_length[4];
        png_byte buff[32];
        png_uint_32 length;

        pngcrush_default_read_data(png_ptr, chunk_length, 4);
        length = pngcrush_get_uint_31(png_ptr,chunk_length);

        pngcrush_reset_crc(png_ptr);
        pngcrush_crc_read(png_ptr, chunk_name, 4);
        chunk_name[4]='\0';

/* Check for valid chunk name [A-Za-z][A-Za-z][A-Z][A-Za-z] */
        if (!(((chunk_name[0] >= PNGCRUSH_a && chunk_name[0] <= PNGCRUSH_z)  ||
               (chunk_name[0] >= PNGCRUSH_A && chunk_name[0] <= PNGCRUSH_Z)) &&
              ((chunk_name[1] >= PNGCRUSH_a && chunk_name[1] <= PNGCRUSH_z)  ||
               (chunk_name[2] >= PNGCRUSH_A && chunk_name[2] <= PNGCRUSH_Z)) &&
              ((chunk_name[2] >= PNGCRUSH_A && chunk_name[2] <= PNGCRUSH_Z)) &&
              ((chunk_name[3] >= PNGCRUSH_a && chunk_name[3] <= PNGCRUSH_z)  ||
               (chunk_name[3] >= PNGCRUSH_A && chunk_name[3] <= PNGCRUSH_Z))))
        {
           int i;

           fprintf (STDERR,"Invalid chunk name: \"");
               for (i=0; i<4; i++)
               {
                 if ((chunk_name[i] >= PNGCRUSH_a &&
                      chunk_name[i] <= PNGCRUSH_z) ||
                     (chunk_name[i] >= PNGCRUSH_A &&
                      chunk_name[i] <= PNGCRUSH_Z)||
                     (chunk_name[i] >= '0' &&
                      chunk_name[i] <= '9'))
                    fprintf(STDERR,"%c",chunk_name[i]);
                 else
                    fprintf(STDERR,"?");
               }
               fprintf(STDERR,"\" (0x%2x 0x%2x 0x%2x 0x%2x)\n",
                  chunk_name[0],chunk_name[1],chunk_name[2],chunk_name[3]);
        }
        else
           if (verbose > 1)
              printf("   Reading %c%c%c%c chunk.\n",
                  chunk_name[0],chunk_name[1],chunk_name[2],chunk_name[3]);

        if (new_mng)
        {
          const png_byte png_DHDR[5] = {  68, 72, 68, 82, '\0' };
          const png_byte png_DEFI[5] = {  68, 69, 70, 73, '\0' };
          const png_byte png_FRAM[5] = {  70, 82, 65, 77, '\0' };
          const png_byte png_nEED[5] = { 110, 69, 69, 68, '\0' };

          if (!png_memcmp(chunk_name, png_nEED, 4))
          {
              /* Skip the nEED chunk */
              if (verbose > 0)
                printf ("  skipping MNG %c%c%c%c chunk, %lu bytes\n",
                  chunk_name[0],
                  chunk_name[1],chunk_name[2],chunk_name[3],
                  (unsigned long)length);
          }
          else
          {
              /* copy the chunk. */
              if (verbose > 0)
                printf ("  reading MNG %c%c%c%c chunk, %lu bytes\n",
                   chunk_name[0],
                   chunk_name[1],chunk_name[2],chunk_name[3],
                   (unsigned long)length);
              if (length+1 > malloced_length)
              {
                  png_free(mng_ptr,bb);
                  if (verbose > 0)
                    printf ("  png_malloc %lu bytes.\n",(unsigned long)length);
                  bb=(png_byte*)png_malloc(mng_ptr, length+1);
                  malloced_length=length+1;
              }
              pngcrush_crc_read(png_ptr, bb, length);
              bb[length]='\0';
              png_write_chunk(mng_ptr, chunk_name,
                            bb, (png_size_t) length);

              if (verbose > 1 && !png_memcmp(chunk_name, png_DHDR, 4))
              {
                  if (length > 19)
                  {
                    printf("  objid=%lu\n",(unsigned long)(bb[1]+(bb[0]<<8)));
                    printf("  itype=%lu\n",(unsigned long)(bb[2]));
                    printf("  dtype=%lu\n",(unsigned long)(bb[3]));
                    printf("  width=%lu\n",
                      (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[4]));
                    printf("  height=%lu\n",
                      (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[8]));
                    printf("  xloc=%lu\n",
                      (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[12]));
                    printf("  yloc=%lu\n",
                      (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[16]));
                  }
              }

              if (verbose > 1 && !png_memcmp(chunk_name, png_DEFI, 4))
              {
                  if (length > 3)
                  {
                    printf("  objid=%lu\n",(unsigned long)(bb[1]+(bb[0]<<8)));
                    printf("  do_not_show=%lu\n",(unsigned long)(bb[2]));
                    printf("  concrete=%lu\n",(unsigned long)(bb[3]));
                  }
                  if (length > 19)
                  {
                      printf("  xloc=%lu\n",
                        (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[12]));
                      printf("  yloc=%lu\n",
                        (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[16]));
                    if (length > 28)
                    {
                      printf("  l_cb=%lu\n",
                        (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[20]));
                      printf("  r_cb=%lu\n",
                        (unsigned long) pngcrush_get_uint_31(png_ptr,&bb[24]));
                    }
                  }
              }
              if (verbose > 1 && !png_memcmp(chunk_name, png_FRAM, 4))
              {
                  printf("  mode=%lu\n",(unsigned long)bb[0]);
                  if (length > 1)
                  {
                      int ib;
                      printf("  name = ");
                      bb[length]='\0';
                      for (ib=0; bb[ib]; ib++)
                      {
                        printf ("%c", bb[ib]);
                      }
                      printf ("\n");
                  }
                  length=0;
              }
          }
        }

#ifdef PNG_UINT_acTL
        else if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_acTL)
#else
        else if (!png_memcmp(chunk_name, png_acTL, 4))
#endif
        {
           found_acTL_chunk = 1;
        }

#ifdef PNG_UINT_IDAT
            if ((pngcrush_get_uint_32(chunk_name) == PNG_UINT_IDAT) ||
#endif
#ifndef PNG_UINT_IDAT
            if ((!png_memcmp(chunk_name, png_IDAT, 4)) ||
#endif
#ifdef PNG_UINT_PLTE
               (pngcrush_get_uint_32(chunk_name) == PNG_UINT_PLTE))
#endif
#ifndef PNG_UINT_PLTE
               (!png_memcmp(chunk_name, png_PLTE, 4)))
#endif
            {
                sum_idat_length += (length + 12);
#if 0 /* disabled */
                if (length > crushed_idat_size)
                    already_crushed++;
#endif
            }

            if (verbose > 1)
            {
                chunk_name[4] = '\0';
                fprintf(STDERR, "   Reading %s chunk, length = %lu.\n",
                  chunk_name, (unsigned long)length);
            }

            if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_CgBI)
            {
                fprintf(STDERR,
                    " This is an Xcode CgBI file, not a PNG file.\n");
                if (salvage)
                {
                    fprintf (STDERR, " Removing the CgBI chunk.\n");
                }
                else
                {
                    fprintf (STDERR,
                    " Try \"pngcrush -fix ...\" to attempt to read it.\n");
                }
                found_CgBI++;
                nosave++;
            }


#ifdef PNG_UINT_IHDR
            if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_IHDR)
#else
            if (!png_memcmp(chunk_name, png_IHDR, 4))
#endif
            {
                /* get the color type */
                pngcrush_crc_read(png_ptr, buff, 13);
                length -= 13;
                input_color_type = buff[9];
            }
            else if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_dSIG)
            {
                if (found_any_chunk == 0 && !all_chunks_are_safe)
                {
                   image_is_immutable=1;
                }
            }
            else
                found_any_chunk=1;

#ifdef PNG_gAMA_SUPPORTED
#ifdef PNG_UINT_gAMA
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_gAMA)
#else
        if (!png_memcmp(chunk_name, png_gAMA, 4))
#endif
          found_gAMA=1;
#endif /* PNG_gAMA_SUPPORTED */

#ifdef PNG_bKGD_SUPPORTED
#ifdef PNG_UINT_bKGD
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_bKGD)
#else
        if (!png_memcmp(chunk_name, png_bKGD, 4))
#endif
          if (length == 6)
            {
                /* Set found_color_bKGD if the components are different,
                 * so we do not do reduction of color-type from color to gray
                 */
                pngcrush_crc_read(png_ptr, buff, 6);
                length -= 6;
                if ((buff[0] != buff[2]) && (buff[0] != buff[4]) &&
                    (buff[1] != buff[3]) && (buff[0] != buff[5]))
                  found_color_bKGD=1;
            }
#endif /* PNG_bKGD_SUPPORTED */

#ifdef PNG_cHRM_SUPPORTED
#ifdef PNG_UINT_cHRM
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_cHRM)
#else
        if (!png_memcmp(chunk_name, png_cHRM, 4))
#endif
          found_cHRM=1;
#endif /* PNG_cHRM_SUPPORTED */

#ifdef PNG_hIST_SUPPORTED
#ifdef PNG_UINT_hIST
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_hIST)
#else
        if (!png_memcmp(chunk_name, png_hIST, 4))
#endif
          found_hIST=1;
#endif /* PNG_hIST_SUPPORTED */

#ifdef PNG_iCCP_SUPPORTED
        /* check for bad Photoshop iCCP chunk */
#ifdef PNG_UINT_iCCP
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_iCCP)
#else
        if (!png_memcmp(chunk_name, png_iCCP, 4))
#endif
        {
            found_iCCP = 1;

            /* Check for bad Photoshop iCCP chunk.  Libpng will reject the
             * bad chunk because the Adler-32 bytes are missing, but we check
             * here to see if it's really the sRGB profile, and if so, set the
             * "intent" flag and gamma so pngcrush will write an sRGB chunk
             * and a gamma chunk.
             */
            if (length == 2615)
            {
                pngcrush_crc_read(png_ptr, buff, 22);
                length -= 22;
                buff[23] = 0;
                if (!strncmp((png_const_charp) buff, "Photoshop ICC profile",
                     21))
                {
                    fprintf(STDERR,
                      "   Replacing bad Photoshop iCCP chunk with an "
                      "sRGB chunk\n");
#ifdef PNG_gAMA_SUPPORTED
#  ifdef PNG_FIXED_POINT_SUPPORTED
                    image_specified_gamma = 45455L;
#  else
                    image_specified_gamma = 0.45455;
#  endif
#endif /* PNG_gAMA_SUPPORTED */
                    found_iCCP = 0;
                    found_sRGB = 1;
                    intent = 0;
                }
            }
            /* To do: recognize other sRGB iCCP chunks and replace them with
             * sRGB chunk
             */
        }
#endif /* PNG_iCCP_SUPPORTED */

#ifdef PNG_sBIT_SUPPORTED
#ifdef PNG_UINT_sBIT
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_sBIT)
#else
        if (!png_memcmp(chunk_name, png_sBIT, 4))
#endif
        {
          if (length <= 4)
          {
            int i;
            pngcrush_crc_read(png_ptr, buff, length);
            found_sBIT_max=0;
            for (i=length; i; i--)
              if (buff[i] > found_sBIT_max)
                 found_sBIT_max=buff[i];

            if (length > 2)
              if (buff[0] != buff[1] || buff[0] != buff[2])
                found_sBIT_different_RGB_bits = 1;

            found_sBIT = 1;
            length = 0;
          }
        }
#endif /* PNG_sBIT_SUPPORTED */

#ifdef PNG_tRNS_SUPPORTED
#ifdef PNG_UINT_tRNS
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_tRNS)
#else
        if (!png_memcmp(chunk_name, png_tRNS, 4))
#endif
          found_tRNS=1;
#endif /* PNG_tRNS_SUPPORTED */

        pngcrush_crc_finish(png_ptr, length);

#ifdef PNGCRUSH_LOCO
#  ifdef PNG_UINT_MEND
        if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_MEND)
            return sum_idat_length;
#  else
        {
            const png_byte png_MEND[5] =
                { 77, 69, 78, 68, '\0' };
            if (!png_memcmp(chunk_name, png_MEND, 4))
            {
                if (new_mng)
                {
                    png_free(mng_ptr,bb);
                    return (0);
                }
                return sum_idat_length;
            }
        }
#  endif
#endif


#ifdef PNGCRUSH_LOCO
        if (input_format == 0)
#endif
        {
#ifdef PNG_UINT_IEND
            if (pngcrush_get_uint_32(chunk_name) == PNG_UINT_IEND)
#else
            if (!png_memcmp(chunk_name, png_IEND, 4))
#endif
            {
                if (!salvage && found_CgBI)
                    return 0;
                else
                    return sum_idat_length;
            }
        }
    }
}


void print_version_info(void)
{
    char *zlib_copyright;

#ifndef ZLIB_VERNUM /* This became available in zlib-1.2 */
         zlib_copyright=" (or later)";
#else
    switch (ZLIB_VERNUM)
    {
      case 0x1220:
         zlib_copyright="-2004";
         break;
      case 0x1230:
         zlib_copyright="-2005";
         break;
      case 0x1240:
      case 0x1250:
         zlib_copyright="-2010";
         break;
      case 0x1260:
      case 0x1270:
         zlib_copyright="-2012";
         break;
      case 0x1271:
      case 0x1280:
         zlib_copyright="-2013";
         break;
      case 0x1290:
      case 0x12a0:
      case 0x12b0:
         zlib_copyright="-2017";
         break;
      default:
         zlib_copyright=" (or later)";
         break;
    }
#endif

#ifdef PNGCRUSH_H
#  define BUNDLED_LIB "bundled"
#else
#  define BUNDLED_LIB "system"
#endif

    fprintf(STDERR,
      "\n"
      " | pngcrush-%s\n"
      /* If you have modified this source, you may insert additional notices
       * immediately after this sentence: */
      " |    Copyright (C) 1998-2002, 2006-2017 Glenn Randers-Pehrson\n"
      " |    Portions Copyright (C) 2005 Greg Roelofs\n"
      " | This is a free, open-source program.  Permission is irrevocably\n"
      " | granted to everyone to use this version of pngcrush without\n"
      " | payment of any fee.\n"
      " | Executable name is %s\n"
      " | It was built with   %s libpng-%s\n"
      " | and is running with %s libpng-%s\n"
      " |    Copyright (C) 1998-2004, 2006-2017 Glenn Randers-Pehrson,\n"
      " |    Copyright (C) 1996, 1997 Andreas Dilger,\n"
      " |    Copyright (C) 1995, Guy Eric Schalnat, Group 42 Inc.,\n"
      " | and %s zlib-%s, Copyright (C) 1995%s,\n"
      " |    Jean-loup Gailly and Mark Adler",
      PNGCRUSH_VERSION, progname,
      BUNDLED_LIB,PNG_LIBPNG_VER_STRING,
      BUNDLED_LIB,png_get_header_ver(NULL),
      BUNDLED_LIB,ZLIB_VERSION,
      zlib_copyright);

#if PNGCRUSH_TIMERS > 0
      fprintf(STDERR,
        ",\n | and using \"%s\".\n",PNGCRUSH_USING_CLOCK);
#else
      fprintf(STDERR,"\n");
#endif
#if defined(__GNUC__)
    fprintf(STDERR,
      " | It was compiled with gcc version %s", __VERSION__);
#  if defined(__DJGPP__)
    fprintf(STDERR,
      "\n"
      " | under DJGPP %d.%d, Copyright (C) 1995, DJ Delorie\n"
      " | and loaded with PMODE/DJ, by Thomas Pytel and Matthias Grimrath\n"
      " |    Copyright (C) 1996, Matthias Grimrath.\n",
      __DJGPP__, __DJGPP_MINOR__);
#  else
    fprintf(STDERR, "\n");
#  endif
#endif

#if PNG_ARM_NEON_OPT > 0
    fprintf(STDERR," | using ARM_NEON optimizations.\n");
#endif
#if PNG_MIPS_MSA_OPT > 0
    fprintf(STDERR," | using MIPS_MSA optimizations.\n");
#endif
#if PNG_POWERPC_VSX_OPT > 0
    fprintf(STDERR," | using POWERPC_VSX optimizations.\n");
#endif
#if PNG_INTEL_SSE_OPT > 0
    fprintf(STDERR," | using INTEL_SSE optimizations.\n");
#endif

    fprintf(STDERR, "\n");
}





static const char *pngcrush_legal[] = {
    "",
    "If you have modified this source, you may insert additional notices",
    "immediately after this sentence.",
    "Copyright (C) 1998-2002, 2006-2017 Glenn Randers-Pehrson",
    "Portions Copyright (C) 2005 Greg Roelofs",
    "",
    "DISCLAIMER: The pngcrush computer program is supplied \"AS IS\".",
    "The Author disclaims all warranties, expressed or implied, including,",
    "without limitation, the warranties of merchantability and of fitness",
    "for  any purpose.  The Author assumes no liability for direct, indirect,",
    "incidental, special, exemplary, or consequential damages, which may",
    "result from the use of the computer program, even if advised of the",
    "possibility of such damage.  There is no warranty against interference",
    "with your enjoyment of the computer program or against infringement.",
    "There is no warranty that my efforts or the computer program will",
    "fulfill any of your particular purposes or needs.  This computer",
    "program is provided with all faults, and the entire risk of satisfactory",
    "quality, performance, accuracy, and effort is with the user.",
    "",
    "LICENSE: Permission is hereby irrevocably granted to everyone to use,",
    "copy, modify, and distribute this computer program, or portions hereof,",
    "purpose, without payment of any fee, subject to the following",
    "restrictions:",
    "",
    "1. The origin of this binary or source code must not be misrepresented.",
    "",
    "2. Altered versions must be plainly marked as such and must not be",
    "misrepresented as being the original binary or source.",
    "",
    "3. The Copyright notice, disclaimer, and license may not be removed",
    "or altered from any source, binary, or altered source distribution.",
    ""
};

static const char *pngcrush_usage[] = {
    "\nusage: %s [options except for -e -d] infile.png outfile.png\n",
    "       %s -e ext [other options] file.png ...\n",
    "       %s -d dir/ [other options] file.png ...\n",
    "       %s -ow [other options] file.png [tempfile.png]\n",
    "       %s -n -v file.png ...\n"
};

struct options_help pngcrush_options[] = {

    {0, "         -bail (bail out of trial when size exceeds best size found"},
    {2, ""},
    {2, "               Default is to bail out and simply report that the"},
    {2, "               filesize for the trial would be greater than the"},
    {2, "               best filesize achieved so far.  Use the \"-nobail\""},
    {2, "               option to prevent that."},
    {2, ""},

    {0, "      -blacken (zero samples underlying fully-transparent pixels)"},
    {2, ""},
    {2, "               Changing the color samples to zero can improve the"},
    {2, "               compressibility.  Since this is a lossy operation,"},
    {2, "               blackening is off by default."},
    {2, ""},

#ifdef Z_RLE
    {0, "        -brute (use brute-force: try 176 different methods)"},
#else
    {0, "        -brute (use brute-force: try 166 different methods)"},
#endif
    {2, ""},
    {2, "               Very time-consuming and generally not worthwhile."},
    {2, "               You can restrict this option to certain filter types,"},
    {2, "               compression levels, or strategies by following it"},
    {2, "               with \"-f filter\", \"-l level\", or \"-z strategy\"."},
    {2, ""},

    {0, FAKE_PAUSE_STRING},

    {0, "            -c color_type of output file [0, 2, 4, or 6]"},
    {2, ""},
    {2, "               Color type for the output file.  Future versions"},
    {2, "               will also allow color_type 3, if there are 256 or"},
    {2, "               fewer colors present in the input file.  Color types"},
    {2, "               4 and 6 are padded with an opaque alpha channel if"},
    {2, "               the input file does not have alpha information."},
    {2, "               You can use 0 or 4 to convert color to grayscale."},
    {2, "               Use 0 or 2 to delete an unwanted alpha channel."},
    {2, "               Default is to use same color type as the input file."},
    {2, ""},

    {0, "        -check (check CRC and ADLER32 checksums)"},
    {2, ""},
    {2, "               Use \"-nocheck\" (default) to skip checking them"},
    {2, ""},

    {0, "            -d directory_name/ (where output files will go)"},
    {2, ""},
    {2, "               If a directory name is given, then the output"},
    {2, "               files are placed in it, with the same filenames as"},
    {2, "               those of the original files. For example,"},
    {2, "               you would type 'pngcrush -directory CRUSHED/ *.png'"},
    {2, "               to get *.png => CRUSHED/*.png.  The trailing slash is"},
    {2, "               optional, but if pngcrush appends the wrong kind of"},
    {2, "               slash or backslash, please include the correct one"},
    {2, "               at the end of the directory_name, as shown."},
    {2, ""},

    {0, FAKE_PAUSE_STRING},

    {0, "            -e extension  (used for creating output filename)"},
    {2, ""},
    {2, "               e.g., -ext .new means *.png => *.new"},
    {2, "               and -e _pc.png means *.png => *_pc.png"},
    {2, ""},

    {0, "            -f user_filter [0-5] for specified method"},
    {2, ""},
    {2, "               filter to use with the method specified in the"},
    {2, "               preceding '-m method' or '-brute_force' argument."},
    {2, "               0: none; 1-4: use specified filter; 5: adaptive."},
    {2, ""},

    {0, FAKE_PAUSE_STRING},

    {0, "          -fix (salvage PNG with otherwise fatal conditions)"},
    {2, ""},
    {2, "               Fixes bad CRCs, bad adaptive filter bytes,"},
    {2, "               or bad CMF bytes in the IDAT chunk that cause"},
    {2, "               the \"Too far back\" error"},
    {2, ""},

    {0, "        -force (default; write output even if IDAT does not decrease)"},
    {2, ""},

#ifdef PNG_FIXED_POINT_SUPPORTED
    {0, "            -g gamma (float or fixed*100000, e.g., 0.45455 or 45455)"},
#else
    {0, "            -g gamma (float, e.g., 0.45455)"},
#endif
    {2, ""},
    {2, "               Value to insert in gAMA chunk, only if the input"},
    {2, "               file has no gAMA chunk.  To replace an existing"},
    {2, "               gAMA chunk, use the '-replace_gamma' option."},
    {2, ""},

    {0, FAKE_PAUSE_STRING},

    {0, "      -huffman (use only zlib strategy 2, Huffman-only)"},
    {2, ""},
    {2, "               Fast, but almost never very effective except for"},
    {2, "               certain rare image types."},
    {2, ""},

#ifdef PNG_iCCP_SUPPORTED
    {0, "         -iccp length \"Profile Name\" iccp_file"},
    {2, ""},
    {2, "               file with ICC profile to insert in an iCCP chunk."},
    {2, ""},
#endif

#ifdef PNG_iTXt_SUPPORTED
    {0, "         -itxt b[efore_IDAT]|a[fter_IDAT] \"keyword\""},
    {2, "               \"language_code\" \"translated_keyword\" \"text\""},
    {2, ""},
    {2, "               Uncompressed iTXt chunk to insert (see -text)."},
    {2, ""},
#endif

    {0, "         -keep chunk_name"},
    {2, ""},
    {2, "               keep named chunk even when pngcrush makes"},
    {2, "               changes to the PNG datastream that cause it"},
    {2, "               to become invalid.  Currently only dSIG is"},
    {2, "               recognized as a chunk to be kept."},
    {2, ""},


    {0, "            -l zlib_compression_level [0-9] for specified method"},
    {2, ""},
    {2, "               zlib compression level to use with method specified"},
    {2, "               with the preceding '-m method' or '-brute_force'"},
    {2, "               argument."},
    {2, ""},

#ifdef PNGCRUSH_LOCO
    {0, FAKE_PAUSE_STRING},

    {0, "         -loco (\"loco crush\" truecolor PNGs)"},
    {2, ""},
    {2, "               Make the file more compressible by performing a"},
    {2, "               lossless, reversible, color transformation."},
    {2, "               The resulting file is a MNG, not a PNG, and should"},
    {2, "               be given the \".mng\" file extension.  The"},
    {2, "               \"loco\" option has no effect on grayscale or"},
    {2, "               indexed-color PNG files."},
    {2, ""},
#endif

    {0, "            -m method [1 through " STRNGIFY(MAX_METHODS) "]"},
    {2, ""},
    {2, "               pngcrush method to try.  Can be repeated as in"},
    {2, "               '-m 1 -m 4 -m 7'. This can be useful if pngcrush"},
    {2, "               runs out of memory when it tries methods 2, 3, 5,"},
    {2, "               6, 8, 9, or 10 which use filtering and are memory-"},
    {2, "               intensive.  Methods 1, 4, and 7 use no filtering;"},
    {2, "               methods 11 and up use a specified filter,"},
    {2, "               compression level, and strategy."},
    {2, ""},
    {2, FAKE_PAUSE_STRING},

    {0, "          -max maximum_IDAT_size [default "STRNGIFY(MAX_IDAT_SIZE)"]"},
    {2, ""},

#ifdef PNGCRUSH_LOCO
    {0, "          -mng (write a new MNG, do not crush embedded PNGs)"},
    {2, ""},
#endif

    {0, "            -n (no save; doesn't do compression or write output PNG)"},
    {2, ""},
    {2, "               Useful in conjunction with -v option to get info."},
    {2, ""},

    {0, "          -new (Use new default settings (-reduce))"},
    {2, ""},

    {0, " -newtimestamp (Reset file modification time [default])"},
    {2, ""},

    {0, "       -nobail (do not bail out early from trial -- see \"-bail\")"},
    {2, ""},
    {2, "               Use this if you want to get a report of the"},
    {2, "               exact filesize achieved by each trial."},
    {2, ""},

    {0, "      -nocheck (do not check CRC and ADLER32 checksums)"},
    {2, ""},
    {2, "               Use \"-check\" to check them"},
    {2, ""},

    {0, FAKE_PAUSE_STRING},

    {0, "  -nofilecheck (do not check for infile.png == outfile.png)"},
    {2, ""},
    {2, "               To avoid false hits from MSVC-compiled code.  Note"},
    {2, "               that if you use this option, you are responsible for"},
    {2, "               ensuring that the input file is not the output file."},
    {2, ""},

    {0, "      -noforce (do not write output when IDAT is not decreased)"},
    {2, ""},

    {0, "     -nolimits (turns off limits on width, height, cache, malloc)"},
    {2, ""},
    {2, "               Instead, the user limits are inherited from libpng."},
    {2, ""},

    {0, "     -noreduce (turns off all \"-reduce\" operations)"},
    {2, ""},

    {0, "-noreduce_palette (turns off \"-reduce_palette\" operation)"},
    {2, ""},

    {0, "          -old (Use old default settings (no -reduce))"},
    {2, ""},

    {0, " -oldtimestamp (Do not reset file modification time)"},
    {2, ""},

    {0, "           -ow (Overwrite)"},
    {2, ""},
    {2, "               Overwrite the input file.  The input file is removed"},
    {2, "               and the temporary file (default \"pngout.png\")"},
    {2, "               is renamed to the input file after recompression"},
    {2, "               and therefore they must reside on the same"},
    {2, "               filesystem."},
    {2, ""},
    {2, "               CAUTION: If you are running multiple instances"},
    {2, "               of pngcrush in parallel, you must specify a"},
    {2, "               different temporary filename for each instance,"},
    {2, "               to avoid collisions."},
    {2, ""},

    {0, "            -q (quiet) suppresses console output except for warnings"},
    {2, ""},
    {2, "               and summary of results."},
    {2, ""},

    {0, "       -reduce (do lossless color-type or bit-depth reduction)"},
    {2, ""},
    {2, "               (if possible).  Also reduces palette length if"},
    {2, "               possible.  Currently only attempts to reduce the"},
    {2, "               bit depth from 16 to 8.  Reduces all-gray RGB"},
    {2, "               or RGBA image to gray or gray-alpha.  Reduces"},
    {2, "               all-opaque RGBA or GA image to RGB or grayscale."},
    {2, "               Since pngcrush version 1.8.0, -reduce is on by"},
    {2, "               default, and you can disable it with -noreduce."},
    {2, ""},

    {0, "          -rem chunkname (or \"alla\" or \"allb\")"},
    {2, ""},
    {2, "               Name of an ancillary chunk or optional PLTE to be"},
    {2, "               removed.  Be careful with this.  Don't use this"},
    {2, "               feature to remove transparency, gamma, copyright,"},
    {2, "               or other valuable information.  To remove several"},
    {2, "               different chunks, repeat: -rem tEXt -rem pHYs."},
    {2, "               Known chunks (those in the PNG 1.1 spec or extensions"},
    {2, "               document) can be named with all lower-case letters,"},
    {2, "               so \"-rem bkgd\" is equivalent to \"-rem bKGD\".  But"},
    {2, "               note: \"-rem text\" removes all forms of text chunks;"},
    {2, "               Exact case is required to remove unknown chunks."},
    {2, "               To do surgery with a chain-saw, \"-rem alla\" removes"},
    {2, "               all known ancillary chunks except for tRNS, and"},
    {2, "               \"-rem allb\" removes all but tRNS and gAMA."},
    {2, ""},

    {0, FAKE_PAUSE_STRING},

#ifdef PNG_FIXED_POINT_SUPPORTED
    {0, "-replace_gamma gamma (float or fixed*100000) even if it is present."},
#else
    {0, "-replace_gamma gamma (float, e.g. 0.45455) even if it is present."},
#endif
    {2, ""},

    {0, "          -res resolution in dpi"},
    {2, ""},
    {2, "               Write a pHYs chunk with the given resolution in dpi"},
    {2, "               written as pixels per meter in x and y directions."},
    {2, ""},

#ifdef Z_RLE
    {0, "          -rle (use only zlib strategy 3, RLE-only)"},
    {2, ""},
    {2, "               A relatively fast subset of the \"-brute\" methods,"},
    {2, "               generally more effective than \"-huffman\" on PNG,"},
    {2, "               images (and quite effective on black-and-white"},
    {2, "               images) but not necessarily worth the bother"},
    {2, "               otherwise."},
    {2, ""},
#endif

    {0, "            -s (silent) suppresses console output including warnings"},
    {2, ""},
    {2, "               benchmark timing, and summary of results."},
    {2, "               (Use \"-warn\" to show only warnings"},
    {2, ""},

    {0, "         -save (keep all copy-unsafe PNG chunks)"},
    {2, ""},
    {2, "               Save otherwise unknown ancillary chunks that would"},
    {2, "               be considered copy-unsafe.  This option makes"},
    {2, "               chunks 'known' to pngcrush, so they can be copied."},
    {2, "               It also causes the dSIG chunk to be saved, even when"},
    {2, "               it becomes invalid due to datastream changes."},
    {2, "               This option does not affect APNG chunks. These"},
    {2, "               chunks (acTL, fcTL, and fdAT) will be saved only"},
    {2, "               if the output file has the \".apng\" extension"},
    {2, "               and the color_type and bit_depth are not changed."},
    {2, ""},

    {0, FAKE_PAUSE_STRING},

    {0, "        -speed Avoid the AVG and PAETH filters, for decoding speed"},
    {2, ""},
    {2, "               Useful for compressing PNG files that are expected"},
    {2, "               to be cached or otherwise to exist on the computer"},
    {2, "               where they will be used rather than being downloaded,"},
    {2, "               so filesize is therefore less important than CPU"},
    {2, "               time expended in defiltering."},
    {2, ""},

    {0, "         -srgb [0, 1, 2, or 3]"},
    {2, ""},
    {2, "               Value of 'rendering intent' for sRGB chunk."},
    {2, ""},

    {0, "         -ster [0 or 1]"},
    {2, ""},
    {2, "               Value of 'stereo mode' for sTER chunk."},
    {2, "               0: cross-fused; 1: divergent-fused"},
    {2, ""},

    {0, "         -text b[efore_IDAT]|a[fter_IDAT] \"keyword\" \"text\""},
    {2, ""},
    {2, "               tEXt chunk to insert.  keyword < 80 chars,"},
    {2, "               text < 2048 chars. For now, you can add no more than"},
    {2, "               ten tEXt, iTXt, or zTXt chunks per pngcrush run."},
    {2, ""},

#ifdef PNG_tRNS_SUPPORTED
    {0, "   -trns_array n trns[0] trns[1] .. trns[n-1]"},
    {2, ""},
    {2, "               Insert a tRNS chunk, if no tRNS chunk found in file."},
    {2, "               Values are for the tRNS array in indexed-color PNG."},
    {2, ""},

    {0, "         -trns index red green blue gray"},
    {2, ""},
    {2, "               Insert a tRNS chunk, if no tRNS chunk found in file."},
    {2, "               You must give all five parameters regardless of the"},
    {2, "               color type, scaled to the output bit depth."},
    {2, ""},
#endif

    {0, FAKE_PAUSE_STRING},

    {0, "            -v (display more detailed information)"},
    {2, ""},
    {2, "               Repeat the option (use \"-v -v\") for even more."},
    {2, ""},

    {0, "      -version (display the pngcrush version)"},
    {2, ""},
    {2, "               Look for the most recent version of pngcrush at"},
    {2, "               http://pmt.sf.net"},
    {2, ""},

    {0, "         -warn (only show warnings)"},
    {2, ""},

    {0, "            -w compression_window_size [32, 16, 8, 4, 2, 1, 512]"},
    {2, ""},
    {2, "               Size of the sliding compression window, in kbytes"},
    {2, "               (or bytes, in case of 512).  It's best to"},
    {2, "               use the default (32) unless you run out of memory."},
    {2, "               The program will use a smaller window anyway when"},
    {2, "               the uncompressed file is smaller than 16k."},
    {2, ""},

#ifdef Z_RLE
    {0, "            -z zlib_strategy [0, 1, 2, or 3] for specified method"},
#else
    {0, "            -z zlib_strategy [0, 1, or 2] for specified method"},
#endif
    {2, ""},
    {2, "               zlib compression strategy to use with the preceding"},
    {2, "               '-m method' argument."},
    {2, ""},

    {0, "         -zmem zlib_compression_mem_level [1-9, default 9]"},
    {2, ""},

#ifdef PNG_iTXt_SUPPORTED
    {0, "        -zitxt b|a \"keyword\" \"lcode\" \"tkey\" \"text\""},
    {2, ""},
    {2, "               (where \"lcode\"==language_code and"},
    {2, "                \"tkey\"==translated_keyword)\""},
    {2, "               Compressed iTXt chunk to insert (see -text)."},
    {2, ""},
#endif

    {0, "         -ztxt b[efore_IDAT]|a[fter_IDAT] \"keyword\" \"text\""},
    {2, ""},
    {2, "               zTXt chunk to insert (see -text)."},
    {2, ""},
    {2, FAKE_PAUSE_STRING},

    {0, "            -h (help and legal notices)"},
    {2, ""},
    {2, "               Display this information."},
    {2, ""},

    {0, "            -p (pause)"}
};





void print_usage(int retval)
{
    int j, jmax;

    if (verbose > 0)
    {
        jmax = sizeof(pngcrush_legal) / sizeof(char *);
        for (j = 0;  j < jmax;  ++j)
            fprintf(STDERR, "%s\n", pngcrush_legal[j]);

        jmax = sizeof(pngcrush_usage) / sizeof(char *);
        for (j = 0;  j < jmax;  ++j)
            fprintf(STDERR, pngcrush_usage[j], progname);  /* special case */
    }

    /* This block is also handled specially due to the "else" clause... */
    if (verbose > 1)
    {
        pngcrush_pause();
        fprintf(STDERR,
          "\n"
          "options (Note: any option can be spelled out for clarity, e.g.,\n"
          "          \"pngcrush -dir New -method 7 -remove bkgd *.png\"\n"
          "          is the same as \"pngcrush -d New/ -m 7 -rem bkgd *.png\"):"
          "\n\n");
    }
    else
        fprintf(STDERR, "options:\n");

    /* This is the main part of the help screen; it is more complex than the
     * other blocks due to the mix of verbose and non-verbose lines
     */
    jmax = sizeof(pngcrush_options) / sizeof(struct options_help);
    for (j = 0;  j < jmax;  ++j)
    {
        if (verbose >= pngcrush_options[j].verbosity)
        {
            if (pngcrush_options[j].textline[0] == FAKE_PAUSE_STRING[0])
                pngcrush_pause();
            else
                fprintf(STDERR, "%s\n", pngcrush_options[j].textline);
        }
    }

    /* due to progname, the verbose part of the -p option is handled explicitly
     * (fortunately, it's the very last option anyway)
     */
    if (verbose > 1)
    {
        fprintf(STDERR, "\n"
          "               Wait for [enter] key before continuing display.\n"
          "               e.g., type '%s -pause -help', if the help\n"
          "               screen scrolls out of sight.\n\n", progname);
    }

    exit(retval);
}

#endif /* PNGCRUSH_LIBPNG_VER < 10800 || defined(PNGCRUSH_H) */
