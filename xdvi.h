#ifndef _xdvi_h
#define _xdvi_h

/*
 *	Written by Eric C. Cooper, CMU
 */

extern	struct drawinf	currinf;

/* entries below with the characters 'dvi' in them are actually stored in
   scaled pixel units */

#define DVI_H   currinf.data.dvi_h
#define PXL_H   ((int) ((currinf.data.dvi_h) / (currwin.shrinkfactor * 65536)))
#define DVI_V   currinf.data.dvi_v
#define PXL_V   currinf.data.pxl_v
#define WW      currinf.data.w
#define XX      currinf.data.x
#define YY      currinf.data.y
#define ZZ      currinf.data.z
#define ROUNDUP(x,y) (((x)+(y)-1)/(y))

extern  unsigned long   num (FILE *, int);
extern  long    snum(FILE *, int);
extern	struct WindowRec mane, currwin;

#define one(fp)		((unsigned char) getc(fp))
#define sone(fp)	((long) one(fp))
#define two(fp)		num (fp, 2)
#define stwo(fp)	snum(fp, 2)
#define four(fp)	num (fp, 4)
#define sfour(fp)	snum(fp, 4)

#endif /* _xdvi_h */
