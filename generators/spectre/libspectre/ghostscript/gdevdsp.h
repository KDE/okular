/* Copyright (C) 2001-2006 Artifex Software, Inc.
   All Rights Reserved.
  
   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied, modified
   or distributed except as expressly authorized under the terms of that
   license.  Refer to licensing information at http://www.artifex.com/
   or contact Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134,
   San Rafael, CA  94903, U.S.A., +1(415)492-9861, for further information.
*/
/* $Id$ */
/* gdevdsp.h - callback structure for DLL based display device */

#ifndef gdevdsp_INCLUDED
#  define gdevdsp_INCLUDED

/*
 * The callback structure must be provided by calling the
 * Ghostscript APIs in the following order:
 *  gsapi_new_instance(&minst);
 *  gsapi_set_display_callback(minst, callback);
 *  gsapi_init_with_args(minst, argc, argv);
 *
 * Supported parameters and default values are:
 * -sDisplayHandle=16#04d2 or 1234        string
 *    Caller supplied handle as a decimal or hexadecimal number
 *    in a string.  On 32-bit platforms, it may be set
 *    using -dDisplayHandle=1234 for backward compatibility.
 *    Included as first parameter of all callback functions.
 *
 * -dDisplayFormat=0                      long
 *    Color format specified using bitfields below.
 *    Included as argument of display_size() and display_presize()
 * These can only be changed when the device is closed.
 *
 * The second parameter of all callback functions "void *device"
 * is the address of the Ghostscript display device instance.
 * The arguments "void *handle" and "void *device" together
 * uniquely identify an instance of the display device.
 *
 * A typical sequence of callbacks would be
 *  open, presize, memalloc, size, sync, page
 *  presize, memfree, memalloc, size, sync, page
 *  preclose, memfree, close
 * The caller should not access the image buffer:
 *  - before the first sync
 *  - between presize and size
 *  - after preclose
 * If opening the device fails, you might see the following:
 *  open, presize, memalloc, memfree, close
 * 
 */

#define DISPLAY_VERSION_MAJOR 2
#define DISPLAY_VERSION_MINOR 0

#define DISPLAY_VERSION_MAJOR_V1 1 /* before separation format was added */
#define DISPLAY_VERSION_MINOR_V1 0

/* The display format is set by a combination of the following bitfields */

/* Define the color space alternatives */
typedef enum {
    DISPLAY_COLORS_NATIVE	 = (1<<0),
    DISPLAY_COLORS_GRAY  	 = (1<<1),
    DISPLAY_COLORS_RGB   	 = (1<<2),
    DISPLAY_COLORS_CMYK  	 = (1<<3),
    DISPLAY_COLORS_SEPARATION    = (1<<19)
} DISPLAY_FORMAT_COLOR;
#define DISPLAY_COLORS_MASK 0x8000fL

/* Define whether alpha information, or an extra unused bytes is included */
/* DISPLAY_ALPHA_FIRST and DISPLAY_ALPHA_LAST are not implemented */
typedef enum {
    DISPLAY_ALPHA_NONE   = (0<<4),
    DISPLAY_ALPHA_FIRST  = (1<<4),
    DISPLAY_ALPHA_LAST   = (1<<5),
    DISPLAY_UNUSED_FIRST = (1<<6),	/* e.g. Mac xRGB */
    DISPLAY_UNUSED_LAST  = (1<<7)	/* e.g. Windows BGRx */
} DISPLAY_FORMAT_ALPHA;
#define DISPLAY_ALPHA_MASK 0x00f0L

/* Define the depth per component for DISPLAY_COLORS_GRAY, 
 * DISPLAY_COLORS_RGB and DISPLAY_COLORS_CMYK, 
 * or the depth per pixel for DISPLAY_COLORS_NATIVE
 * DISPLAY_DEPTH_2 and DISPLAY_DEPTH_12 have not been tested.
 */
typedef enum {
    DISPLAY_DEPTH_1   = (1<<8),
    DISPLAY_DEPTH_2   = (1<<9),
    DISPLAY_DEPTH_4   = (1<<10),
    DISPLAY_DEPTH_8   = (1<<11),
    DISPLAY_DEPTH_12  = (1<<12),
    DISPLAY_DEPTH_16  = (1<<13)
    /* unused (1<<14) */
    /* unused (1<<15) */
} DISPLAY_FORMAT_DEPTH;
#define DISPLAY_DEPTH_MASK 0xff00L


/* Define whether Red/Cyan should come first, 
 * or whether Blue/Black should come first
 */
typedef enum {
    DISPLAY_BIGENDIAN    = (0<<16),	/* Red/Cyan first */
    DISPLAY_LITTLEENDIAN = (1<<16)	/* Blue/Black first */
} DISPLAY_FORMAT_ENDIAN;
#define DISPLAY_ENDIAN_MASK 0x00010000L

/* Define whether the raster starts at the top or bottom of the bitmap */
typedef enum {
    DISPLAY_TOPFIRST    = (0<<17),	/* Unix, Mac */
    DISPLAY_BOTTOMFIRST = (1<<17)	/* Windows */
} DISPLAY_FORMAT_FIRSTROW;
#define DISPLAY_FIRSTROW_MASK 0x00020000L


/* Define whether packing RGB in 16-bits should use 555
 * or 565 (extra bit for green)
 */
typedef enum {
    DISPLAY_NATIVE_555 = (0<<18),
    DISPLAY_NATIVE_565 = (1<<18)
} DISPLAY_FORMAT_555;
#define DISPLAY_555_MASK 0x00040000L

/* Define the row alignment, which must be equal to or greater than
 * the size of a pointer.
 * The default (DISPLAY_ROW_ALIGN_DEFAULT) is the size of a pointer, 
 * 4 bytes (DISPLAY_ROW_ALIGN_4) on 32-bit systems or 8 bytes 
 * (DISPLAY_ROW_ALIGN_8) on 64-bit systems.
 */
typedef enum {
    DISPLAY_ROW_ALIGN_DEFAULT = (0<<20),
    /* DISPLAY_ROW_ALIGN_1 = (1<<20), */ /* not currently possible */
    /* DISPLAY_ROW_ALIGN_2 = (2<<20), */ /* not currently possible */
    DISPLAY_ROW_ALIGN_4 = (3<<20),
    DISPLAY_ROW_ALIGN_8 = (4<<20),
    DISPLAY_ROW_ALIGN_16 = (5<<20),
    DISPLAY_ROW_ALIGN_32 = (6<<20),
    DISPLAY_ROW_ALIGN_64 = (7<<20)
} DISPLAY_FORMAT_ROW_ALIGN;
#define DISPLAY_ROW_ALIGN_MASK 0x00700000L


#ifndef display_callback_DEFINED
#define display_callback_DEFINED
typedef struct display_callback_s display_callback;
#endif

/*
 * Note that for Windows, the display callback functions are 
 * cdecl, not stdcall.  This differs from those in iapi.h.
 */

struct display_callback_s {
    /* Size of this structure */
    /* Used for checking if we have been handed a valid structure */
    int size;

    /* Major version of this structure  */
    /* The major version number will change if this structure changes. */
    int version_major;

    /* Minor version of this structure */
    /* The minor version number will change if new features are added
     * without changes to this structure.  For example, a new color
     * format.
     */
    int version_minor;

    /* New device has been opened */
    /* This is the first event from this device. */
    int (*display_open)(void *handle, void *device);

    /* Device is about to be closed. */
    /* Device will not be closed until this function returns. */
    int (*display_preclose)(void *handle, void *device);

    /* Device has been closed. */
    /* This is the last event from this device. */
    int (*display_close)(void *handle, void *device);

    /* Device is about to be resized. */
    /* Resize will only occur if this function returns 0. */
    /* raster is byte count of a row. */
    int (*display_presize)(void *handle, void *device,
	int width, int height, int raster, unsigned int format);
   
    /* Device has been resized. */
    /* New pointer to raster returned in pimage */
    int (*display_size)(void *handle, void *device, int width, int height, 
	int raster, unsigned int format, unsigned char *pimage);

    /* flushpage */
    int (*display_sync)(void *handle, void *device);

    /* showpage */
    /* If you want to pause on showpage, then don't return immediately */
    int (*display_page)(void *handle, void *device, int copies, int flush);

    /* Notify the caller whenever a portion of the raster is updated. */
    /* This can be used for cooperative multitasking or for
     * progressive update of the display.
     * This function pointer may be set to NULL if not required.
     */
    int (*display_update)(void *handle, void *device, int x, int y, 
	int w, int h);

    /* Allocate memory for bitmap */
    /* This is provided in case you need to create memory in a special
     * way, e.g. shared.  If this is NULL, the Ghostscript memory device 
     * allocates the bitmap. This will only called to allocate the
     * image buffer. The first row will be placed at the address 
     * returned by display_memalloc.
     */
    void *(*display_memalloc)(void *handle, void *device, unsigned long size);

    /* Free memory for bitmap */
    /* If this is NULL, the Ghostscript memory device will free the bitmap */
    int (*display_memfree)(void *handle, void *device, void *mem);
   
    /* Added in V2 */
    /* When using separation color space (DISPLAY_COLORS_SEPARATION),
     * give a mapping for one separation component.
     * This is called for each new component found.
     * It may be called multiple times for each component.
     * It may be called at any time between display_size
     * and display_close.
     * The client uses this to map from the separations to CMYK
     * and hence to RGB for display.
     * GS must only use this callback if version_major >= 2.
     * The unsigned short c,m,y,k values are 65535 = 1.0.
     * This function pointer may be set to NULL if not required.
     */
    int (*display_separation)(void *handle, void *device,
	int component, const char *component_name,
	unsigned short c, unsigned short m, 
	unsigned short y, unsigned short k);
};

/* This is the V1 structure, before separation format was added */
struct display_callback_v1_s {
    int size;
    int version_major;
    int version_minor;
    int (*display_open)(void *handle, void *device);
    int (*display_preclose)(void *handle, void *device);
    int (*display_close)(void *handle, void *device);
    int (*display_presize)(void *handle, void *device,
	int width, int height, int raster, unsigned int format);
    int (*display_size)(void *handle, void *device, int width, int height, 
	int raster, unsigned int format, unsigned char *pimage);
    int (*display_sync)(void *handle, void *device);
    int (*display_page)(void *handle, void *device, int copies, int flush);
    int (*display_update)(void *handle, void *device, int x, int y, 
	int w, int h);
    void *(*display_memalloc)(void *handle, void *device, unsigned long size);
    int (*display_memfree)(void *handle, void *device, void *mem);
};

#define DISPLAY_CALLBACK_V1_SIZEOF sizeof(struct display_callback_v1_s)

#endif /* gdevdsp_INCLUDED */
