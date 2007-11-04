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
/* Definition of error codes */

#ifndef ierrors_INCLUDED
#  define ierrors_INCLUDED

/*
 * DO NOT USE THIS FILE IN THE GRAPHICS LIBRARY.
 * THIS FILE IS PART OF THE POSTSCRIPT INTERPRETER.
 * USE gserrors.h IN THE LIBRARY.
 */

/*
 * A procedure that may return an error always returns
 * a non-negative value (zero, unless otherwise noted) for success,
 * or negative for failure.
 * We use ints rather than an enum to avoid a lot of casting.
 */

/* Define the error name table */
extern const char *const gs_error_names[];

		/* ------ PostScript Level 1 errors ------ */

#define e_unknownerror (-1)	/* unknown error */
#define e_dictfull (-2)
#define e_dictstackoverflow (-3)
#define e_dictstackunderflow (-4)
#define e_execstackoverflow (-5)
#define e_interrupt (-6)
#define e_invalidaccess (-7)
#define e_invalidexit (-8)
#define e_invalidfileaccess (-9)
#define e_invalidfont (-10)
#define e_invalidrestore (-11)
#define e_ioerror (-12)
#define e_limitcheck (-13)
#define e_nocurrentpoint (-14)
#define e_rangecheck (-15)
#define e_stackoverflow (-16)
#define e_stackunderflow (-17)
#define e_syntaxerror (-18)
#define e_timeout (-19)
#define e_typecheck (-20)
#define e_undefined (-21)
#define e_undefinedfilename (-22)
#define e_undefinedresult (-23)
#define e_unmatchedmark (-24)
#define e_VMerror (-25)		/* must be the last Level 1 error */

#define LEVEL1_ERROR_NAMES\
 "unknownerror", "dictfull", "dictstackoverflow", "dictstackunderflow",\
 "execstackoverflow", "interrupt", "invalidaccess", "invalidexit",\
 "invalidfileaccess", "invalidfont", "invalidrestore", "ioerror",\
 "limitcheck", "nocurrentpoint", "rangecheck", "stackoverflow",\
 "stackunderflow", "syntaxerror", "timeout", "typecheck", "undefined",\
 "undefinedfilename", "undefinedresult", "unmatchedmark", "VMerror"

	/* ------ Additional Level 2 errors (also in DPS) ------ */

#define e_configurationerror (-26)
#define e_undefinedresource (-27)
#define e_unregistered (-28)

#define LEVEL2_ERROR_NAMES\
 "configurationerror", "undefinedresource", "unregistered"

	/* ------ Additional DPS errors ------ */

#define e_invalidcontext (-29)
/* invalidid is for the NeXT DPS extension. */
#define e_invalidid (-30)

#define DPS_ERROR_NAMES\
 "invalidcontext", "invalidid"

#define ERROR_NAMES\
 LEVEL1_ERROR_NAMES, LEVEL2_ERROR_NAMES, DPS_ERROR_NAMES

	/* ------ Pseudo-errors used internally ------ */

/*
 * Internal code for a fatal error.
 * gs_interpret also returns this for a .quit with a positive exit code.
 */
#define e_Fatal (-100)

/*
 * Internal code for the .quit operator.
 * The real quit code is an integer on the operand stack.
 * gs_interpret returns this only for a .quit with a zero exit code.
 */
#define e_Quit (-101)

/*
 * Internal code for a normal exit from the interpreter.
 * Do not use outside of interp.c.
 */
#define e_InterpreterExit (-102)

/*
 * Internal code that indicates that a procedure has been stored in the
 * remap_proc of the graphics state, and should be called before retrying
 * the current token.  This is used for color remapping involving a call
 * back into the interpreter -- inelegant, but effective.
 */
#define e_RemapColor (-103)

/*
 * Internal code to indicate we have underflowed the top block
 * of the e-stack.
 */
#define e_ExecStackUnderflow (-104)

/*
 * Internal code for the vmreclaim operator with a positive operand.
 * We need to handle this as an error because otherwise the interpreter
 * won't reload enough of its state when the operator returns.
 */
#define e_VMreclaim (-105)

/*
 * Internal code for requesting more input from run_string.
 */
#define e_NeedInput (-106)

/*
 * Internal code for a normal exit when usage info is displayed.
 * This allows Window versions of Ghostscript to pause until
 * the message can be read.
 */
#define e_Info (-110)

/*
 * Define which error codes require re-executing the current object.
 */
#define ERROR_IS_INTERRUPT(ecode)\
  ((ecode) == e_interrupt || (ecode) == e_timeout)

#endif /* ierrors_INCLUDED */
