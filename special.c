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
 *	This module is based on prior work as noted below.
 */

/*
 * Support drawing routines for TeXsun and TeX
 *
 *      Copyright, (C) 1987, 1988 Tim Morgan, UC Irvine
 *	Adapted for xdvi by Jeffrey Lee, U. of Toronto
 *
 * At the time these routines are called, the values of hh and vv should
 * have been updated to the upper left corner of the graph (the position
 * the \special appears at in the dvi file).  Then the coordinates in the
 * graphics commands are in terms of a virtual page with axes oriented the
 * same as the Imagen and the SUN normally have:
 *
 *                      0,0
 *                       +-----------> +x
 *                       |
 *                       |
 *                       |
 *                      \ /
 *                       +y
 *
 * Angles are measured in the conventional way, from +x towards +y.
 * Unfortunately, that reverses the meaning of "counterclockwise"
 * from what it's normally thought of.
 *
 * A lot of floating point arithmetic has been converted to integer
 * arithmetic for speed.  In some places, this is kind-of kludgy, but
 * it's worth it.
 */

#include "oconfig.h"
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/line.h>
#include <kpathsea/tex-file.h>

#define	MAXPOINTS	300	/* Max points in a path */
#define	TWOPI		(3.14159265359*2.0)
#define	MAX_PEN_SIZE	7	/* Max pixels of pen width */


static	int	xx[MAXPOINTS], yy[MAXPOINTS];	/* Path in milli-inches */
static	int	path_len = 0;	/* # points in current path */
static	int	pen_size = 1;	/* Pixel width of lines drawn */

static	Boolean	whiten = False;
static	Boolean	shade = False;
static	Boolean	blacken = False;

/* Unfortunately, these values also appear in dvisun.c */
#define	xRESOLUTION	(pixels_per_inch/shrink_factor)
#define	yRESOLUTION	(pixels_per_inch/shrink_factor)


/*
 *	Issue warning messages
 */

static	void
Warning(fmt, msg)
	char	*fmt, *msg;
{
	Fprintf(stderr, fmt, msg);
	(void) fputc('\n', stderr);
}


/*
 *	X drawing routines
 */

#define	toint(x)	((int) ((x) + 0.5))
#define	xconv(x)	(toint(tpic_conv*(x))/shrink_factor + PXL_H)
#define	yconv(y)	(toint(tpic_conv*(y))/shrink_factor + PXL_V)

/*
 *	Draw a line from (fx,fy) to (tx,ty).
 *	Right now, we ignore pen_size.
 */
static	void
line_btw(fx, fy, tx, ty)
int fx, fy, tx, ty;
{
	register int	fcx = xconv(fx),
			tcx = xconv(tx),
			fcy = yconv(fy),
			tcy = yconv(ty);

	if ((fcx < max_x || tcx < max_x) && (fcx >= min_x || tcx >= min_x) &&
	    (fcy < max_y || tcy < max_y) && (fcy >= min_y || tcy >= min_y))
		XDrawLine(DISP, currwin.win, ruleGC,
		    fcx - currwin.base_x, fcy - currwin.base_y,
		    tcx - currwin.base_x, tcy - currwin.base_y);
}

/*
 *	Draw a dot at (x,y)
 */
static	void
dot_at(x, y)
	int	x, y;
{
	register int	cx = xconv(x),
			cy = yconv(y);

	if (cx < max_x && cx >= min_x && cy < max_y && cy >= min_y)
	    XDrawPoint(DISP, currwin.win, ruleGC,
		cx - currwin.base_x, cy - currwin.base_y);
}

/*
 *	Apply the requested attributes to the last path (box) drawn.
 *	Attributes are reset.
 *	(Not currently implemented.)
 */
	/* ARGSUSED */
static	void
do_attribute_path(last_min_x, last_max_x, last_min_y, last_max_y)
int last_min_x, last_max_x, last_min_y, last_max_y;
{
}

/*
 *	Set the size of the virtual pen used to draw in milli-inches
 */

/* ARGSUSED */
static	void
set_pen_size(cp)
	char	*cp;
{
	int	ps;

	if (sscanf(cp, " %d ", &ps) != 1) {
	    Warning("illegal .ps command format: %s", cp);
	    return;
	}
	pen_size = (ps * (xRESOLUTION + yRESOLUTION) + 1000) / 2000;
	if (pen_size < 1) pen_size = 1;
	else if (pen_size > MAX_PEN_SIZE) pen_size = MAX_PEN_SIZE;
}


/*
 *	Print the line defined by previous path commands
 */

static	void
flush_path()
{
	register int i;
	int	last_min_x, last_max_x, last_min_y, last_max_y;

	last_min_x = 30000;
	last_min_y = 30000;
	last_max_x = -30000;
	last_max_y = -30000;
	for (i = 1; i < path_len; i++) {
	    if (xx[i] > last_max_x) last_max_x = xx[i];
	    if (xx[i] < last_min_x) last_min_x = xx[i];
	    if (yy[i] > last_max_y) last_max_y = yy[i];
	    if (yy[i] < last_min_y) last_min_y = yy[i];
	    line_btw(xx[i], yy[i], xx[i+1], yy[i+1]);
	}
	if (xx[path_len] > last_max_x) last_max_x = xx[path_len];
	if (xx[path_len] < last_min_x) last_min_x = xx[path_len];
	if (yy[path_len] > last_max_y) last_max_y = yy[path_len];
	if (yy[path_len] < last_min_y) last_min_y = yy[path_len];
	path_len = 0;
	do_attribute_path(last_min_x, last_max_x, last_min_y, last_max_y);
}


/*
 *	Print a dashed line along the previously defined path, with
 *	the dashes/inch defined.
 */

static	void
flush_dashed(cp, dotted)
	char	*cp;
	Boolean	dotted;
{
	int	i;
	int	numdots;
	int	lx0, ly0, lx1, ly1;
	int	cx0, cy0, cx1, cy1;
	float	inchesperdash;
	double	d, spacesize, a, b, dx, dy, milliperdash;

	if (sscanf(cp, " %f ", &inchesperdash) != 1) {
	    Warning("illegal format for dotted/dashed line: %s", cp);
	    return;
	}
	if (path_len <= 1 || inchesperdash <= 0.0) {
	    Warning("illegal conditions for dotted/dashed line", "");
	    return;
	}
	milliperdash = inchesperdash * 1000.0;
	lx0 = xx[1];	ly0 = yy[1];
	lx1 = xx[2];	ly1 = yy[2];
	dx = lx1 - lx0;
	dy = ly1 - ly0;
	if (dotted) {
	    numdots = sqrt(dx*dx + dy*dy) / milliperdash + 0.5;
	    if (numdots == 0) numdots = 1;
	    for (i = 0; i <= numdots; i++) {
		a = (float) i / (float) numdots;
		cx0 = lx0 + a * dx + 0.5;
		cy0 = ly0 + a * dy + 0.5;
		dot_at(cx0, cy0);
	    }
	}
	else {
	    d = sqrt(dx*dx + dy*dy);
	    numdots = d / (2.0 * milliperdash) + 1.0;
	    if (numdots <= 1)
		line_btw(lx0, ly0, lx1, ly1);
	    else {
		spacesize = (d - numdots * milliperdash) / (numdots - 1);
		for (i = 0; i < numdots - 1; i++) {
		    a = i * (milliperdash + spacesize) / d;
		    b = a + milliperdash / d;
		    cx0 = lx0 + a * dx + 0.5;
		    cy0 = ly0 + a * dy + 0.5;
		    cx1 = lx0 + b * dx + 0.5;
		    cy1 = ly0 + b * dy + 0.5;
		    line_btw(cx0, cy0, cx1, cy1);
		    b += spacesize / d;
		}
		cx0 = lx0 + b * dx + 0.5;
		cy0 = ly0 + b * dy + 0.5;
		line_btw(cx0, cy0, lx1, ly1);
	    }
	}
	path_len = 0;
}


/*
 *	Add a point to the current path
 */

static	void
add_path(cp)
	char	*cp;
{
	int	pathx, pathy;

	if (++path_len >= MAXPOINTS) oops("Too many points");
	if (sscanf(cp, " %d %d ", &pathx, &pathy) != 2)
	    oops("Malformed path command");
	xx[path_len] = pathx;
	yy[path_len] = pathy;
}


/*
 *	Draw to a floating point position
 */

static void
im_fdraw(x, y)
	double	x,y;
{
	if (++path_len >= MAXPOINTS) oops("Too many arc points");
	xx[path_len] = x + 0.5;
	yy[path_len] = y + 0.5;
}


/*
 *	Draw an ellipse with the indicated center and radices.
 */

static	void
draw_ellipse(xc, yc, xr, yr)
	int	xc, yc, xr, yr;
{
	double	angle, theta;
	int	n;
	int	px0, py0, px1, py1;

	angle = (xr + yr) / 2.0;
	theta = sqrt(1.0 / angle);
	n = TWOPI / theta + 0.5;
	if (n < 12) n = 12;
	else if (n > 80) n = 80;
	n /= 2;
	theta = TWOPI / n;

	angle = 0.0;
	px0 = xc + xr;		/* cos(0) = 1 */
	py0 = yc;		/* sin(0) = 0 */
	while ((angle += theta) <= TWOPI) {
	    px1 = xc + xr*cos(angle) + 0.5;
	    py1 = yc + yr*sin(angle) + 0.5;
	    line_btw(px0, py0, px1, py1);
	    px0 = px1;
	    py0 = py1;
	}
	line_btw(px0, py0, xc + xr, yc);
}

/*
 *	Draw an arc
 */

static	void
arc(cp, invis)
	char	*cp;
	Boolean	invis;
{
	int	xc, yc, xrad, yrad, n;
	float	start_angle, end_angle, angle, theta, r;
	double	xradius, yradius, xcenter, ycenter;

	if (sscanf(cp, " %d %d %d %d %f %f ", &xc, &yc, &xrad, &yrad,
		&start_angle, &end_angle) != 6) {
	    Warning("illegal arc specification: %s", cp);
	    return;
	}

	if (invis) return;

	/* We have a specialized fast way to draw closed circles/ellipses */
	if (start_angle <= 0.0 && end_angle >= 6.282) {
	    draw_ellipse(xc, yc, xrad, yrad);
	    return;
	}
	xcenter = xc;
	ycenter = yc;
	xradius = xrad;
	yradius = yrad;
	r = (xradius + yradius) / 2.0;
	theta = sqrt(1.0 / r);
	n = 0.3 * TWOPI / theta + 0.5;
	if (n < 12) n = 12;
	else if (n > 80) n = 80;
	n /= 2;
	theta = TWOPI / n;
	flush_path();
	im_fdraw(xcenter + xradius * cos(start_angle),
	    ycenter + yradius * sin(start_angle));
	angle = start_angle + theta;
	if (end_angle < start_angle) end_angle += TWOPI;
	while (angle < end_angle) {
	    im_fdraw(xcenter + xradius * cos(angle),
		ycenter + yradius * sin(angle));
	    angle += theta;
	}
	im_fdraw(xcenter + xradius * cos(end_angle),
	    ycenter + yradius * sin(end_angle));
	flush_path();
}


/*
 *	APPROXIMATE integer distance between two points
 */

#define	dist(x0, y0, x1, y1)	(abs(x0 - x1) + abs(y0 - y1))


/*
 *	Draw a spline along the previously defined path
 */

static	void
flush_spline()
{
	int	xp, yp;
	int	N;
	int	lastx, lasty;
	Boolean	lastvalid = False;
	int	t1, t2, t3;
	int	steps;
	int	j;
	register int i, w;

#ifdef	lint
	lastx = lasty = -1;
#endif
	N = path_len + 1;
	xx[0] = xx[1];
	yy[0] = yy[1];
	xx[N] = xx[N-1];
	yy[N] = yy[N-1];
	for (i = 0; i < N - 1; i++) {	/* interval */
	    steps = (dist(xx[i], yy[i], xx[i+1], yy[i+1]) +
		dist(xx[i+1], yy[i+1], xx[i+2], yy[i+2])) / 80;
	    for (j = 0; j < steps; j++) {	/* points within */
		w = (j * 1000 + 500) / steps;
		t1 = w * w / 20;
		w -= 500;
		t2 = (750000 - w * w) / 10;
		w -= 500;
		t3 = w * w / 20;
		xp = (t1*xx[i+2] + t2*xx[i+1] + t3*xx[i] + 50000) / 100000;
		yp = (t1*yy[i+2] + t2*yy[i+1] + t3*yy[i] + 50000) / 100000;
		if (lastvalid) line_btw(lastx, lasty, xp, yp);
		lastx = xp;
		lasty = yp;
		lastvalid = True;
	    }
	}
	path_len = 0;
}


/*
 *	Shade the last box, circle, or ellipse
 */

static	void
shade_last()
{
	blacken = whiten = False;
	shade = True;
}


/*
 *	Make the last box, circle, or ellipse, white inside (shade with white)
 */

static	void
whiten_last()
{
	whiten = True;
	blacken = shade = False;
}


/*
 *	Make last box, etc, black inside
 */

static	void
blacken_last()
{
	blacken = True;
	whiten = shade = False;
}


/*
 *	Code for PostScript<tm> specials begins here.
 */

#if	PS

static	void	ps_startup ARGS((int, int, char *));
void	NullProc ARGS((void)) {}
/* ARGSUSED */
static	void	NullProc2 ARGS((char *));

struct psprocs	psp = {		/* used for lazy startup of the ps machinery */
	/* toggle */		NullProc,
	/* destroy */		NullProc,
	/* interrupt */		NullProc,
	/* endpage */		NullProc,
	/* drawbegin */		ps_startup,
	/* drawraw */		NullProc2,
	/* drawfile */		NullProc2,
	/* drawend */		NullProc2};

struct psprocs	no_ps_procs = {		/* used if postscript is unavailable */
	/* toggle */		NullProc,
	/* destroy */		NullProc,
	/* interrupt */		NullProc,
	/* endpage */		NullProc,
	/* drawbegin */		drawbegin_none,
	/* drawraw */		NullProc2,
	/* drawfile */		NullProc2,
	/* drawend */		NullProc2};

#endif	/* PS */

static	Boolean		bbox_valid;
static	unsigned int	bbox_width;
static	unsigned int	bbox_height;
static	int		bbox_voffset;

void
draw_bbox()
{
	if (bbox_valid) {
	    put_border(PXL_H - currwin.base_x,
		PXL_V - currwin.base_y - bbox_voffset,
		bbox_width, bbox_height, ruleGC);
	    bbox_valid = False;
	}
}

#if	PS
static	void
actual_startup()
{
	/*
	 * Figure out what we want to use to display postscript figures
	 * and set at most one of the following to True:
	 * useGS, resource.useDPS, resource.useNeWS
	 *
	 * Choose DPS then NEWS then GhostScript if they are available
	 */
	if (!(
#ifdef	PS_DPS
	    (resource.useDPS && initDPS())
#if	defined(PS_NEWS) || defined(PS_GS)
	    ||
#endif
#endif	/* PS_DPS */

#ifdef	PS_NEWS
	    (resource.useNeWS && initNeWS())
#ifdef	PS_GS
	    ||
#endif
#endif	/* PS_NEWS */

#ifdef	PS_GS
	    (useGS && initGS())
#endif

	    ))
	    psp = no_ps_procs;
}

static	void
ps_startup(xul, yul, cp)
	int	xul, yul;
	char	*cp;
{
	if (!_postscript) {
	    psp.toggle = actual_startup;
	    draw_bbox();
	    return;
	}
	actual_startup();
	psp.drawbegin(xul, yul, cp);
}

/* ARGSUSED */
static	void
NullProc2(cp)
	char	*cp;
{}

/* ARGSUSED */
void
#if	NeedFunctionPrototypes
drawbegin_none(int xul, int yul, char *cp)
#else	/* !NeedFunctionPrototypes */
drawbegin_none(xul, yul, cp)
	int	xul, yul;
	char	*cp;
#endif	/* NeedFunctionPrototypes */
{
	draw_bbox();
}
#endif	/* PS */


/* If FILENAME starts with a left quote, set *DECOMPRESS to 1 and return
   the rest of FILENAME. Otherwise, look up FILENAME along the usual
   path for figure files, set *DECOMPRESS to 0, and return the result
   (NULL if can't find the file).  */

static string
find_fig_file (filename, decompress)
    char *filename;
    int *decompress;
{
  char *name;
  
  if (*filename == '`') {
     name = filename + 1;
    *decompress = 1;
  } else {
    name = kpse_find_pict (filename);
    if (!name)
      fprintf (stderr, "xdvi: %s: Cannot open PS file.\n", filename);
    *decompress = 0;
  }
  
  return name;
}


#if PS
/* If DECOMPRESS is zero, pass NAME to the drawfile proc.  But if
   DECOMPRESS is nonzero, open a pipe to it and pass the resulting
   output to the drawraw proc (in chunks).  */

static void
draw_file (psp, name, decompress)
    struct psprocs psp;
    char *name;
    int decompress;
{
  if (decompress) {
    FILE *pipe;
    if (debug & DBG_PS)
      printf ("%s: piping to PostScript\n", name);
      
    pipe = popen (name, FOPEN_R_MODE);
    if (pipe == NULL)
      perror (name);
    else
      {
        char *line;
        int save_debug = debug;
        debug = 0; /* don't print every line we send */
        while ((line = read_line (pipe)) != NULL) {
          psp.drawraw (line);
          free (line);
        }
        pclose (pipe); /* Linux gives a spurious error, so don't check. */
        debug = save_debug;
      }

  } else { /* a regular file, not decompressing */
    psp.drawfile (name);
  }
}
#endif /* PS */

static	void
psfig_special(cp)
	char	*cp;
{
	char	*filename;
	int	raww, rawh;

	if (strncmp(cp, ":[begin]", 8) == 0) {
	    cp += 8;
	    bbox_valid = False;
	    if (sscanf(cp,"%d %d\n", &raww, &rawh) >= 2) {
		bbox_valid = True;
		bbox_width = pixel_conv(spell_conv(raww));
		bbox_height = pixel_conv(spell_conv(rawh));
		bbox_voffset = 0;
	    }
	    if (currwin.win == mane.win)
#if	PS
		psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y,
		    cp);
#else
		draw_bbox();
#endif
	} else if (strncmp(cp, " plotfile ", 10) == 0) {
	    cp += 10;
	    while (isspace(*cp)) cp++;
	    for (filename = cp; !isspace(*cp); ++cp);
	    *cp = '\0';
#if	PS
	    {
	      int decompress;
	      char *name = find_fig_file (filename, &decompress);
              if (name && currwin.win == mane.win) {
                  draw_file(psp, name, decompress);
                  if (!decompress && name != filename)
                    free (name);
              }
	    }
#endif
	} else if (strncmp(cp, ":[end]", 6) == 0) {
	    cp += 6;
#if	PS
	    if (currwin.win == mane.win) psp.drawend(cp);
#endif
	    bbox_valid = False;
	} else { /* I am going to send some raw postscript stuff */
	  if (*cp == ':')
	    ++cp;	/* skip second colon in ps:: */
#if	PS
	  if (currwin.win == mane.win) {
            /* It's drawbegin that initializes the ps process, so make
               sure it's started up.  */
            psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y, "");
            psp.drawend("");
            psp.drawraw(cp);
	  }
#endif
	}
}


/*	Keys for epsf specials */

static	char	*keytab[]	= {"clip", 
                    "llx",
                    "lly",
                    "urx",
                    "ury",
                    "rwi",
                    "rhi",
                    "hsize",
                    "vsize",
                    "hoffset",
                    "voffset",
                    "hscale",
                    "vscale",
                    "angle"};

#define	KEY_LLX	keyval[0]
#define	KEY_LLY	keyval[1]
#define	KEY_URX	keyval[2]
#define	KEY_URY	keyval[3]
#define	KEY_RWI	keyval[4]
#define	KEY_RHI	keyval[5]

#define	NKEYS	(sizeof(keytab)/sizeof(*keytab))
#define	N_ARGLESS_KEYS 1

static	void
epsf_special(cp)
	char	*cp;
{
	char	*filename, *name;
	int 	decompress;
	static	char		*buffer;
	static	unsigned int	buflen	= 0;
	unsigned int		len;
	char	*p;
	char	*q;
	int	flags	= 0;
	double	keyval[6];

	if (memcmp(cp, "ile=", 4) != 0) {
	    if (!hush_spec_now)
		Fprintf(stderr, "epsf special PSf%s is unknown\n", cp);
	    return;
	}

	p = cp + 4;
	filename = p;
	if (*p == '\'' || *p == '"') {
	    do ++p;
	    while (*p != '\0' && *p != *filename);
	    ++filename;
	}
	else
	    while (*p != '\0' && *p != ' ' && *p != '\t') ++p;
	if (*p != '\0') *p++ = '\0';
	name = find_fig_file (filename, &decompress);
	while (*p == ' ' || *p == '\t') ++p;
	len = strlen(p) + NKEYS + 30;
	if (buflen < len) {
	    if (buflen != 0) free(buffer);
	    buflen = len;
	    buffer = xmalloc(buflen, "epsf buffer");
	}
	Strcpy(buffer, "@beginspecial");
	q = buffer + strlen(buffer);
	while (*p != '\0') {
	    char *p1 = p;
	    int keyno;

	    while (*p1 != '=' && !isspace(*p1) && *p1 != '\0') ++p1;
	    for (keyno = 0;; ++keyno) {
		if (keyno >= NKEYS) {
		    if (!hush_spec_now)
			Fprintf(stderr,
			    "unknown keyword (%*s) in \\special will be ignored\n",
			    (int) (p1 - p), p);
		    break;
		}
		if (memcmp(p, keytab[keyno], p1 - p) == 0) {
		    if (keyno >= N_ARGLESS_KEYS) {
			if (*p1 == '=') ++p1;
			if (keyno < N_ARGLESS_KEYS + 6) {
			    keyval[keyno - N_ARGLESS_KEYS] = atof(p1);
			    flags |= (1 << (keyno - N_ARGLESS_KEYS));
			}
			*q++ = ' ';
			while (!isspace(*p1) && *p1 != '\0') *q++ = *p1++;
		    }
		    *q++ = ' ';
		    *q++ = '@';
		    Strcpy(q, keytab[keyno]);
		    q += strlen(q);
		    break;
		}
	    }
	    p = p1;
	    while (!isspace(*p) && *p != '\0') ++p;
	    while (isspace(*p)) ++p;
	}
	Strcpy(q, " @setspecial\n");

	bbox_valid = False;
	if ((flags & 0x30) == 0x30 || ((flags & 0x30) && (flags & 0xf) == 0xf)){
	    bbox_valid = True;
	    bbox_width = 0.1 * ((flags & 0x10) ? KEY_RWI
		: KEY_RHI * (KEY_URX - KEY_LLX) / (KEY_URY - KEY_LLY))
		* dimconv / shrink_factor + 0.5;
	    bbox_voffset = bbox_height = 0.1 * ((flags & 0x20) ? KEY_RHI
		: KEY_RWI * (KEY_URY - KEY_LLY) / (KEY_URX - KEY_LLX))
		* dimconv / shrink_factor + 0.5;
	}

	if (name && currwin.win == mane.win) {
#if	PS
	    psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y,
		buffer);
	    draw_file(psp, name, decompress);
	    psp.drawend(" @endspecial");
	    if (!decompress && name != filename)
	     free (name);
#else
	    draw_bbox();
#endif
	}
	bbox_valid = False;
}


static	void
bang_special(cp)
	char	*cp;
{
	bbox_valid = False;

#if	PS
	if (currwin.win == mane.win) {
	    psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y,
	        "@defspecial ");
	    /* talk directly with the DPSHandler here */
	    psp.drawraw(cp);
	    psp.drawend(" @fedspecial");
	}
#endif

	/* nothing else to do--there's no bbox here */
}

static	void
quote_special(cp)
	char	*cp;
{
	bbox_valid = False;

#if	PS
	if (currwin.win == mane.win) {
	    psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y,
		"@beginspecial @setspecial ");
	    /* talk directly with the DPSHandler here */
	    psp.drawraw(cp);
	    psp.drawend(" @endspecial");
	}
#endif

	/* nothing else to do--there's no bbox here */
}


/*
 *	The following copyright message applies to the rest of this file.  --PV
 */

/*
 *	This program is Copyright (C) 1987 by the Board of Trustees of the
 *	University of Illinois, and by the author Dirk Grunwald.
 *
 *	This program may be freely copied, as long as this copyright
 *	message remaines affixed. It may not be sold, although it may
 *	be distributed with other software which is sold. If the
 *	software is distributed, the source code must be made available.
 *
 *	No warranty, expressed or implied, is given with this software.
 *	It is presented in the hope that it will prove useful.
 *
 *	Hacked in ignorance and desperation by jonah@db.toronto.edu
 */

/*
 *      The code to handle the \specials generated by tpic was modified
 *      by Dirk Grunwald using the code Tim Morgan at Univ. of Calif, Irvine
 *      wrote for TeXsun.
 */

#define	COMLEN	3

void
applicationDoSpecial(cp)
	char	*cp;
{
	char	command[COMLEN + 1];
	char	*q;
	char	*orig_cp;

	orig_cp = cp;
	while (ISSPACE(*cp)) ++cp;
	q = command;
	while (!ISSPACE(*cp) && *cp && q < command + COMLEN) *q++ = *cp++;
	*q = '\0';
	if (strcmp(command, "pn") == 0) set_pen_size(cp);
	else if (strcmp(command, "fp") == 0) flush_path();
	else if (strcmp(command, "da") == 0) flush_dashed(cp, False);
	else if (strcmp(command, "dt") == 0) flush_dashed(cp, True);
	else if (strcmp(command, "pa") == 0) add_path(cp);
	else if (strcmp(command, "ar") == 0) arc(cp, False);
	else if (strcmp(command, "ia") == 0) arc(cp, True);
	else if (strcmp(command, "sp") == 0) flush_spline();
	else if (strcmp(command, "sh") == 0) shade_last();
	else if (strcmp(command, "wh") == 0) whiten_last();
	else if (strcmp(command, "bk") == 0) blacken_last();
	/* throw away the path -- jansteen */
	else if (strcmp(command, "ip") == 0) path_len = 0;
	else if (strcmp(command, "ps:") == 0) psfig_special(cp);
	else if (strcmp(command, "PSf") == 0) epsf_special(cp);
	else if (strcmp(command, "psf") == 0) epsf_special(cp);
	else if (*orig_cp == '"') quote_special(orig_cp + 1);
	else if (*orig_cp == '!') bang_special(orig_cp + 1);
	else if (!hush_spec_now)
	    Fprintf(stderr, "%s:  special \"%s\" not implemented\n", prog,
		orig_cp);
}
#ifdef KDVI
void psp_destroy()	{	psp.destroy();		}
void psp_toggle()	{	psp.toggle();		}
void psp_interrupt()	{	psp.interrupt();	}
#endif /* KDVI */
