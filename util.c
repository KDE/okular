/*
 * Copyright (c) 1994 Paul Vojta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * NOTE:
 *	xdvi is based on prior work as noted in the modification history, below.
 */

/*
 * DVI previewer for X.
 *
 * Eric Cooper, CMU, September 1985.
 *
 * Code derived from dvi-imagen.c.
 *
 * Modification history:
 * 1/1986	Modified for X.10	--Bob Scheifler, MIT LCS.
 * 7/1988	Modified for X.11	--Mark Eichin, MIT
 * 12/1988	Added 'R' option, toolkit, magnifying glass
 *					--Paul Vojta, UC Berkeley.
 * 2/1989	Added tpic support	--Jeffrey Lee, U of Toronto
 * 4/1989	Modified for System V	--Donald Richardson, Clarkson Univ.
 * 3/1990	Added VMS support	--Scott Allendorf, U of Iowa
 * 7/1990	Added reflection mode	--Michael Pak, Hebrew U of Jerusalem
 * 1/1992	Added greyscale code	--Till Brychcy, Techn. Univ. Muenchen
 *					  and Lee Hetherington, MIT
 * 4/1994	Added DPS support, bounding box
 *					--Ricardo Telichevesky
 *					  and Luis Miguel Silveira, MIT RLE.
 */

#include "oconfig.h"
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-vararg.h>

#ifdef	Mips
extern	int	errno;
#endif

#ifdef VMS
#include <rmsdef.h>
#endif /* VMS */


/*
 *	General utility routines.
 */

/*
 *	Print error message and quit.
 */

#if	NeedVarargsPrototypes
NORETURN void
oops(_Xconst char *message, ...)
#else
/* VARARGS */
NORETURN void
oops(va_alist)
	va_dcl
#endif
{
#if	!NeedVarargsPrototypes
	_Xconst char *message;
#endif
	va_list	args;

	Fprintf(stderr, "%s: ", prog);
#if	NeedVarargsPrototypes
	va_start(args, message);
#else
	va_start(args);
	message = va_arg(args, _Xconst char *);
#endif
	(void) vfprintf(stderr, message, args);
	va_end(args);
	Putc('\n', stderr);
	exit(1);
}

/*
 *	Either allocate storage or fail with explanation.
 */

char *
xmalloc(size, why)
	unsigned	size;
	_Xconst char	*why;
{
	/* Avoid malloc(0), though it's not clear if it ever actually
	   happens any more.  */
	char *mem = malloc(size ? size : 1);

	if (mem == NULL)
	    oops("! Cannot allocate %u bytes for %s.\n", size, why);
	return mem;
}

/*
 *	Allocate bitmap for given font and character
 */

void
alloc_bitmap(bitmap)
	register struct bitmap *bitmap;
{
	register unsigned int	size;

	/* width must be multiple of 16 bits for raster_op */
	bitmap->bytes_wide = ROUNDUP((int) bitmap->w, BITS_PER_BMUNIT) *
	    BYTES_PER_BMUNIT;
	size = bitmap->bytes_wide * bitmap->h;
	bitmap->bits = xmalloc(size != 0 ? size : 1, "character bitmap");
}


/*
 *	Close the pixel file for the least recently used font.
 */

static	void
close_a_file()
{
	register struct font *fontp;
	unsigned short oldest = ~0;
	struct font *f = NULL;

	for (fontp = font_head; fontp != NULL; fontp = fontp->next)
	    if (fontp->file != NULL && fontp->timestamp <= oldest) {
		f = fontp;
		oldest = fontp->timestamp;
	    }
	if (f == NULL)
	    oops("Can't find an open pixel file to close");
	Fclose(f->file);
	f->file = NULL;
	++n_files_left;
}

/*
 *	Open a file in the given mode.
 */

FILE *
#ifndef	VMS
xfopen(filename, type)
	_Xconst char	*filename;
	_Xconst char	*type;
#define	TYPE	type
#else
xfopen(filename, type, type2)
	_Xconst char	*filename;
	_Xconst char	*type;
	_Xconst char	*type2;
#define	TYPE	type, type2
#endif	/* VMS */
{
	FILE	*f;

        /* Try not to let the file table fill up completely.  */
	if (n_files_left <= 5)
	  close_a_file();
	f = fopen(filename, OPEN_MODE);
	/* If the open failed, try closing a file unconditionally.
  	   Interactive Unix 2.2.1, at least, doesn't set errno to EMFILE
  	   or ENFILE even when it should.  In any case, it doesn't hurt
  	   much to always try.  */
	if (f == NULL)
	{
	    n_files_left = 0;
	    close_a_file();
	    f = fopen(filename, TYPE);
	}
	return f;
}
#undef	TYPE


#ifdef	PS_GS
/*
 *	Create a pipe, closing a file if necessary.  This is (so far) used only
 *	in psgs.c.
 */

int
xpipe(fd)
	int	*fd;
{
	int	retval;

	for (;;) {
	    retval = pipe(fd);
	    if (retval == 0 || (errno != EMFILE && errno != ENFILE)) break;
	    n_files_left = 0;
	    close_a_file();
	}
	return retval;
}
#endif	/* PS_GS */


/*
 *
 *      Read size bytes from the FILE fp, constructing them into a
 *      signed/unsigned integer.
 *
 */

unsigned long
num(fp, size)
	register FILE *fp;
	register int size;
{
	register long x = 0;

	while (size--) x = (x << 8) | one(fp);
	return x;
}

long
snum(fp, size)
	register FILE *fp;
	register int size;
{
	register long x;

#ifdef	__STDC__
	x = (signed char) getc(fp);
#else
	x = (unsigned char) getc(fp);
	if (x & 0x80) x -= 0x100;
#endif
	while (--size) x = (x << 8) | one(fp);
	return x;
}
