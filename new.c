#define	EXTERN
#define	INIT(x)	=x
#include "oconfig.h"

struct WindowRec mane	= {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};
struct WindowRec currwin = {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};


void
init_pix(warn)
	Boolean	warn;
{
	static	int	shrink_allocated_for = 0;
	static Boolean colors_allocated = False;
	static int _copy = 0;
	int	i;

	if (!colors_allocated)
	{
	    Pixel plane_masks[4];
	    Pixel pixel;
	    XColor color, fc, bc;
	    XGCValues	values;

	    if ( _gamma == 0.0) _gamma = 1.0;

	    if (!_copy)
		/* allocate 4 color planes for 16 colors (for GXor drawing) */
		if (!XAllocColorCells(DISP, DefaultColormapOfScreen(SCRN),
					  False, plane_masks, 4, &pixel, 1))
		    _copy = warn = True;

	    /* get foreground and background RGB values for interpolating */
	    fc.pixel = _fore_Pixel;
	    XQueryColor(DISP, DefaultColormapOfScreen(SCRN), &fc);
	    bc.pixel = _back_Pixel;
	    XQueryColor(DISP, DefaultColormapOfScreen(SCRN), &bc);

	    for (i = 0; i < 16; ++i) {
		double	pow();
		double	frac = _gamma > 0 ? pow((double) i / 15, 1 / _gamma)
		    : 1 - pow((double) (15 - i) / 15, -_gamma);

		color.red = frac * ((double) fc.red - bc.red) + bc.red;
		color.green = frac * ((double) fc.green - bc.green) + bc.green;
		color.blue = frac * ((double) fc.blue - bc.blue) + bc.blue;

		color.pixel = pixel;
		color.flags = DoRed | DoGreen | DoBlue;

		if (!_copy) {
		    if (i & 1) color.pixel |= plane_masks[0];
		    if (i & 2) color.pixel |= plane_masks[1];
		    if (i & 4) color.pixel |= plane_masks[2];
		    if (i & 8) color.pixel |= plane_masks[3];
		    XStoreColor(DISP, DefaultColormapOfScreen(SCRN), &color);
		    palette[i] = color.pixel;
		}
		else {
		    if (!XAllocColor(DISP, DefaultColormapOfScreen(SCRN),
			&color))
			palette[i] = (i * 100 >= density * 15)
			    ? _fore_Pixel : _back_Pixel;
		    else
			palette[i] = color.pixel;
		}
	    }

	    /* Make sure fore_ and back_Pixel are a part of the palette */
	    _fore_Pixel = palette[15];
	    _back_Pixel = palette[0];
	    if (mane.win != (Window) 0)
		XSetWindowBackground(DISP, mane.win, palette[0]);

#define	MakeGC(fcn, fg, bg)	(values.function = fcn, values.foreground=fg,\
		values.background=bg,\
		XCreateGC(DISP, RootWindowOfScreen(SCRN),\
			GCFunction|GCForeground|GCBackground, &values))

	    foreGC = ruleGC = MakeGC(_copy ? GXcopy : GXor,
		_fore_Pixel, _back_Pixel);
	    foreGC2 = NULL;

	    colors_allocated = True;
#define CopyByDefault() (_copy == 2)
	    /* warn only if copy was not explicitly requested */
	    if (CopyByDefault() && warn)
		Puts("Note:  overstrike characters may be incorrect.");
	}
#undef	MakeGC

	if (mane.shrinkfactor == 1) return;

	if (shrink_allocated_for < mane.shrinkfactor) {
	    if (pixeltbl != NULL) free((char *) pixeltbl);
	    pixeltbl = (Pixel *) xmalloc((unsigned)
		(mane.shrinkfactor * mane.shrinkfactor + 1) * sizeof(Pixel),
		"pixel table");
	    shrink_allocated_for = mane.shrinkfactor;
	}

	for (i = 0; i <= mane.shrinkfactor * mane.shrinkfactor; ++i)
	    pixeltbl[i] =
		palette[(i * 30 + mane.shrinkfactor * mane.shrinkfactor)
		    / (2 * mane.shrinkfactor * mane.shrinkfactor)];
}
