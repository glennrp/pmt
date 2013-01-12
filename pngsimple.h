/*- pngsimple.h
 *
 * Proposed prototypes for a simpler interface to libpng.
 *
 * Date: 20111107
 * Copyright:  Published in the public domain for comment and use without
 *             restriction.
 * Original copyright (c) John Bowler <jbowler@acm.org>
 *
 * 1) INTRODUCTION
 * ---------------
 * These APIs provide support for reading writing a limited number of in-memory
 * bitmap formats from and to the PNG format.  They hide the details of the
 * necessary transformations and the libpng error handling.
 *
 * The supported formats are limited to 8-bit RGB or RGBA data encoded according
 * to the sRGB specification and 16-bit RGBA data using the sRGB color space
 * with a linear encoding.  The 8-bit formats are intended for image display and
 * distribution, the 16-bit format for real-world image data used as the input
 * or output of image processing.
 *
 * The APIs use a common control structure, 'png_image', to describe the
 * in-memory format and to hold libpng control data via a pointer to an opaque
 * structure of type 'png_controlp'.  The structure should be allocated on the
 * stack or in the data of an appropriate object.
 *
 * 1.2) Reading an existing PNG image
 * ----------------------------------
 * Reading an existing image requires two calls to the libpng APIs, the first to
 * find the size of the PNG image and the second to decode the image into the
 * application provided buffer.  The application should take the following
 * steps:
 *
 * 1.2.1) Initialize the png_controlp member of a png_image to NULL.
 * 1.2.2) Call one of the png_image_begin_read functions to fill in the
 * remainder of the png_image structure.
 * 1.2.3) Change the png_image 'format' parameter to the desired in-memory
 * format and allocate sufficient memory to hold the complete image.
 * 1.2.4) Call png_image_finish_read to read the PNG image into the supplied
 * buffer.
 *
 * At any step the application can abort the image read by calling
 * png_image_free to release any data held in the png_controlp.
 *
 * 1.3 Writing a new PNG image
 * ---------------------------
 * Writing a new PNG image can be accomplished with a single call to libpng:
 *
 * 1.3.1) Initialize a png_image structure with a description of the in-memory
 * format and a NULL png_controlp.
 * 1.3.2) Call one of the png_image_write functions with a pointer to the
 * in-memory bitmap.
 *
 * The png_controlp member is used by libpng during the write but will be NULL
 * on exit.  Nevertheless is must be initialized to NULL to allow for future
 * extensions.
 *
 * 2) STRUCTURES
 * -------------
 *
 * 2.1) struct png_control
 * -----------------------
 * An opaque structure used during the read or write of a PNG image.
 */
typedef struct png_control *png_controlp;

/* 2.2) png_image
 * --------------
 * A structure containing a description of an in-memory bitmap plus control
 * information (png_controlp) used by libpng while reading or writing the bitmap
 * from or to the PNG format.
 *
 * On read the information is filled in from the information in the PNG file
 * header.  This includes a suggested memory format, but this will normally be
 * changed by the application to the format required.
 *
 * On write the information is filled in by the application before calling
 * libpng.
 */
typedef struct
{
   png_uint_32  width;  /* Image width in pixels (columns) */
   png_uint_32  height; /* Image height in pixels (rows) */
   png_uint_32  format; /* Image format as defined below */
   png_uint_32  flags;  /* A bit mask containing informational flags */
   png_controlp opaque; /* Initialize to NULL, free with png_image_free */

   /* In the event of an error or warning the following field will be set to a
    * non-zero value and the 'message' field will contain a '\0' terminated
    * string with the libpng error message.  The error message will be truncated
    * to 63 characters if necessary.
    */
   png_uint_32  warning_or_error;
   char         message[64];
} png_image, *png_imagep;

/* The pixels (samples) of the image have one to four channels in the range 0 to
 * 1.0:
 *
 * 1: A single gray or luminance channel (G).
 * 2: A gray/luminance channel and an alpha channel (GA).
 * 3: Three red, green, blue color channels (RGB).
 * 4: Three color channels and an alpha channel (RGBA).
 *
 * The channels are encoded in one of two ways:
 *
 * a) As a small integer, value 0..255, contained in a (png_byte).  For the
 * alpha channel the original value is simply value/255.  For the color or
 * luminance channels the value is encoded according to the sRGB specification
 * and matches the 8-bit format expected by typical display devices.
 *
 *    The color/gray channels are not scaled (pre-multiplied) by the alpha
 * channel and are suitable for passing to color management software.
 *
 * b) As a value in the range 0..65535, contained in a (png_uint_16).  All
 * channels can be converted to the original value by dividing by 65535; all
 * channels are linear.  Color channels use the RGB encoding (RGB end-points) of
 * the sRGB specification.  This encoding is identified by the
 * PNG_FORMAT_FLAG_LINEAR flag below.
 *
 *    When an alpha channel is present it is expected to denote pixel coverage
 * of the color or luminance channels and is returned as an associated alpha
 * channel: the color/gray channels are scaled (pre-multiplied) the alpha value.
 */

/* PNG_FORMAT_*
 *
 * #defines to be used in png_image::format.  Each #define identifies a
 * particular layout of channel data and, if present, alpha values.  There are
 * separate defines for each of the two channel encodings.
 *
 * A format is built up using single bit flag values.  Not all combinations are
 * valid: use the bit flag values below for testing a format returned by the
 * read APIs, but set formats from the derived values.
 *
 * First basic desciptions of the channels:
 */
#define PNG_FORMAT_FLAG_ALPHA    0x01 /* format with an alpha channel */
#define PNG_FORMAT_FLAG_COLOR    0x02 /* color format: otherwise grayscale */
#define PNG_FORMAT_FLAG_LINEAR   0x04 /* png_uint_16 channels else png_byte */

/* Then layout descriptions.  The implementation will ignore flags below that
 * aren't applicable given the flags above, but the application should use the
 * pre-defined constants below because some combinations of the flags may not be
 * supported.
 *
 * NOTE: libpng can be built with particular features disabled, if you see
 * compiler errors because the definition of one of the following flags has been
 * compiled out it is because libpng does not have the required support.  It is
 * possible, however, for the libpng configuration to enable the format on just
 * read or just write, in that case you will may see an error at run time.  You
 * can guard against this by checking for the definition of:
 *
 *    PNG_SIMPLIFIED_{READ,WRITE}_{BGR,AFIRST}_SUPPORTED
 */
#ifdef PNG_FORMAT_BGR_SUPPORTED
#  define PNG_FORMAT_FLAG_BGR    0x08 /* BGR colors, else order is RGB */
#endif

#ifdef PNG_FORMAT_AFIRST_SUPPORTED
#  define PNG_FORMAT_FLAG_AFIRST 0x10 /* alpha channel comes first */
#endif

/* Supported formats are as follows.  Future versions of libpng may support more
 * formats, for compatibility with older versions simply check if the format
 * macro is defined using #ifdef.  These defines describe the in-memory layout
 * of the components of the pixels of the image.
 *
 * First the single byte formats:
 */
#define PNG_FORMAT_GRAY 0
#define PNG_FORMAT_GA   PNG_FORMAT_FLAG_ALPHA
#define PNG_FORMAT_AG   (PNG_FORMAT_GA|PNG_FORMAT_FLAG_AFIRST)
#define PNG_FORMAT_RGB  PNG_FORMAT_FLAG_COLOR
#define PNG_FORMAT_BGR  (PNG_FORMAT_FLAG_COLOR|PNG_FORMAT_FLAG_BGR)
#define PNG_FORMAT_RGBA (PNG_FORMAT_RGB|PNG_FORMAT_FLAG_ALPHA)
#define PNG_FORMAT_ARGB (PNG_FORMAT_RGBA|PNG_FORMAT_FLAG_AFIRST)
#define PNG_FORMAT_BGRA (PNG_FORMAT_BGR|PNG_FORMAT_FLAG_ALPHA)
#define PNG_FORMAT_ABGR (PNG_FORMAT_BGRA|PNG_FORMAT_FLAG_AFIRST)

/* Then the linear (png_uint_16) formats.  When naming these "Y" is used to
 * indicate a luminance channel.  The component order within the pixel is
 * always the same - there is no provision for swapping the order of the
 * components in the linear format.
 */
#define PNG_FORMAT_LINEAR_Y PNG_FORMAT_FLAG_LINEAR
#define PNG_FORMAT_LINEAR_Y_ALPHA (PNG_FORMAT_FLAG_LINEAR|PNG_FORMAT_FLAG_ALPHA)
#define PNG_FORMAT_LINEAR_RGB (PNG_FORMAT_FLAG_LINEAR|PNG_FORMAT_FLAG_COLOR)
#define PNG_FORMAT_LINEAR_RGB_ALPHA \
   (PNG_FORMAT_FLAG_LINEAR|PNG_FORMAT_FLAG_COLOR|PNG_FORMAT_FLAG_ALPHA)

/* PNG_IMAGE macros
 *
 * These are convenience macros to derive information from a png_image structure
 */
#define PNG_IMAGE_CHANNELS(fmt)\
   (1+((fmt)&(PNG_FORMAT_FLAG_COLOR|PNG_FORMAT_FLAG_ALPHA)))
   /* Return the total number of channels in a given format: 1..4 */

#define PNG_IMAGE_COMPONENT_SIZE(fmt)\
   (((fmt) & PNG_FORMAT_FLAG_LINEAR) ? sizeof (png_uint_16) : sizeof (png_byte))
   /* Return the size in bytes of a single component of a pixel in the image. */

#define PNG_IMAGE_PIXEL_SIZE(fmt)\
   (PNG_IMAGE_CHANNELS(fmt) * PNG_IMAGE_COMPONENT_SIZE(fmt))
   /* Return the size in bytes of a single pixel in the image. */
   
#define PNG_IMAGE_ROW_STRIDE(image)\
   (PNG_IMAGE_CHANNELS((image).format) * (image).width)
   /* Return the total number of components in a single row of the image; this
    * is the minimum 'row stride', the minimum count of components between each
    * row.  */

#define PNG_IMAGE_BUFFER_SIZE(image, row_stride)\
   (PNG_IMAGE_COMPONENT_SIZE((image).format) * (image).height * (row_stride))
   /* Return the size, in bytes, of an image buffer given a png_image and a row
    * stride - the number of components to leave space for in each row. */

/* PNG_IMAGE_FLAG_*
 *
 * Flags containing additional information about the image are held in the
 * 'flags' field of png_image.
 */
#define PNG_IMAGE_FLAG_COLORSPACE_NOT_sRGB 1
   /* This indicates the the RGB values of the in-memory bitmap do not
    * correspond to the red, green and blue end-points defined by sRGB.  If this
    * flag is set on read the PNG file contained information which identified a
    * different colorspace.  The application should set the flag on write if it
    * knows that the data being written has different end-points.
    *
    * When writing 8-bit format files libpng will write an 'sRGB' chunk, to
    * indicate sRGB data, unless this flag is set.  When writing 16-bit format
    * files libpng will write color space information matching the sRGB
    * end-points unless the flag is set.
    *
    * Regardless of the setting of this flag the gamma encoding is always either
    * linear or (approximately) sRGB; a gamma of about 2.2
    */

#ifdef PNG_SIMPLIFIED_READ_SUPPORTED
/* 3) READ APIs
 * ------------
 *
 * The png_image passed to the read APIs must have been initialized by setting
 * the png_controlp field 'opaque' to NULL.
 */
#ifdef PNG_STDIO_SUPPORTED
int png_image_begin_read_from_file(png_imagep image, const char *file_name);
   /* The named file is opened for read and the image filled in from the PNG
    * header in the file, the file may remain open until the image is read from
    * it.
    */

int png_image_begin_read_from_stdio(png_imagep image, FILE* file);
   /* The PNG header is read from the stdio FILE object.  The FILE must be open
    * for read and positioned at the start of the PNG data (the signature), the
    * FILE will be retained until the image has been read from it, however it is
    * not closed.  After a successful image read the FILE will point to just
    * after the end of the PNG data.
    */
#endif

int png_image_begin_read_from_memory(png_imagep image, png_const_voidp memory,
   png_size_t size);
   /* The PNG header is read from the given memory buffer.  A pointer to the
    * buffer is retained until the image has been read from it - the buffer is
    * not copied.
    */

/* After initialization all members of the png_image structure will have been
 * filled in.  In the event of error the APIs return false and fill in
 * png_image::warning_or_error and png_image::message with information about the
 * nature of the error.
 *
 * After a successful return (true) the application can immediately call
 * png_image_finish_read (below) with an appropriately sized buffer.  More
 * usually, however, the application will change the png_image::format field to
 * the desired format.
 *
 * The initialization APIs fill in png_image::format as follows:
 *
 * 1) For any PNG image with a component bit depth of 16 PNG_FORMAT_FLAG_LINEAR
 * is set in the result.  PNG_FORMAT_FLAG_RGB and PNG_FORMAT_FLAG_ALPHA are
 * added as appropriate.
 *
 * 2) For any other image with an alpha channel or with transparency information
 * PNG_FORMAT_RGBA or PNG_FORMAT_GA is returned.
 *
 * 3) For all other images PNG_FORMAT_RGB or PNG_FORMAT_GRAY is returned.
 *
 * The presence of alpha/transparency information can be detected by using a
 * bitwise 'and' for the format value with PNG_FORMAT_FLAG_ALPHA.  The presence
 * of color channels (as opposed to a single gray/luminance channel) can be
 * detected using PNG_FORMAT_FLAG_COLOR.
 *
 * The format field may be changed after the first read API has been called to
 * any of the formats defined above.
 *
 * png_image::flags will be set to indicate potential problems decoding the
 * image to one of the in-memory formats.  At present only one flag is set:
 *
 * PNG_IMAGE_FLAG_COLORSPACE_NOT_sRGB
 *    If set this indicates that the PNG file contains color data with end
 *    points that do not match those used in the sRGB specification, for example
 *    if the file uses one of the wide gamut color spaces.  The field is only
 *    set if a cHRM chunk or iCCP chunk was present, the image was in a color
 *    format (not grayscale) and the cHRM chunk or (if absent) the iCCP chunk
 *    did not match the sRGB color end points.  The gamma encoding of the PNG
 *    file is irrelevant - it is always converted to either linear (for the
 *    16-bit format) or a gamma approximating sRGB.
 *
 * To read the image the application must first allocate or identify a buffer
 * large enough for the whole image in the desired format.  If the output format
 * will not contain an alpha channel yet the format from the PNG file does have
 * alpha information (PNG_FORMAT_FLAG_ALPHA is set) the application must decide
 * how to remove the alpha channel:
 *
 * 1) For 16-bit linear output the alpha channel is simply stripped - the
 * remaining channels contain the pre-multiplied component values.  This is
 * equivalent to compositing the image onto a black background.
 *
 * 2) For 8-bit sRGB output the application can chose to provide an sRGB
 * background color.  The image is composited onto this color.  If the output
 * will only have one channel the sRGB green value is used as the background
 * (gray) color.
 *
 * 3) If a background color is not supplied the application must initialize the
 * output buffer with the background.  The image will be composited directly
 * onto this.
 *
 * Once a buffer has been prepared the application should call:
 */
int png_image_finish_read(png_imagep image, png_colorp background, void *buffer,
   png_int_32 row_stride);
   /* Finish reading the image into the supplied buffer and clean up the
    * png_image structure.
    *
    * row_stride is the step, in png_byte or png_uint_16 units as appropriate,
    * between adjacent rows.  A positive stride indicates that the top-most row
    * is first in the buffer - the normal top-down arrangement.  A negative
    * stride indicates that the bottom-most row is first in the buffer.
    *
    * background need only be supplied if an alpha channel must be removed from
    * a png_byte format and the removal is to be done by compositing on a solid
    * color; otherwise it may be NULL and any composition will be done directly
    * onto the buffer.  The value is an sRGB color to use for the background,
    * for grayscale output the green channel is used.
    *
    * For linear output removing an alpha channel is always done by compositing
    * on black.
    *
    * The buffer must be large enough to contain the whole image -
    * height*row_stride pixels.  Each pixel will contain between 1 and 4
    * png_byte or png_uint_16 values.  Use the PNG_IMAGE_ macros above to
    * perform the required calculations.
    *
    * After png_image_finish_read returns the png_image::opaque structure left
    * by the initialization routine will have been cleaned up; the pointer will
    * be NULL.  If you don't call png_image_finish_read for some reason,
    * however, simply call:
    */
#endif

void png_image_free(png_imagep image);
   /* Free any data allocated by libpng in image->opaque, setting the pointer to
    * NULL.  May be called at any time after the structure is initialized.  It
    * is not necessary to use this with the write API, but it can be called
    * because the opaque pointer must be initialized to NULL for write.
    *
    * The function does not change, or use, any other field in the png_image it
    * is passed unless an error or warning occurs, in which case the error
    * fields will be set.
    */

#ifdef PNG_SIMPLIFIED_WRITE_SUPPORTED
/* 4) WRITE APIS
 * -------------
 * For write you must initialize a png_image structure.  It is suggested that
 * you use memset(&image, 0, sizeof image) to ensure safe initialization.  Doing
 * this will also ensure that extensions to png_image in the future will be
 * safe.
 *
 * You need to explicitly initialize the following fields:
 *
 * opaque: must be initialized to NULL
 * width: image width in pixels
 * height: image height in rows
 * format: the format of the data you wish to write
 * flags: set to 0 unless one of the defined flags applies; set
 *    PNG_IMAGE_FLAG_COLORSPACE_NOT_sRGB for color format images where the RGB
 *    values do not correspond to the colors in sRGB.
 *
 * At present STDIO support is required for the write API and write only
 * supports output to a named file or a stdio stream (FILE*).  Once the
 * png_image structure is initialized call one of:
 */
#ifdef PNG_STDIO_SUPPORTED
int png_image_write_to_file(png_imagep image, const char *file,
   int convert_to_8bit, const void *buffer, png_int_32 row_stride);
   /* Write the image to the named file.  The file will be opened for write and
    * truncated.  In the event of error the API will attempt to remove a
    * partially written file (but it will not attempt to remove a file if the
    * open fails.)  In the event of success the file will have been closed
    * before the API returns.  The API does not fsync() the file.
    */

int png_image_write_to_stdio(png_imagep image, FILE *file,
   int convert_to_8_bit, const void *buffer, png_int_32 row_stride);
   /* Write the image to the given (FILE*).  This is only supported on operating
    * systems with stdio support.  The PNG data is written to the given FILE*
    * using the standard stdio function fwrite(), fflush() will not necessarily
    * be called and the file pointer will be left at the end of the PNG data.
    */
#endif

/* With all APIs if image->format is PNG_FORMAT_FP then setting convert_to_8_bit
 * will cause the output to be an 8-bit RGBA PNG gamma encoded according to the
 * sRGB specification.  If the flag is not set PNG_FORMAT_FP is written as a
 * linear 16-bit component depth PNG.  In either case the color components and
 * alpha channel, if present, are converted to match the PNG format.
 *
 * With all APIs row_stride is handled as in the read APIs - it is the spacing
 * from one row to the next in component sized units (float) and if negative
 * indicates a bottom-up row layout in the buffer.
 *
 * All APIs clear the error handling fields, however the opaque pointer must
 * have been initialized to NULL.  Future extensions may used this field to
 * communicate information to a subsequent write.  You may call png_image_free
 * on the png_image, but it is not currently necessary.
 */

/* 5) WRITING TO MEMORY
 * --------------------
 * libpng does not provide a png_image API to write directly to memory.  This is
 * both because of the wide variety of restrictions and variations in efficiency
 * of memory allocators on different platforms and because the simple approach
 * of reallocing a memory block to accomodate the PNG data as it is written is
 * disastrously slow for large PNG files.
 *
 * Instead you may copy and paste the following code into your application,
 * changing the call to 'malloc' if required.  Notice that the size of data
 * required may exceed the capacity of malloc on some older, smaller, systems.
 *
 * The ANSI-C function tmpfile() is used here because it is normally extremely
 * efficient and fast (more so than realloc.)  You can also use png_image_write
 * to write to memory in a different format, for instance a linked list of
 * memory blocks, if this is appropriate.
 */
#ifdef PNG_EXAMPLE_CODE_IN_PNG_H
#include <stdlib.h>
#include <stdio.h>
#include <png.h>

void *png_image_write_to_memory(png_imagep image, size_t *size,
   int convert_to_8bit, const void *buffer, png_int_32 row_stride)
{
   FILE *tmp = tmpfile();
   void *mem = NULL;

   if (tmp != NULL)
   {
      if (png_image_write_to_file(image, tmp, convert_to_8_bit, buffer,
         row_stride))
      {
         long int cb = ftell(tmp);

         image->error_or_warning = 0;

         if (cb > 0)
         {
            /* On some systems (size_t) may be smaller than (long int), in that
             * case you may need to use a function other than malloc to obtain
             * the required memory here, depending on how big the PNG is.
             */
            *size = cb;
            mem = malloc(*size);

            if (mem != NULL)
            {
               rewind(tmp);

               if (fread(mem, *size, 1, tmp) != 1)
               {
                  free(mem);
                  mem = NULL;
               }
            }
         }
      }

      fclose(tmp);
   }

   return mem;
}
#endif
#endif
