//========================================================================
//
// TextOutputDev.cc
//
// Copyright 1997-2002 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>
#include "GString.h"
#include "gmem.h"
#include "config.h"
#include "Error.h"
#include "GlobalParams.h"
#include "UnicodeMap.h"
#include "GfxState.h"
#include "TextOutputDev.h"

#ifdef MACOS
// needed for setting type/creator of MacOS files
#include "ICSupport.h"
#endif

//------------------------------------------------------------------------

#define textOutSpace    0.2
#define textOutColSpace 0.2

//------------------------------------------------------------------------

struct TextOutColumnEdge {
  double x, y0, y1;
};

//------------------------------------------------------------------------
// TextBlock
//------------------------------------------------------------------------

class TextBlock {
public:

  TextBlock();
  ~TextBlock();

  double xMin, xMax;
  double yMin, yMax;
  TextString *strings;		// list of strings in the block
  TextBlock *next;		// next block in line
  TextBlock *xyNext;		// next block on xyBlocks list
  Unicode *text;		// Unicode text of the block, including
				//   spaces between strings
  double *xRight;		// right-hand x coord of each char
  int len;			// total number of Unicode characters
  int convertedLen;		// total number of converted characters
  int *col;			// starting column number for each
				//   Unicode character
};

TextBlock::TextBlock() {
  strings = NULL;
  next = NULL;
  xyNext = NULL;
  text = NULL;
  xRight = NULL;
  col = NULL;
}

TextBlock::~TextBlock() {
  TextString *p1, *p2;

  for (p1 = strings; p1; p1 = p2) {
    p2 = p1->next;
    delete p1;
  }
  gfree(text);
  gfree(xRight);
  gfree(col);
}

//------------------------------------------------------------------------
// TextLine
//------------------------------------------------------------------------

class TextLine {
public:

  TextLine();
  ~TextLine();

  TextBlock *blocks;
  TextLine *next;
  double yMin, yMax;
};

TextLine::TextLine() {
  blocks = NULL;
  next = NULL;
}

TextLine::~TextLine() {
  TextBlock *p1, *p2;

  for (p1 = blocks; p1; p1 = p2) {
    p2 = p1->next;
    delete p1;
  }
}

//------------------------------------------------------------------------
// TextString
//------------------------------------------------------------------------

TextString::TextString(GfxState *state, double x0, double y0,
		       double fontSize) {
  GfxFont *font;
  double x, y;

  state->transform(x0, y0, &x, &y);
  if ((font = state->getFont())) {
    yMin = y - font->getAscent() * fontSize;
    yMax = y - font->getDescent() * fontSize;
  } else {
    // this means that the PDF file draws text without a current font,
    // which should never happen
    yMin = y - 0.95 * fontSize;
    yMax = y + 0.35 * fontSize;
  }
  if (yMin == yMax) {
    // this is a sanity check for a case that shouldn't happen -- but
    // if it does happen, we want to avoid dividing by zero later
    yMin = y;
    yMax = y + 1;
  }
  marked = gFalse;
  text = NULL;
  xRight = NULL;
  len = size = 0;
  next = NULL;
}


TextString::~TextString() {
  gfree(text);
  gfree(xRight);
}

void TextString::addChar(GfxState *state, double x, double y,
			 double dx, double dy, Unicode u) {
  if (len == size) {
    size += 16;
    text = (Unicode *)grealloc(text, size * sizeof(Unicode));
    xRight = (double *)grealloc(xRight, size * sizeof(double));
  }
  text[len] = u;
  if (len == 0) {
    xMin = x;
  }
  xMax = xRight[len] = x + dx;
  ++len;
}

//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------

TextPage::TextPage(GBool rawOrderA) {
  rawOrder = rawOrderA;
  curStr = NULL;
  fontSize = 0;
  xyStrings = NULL;
  xyCur1 = xyCur2 = NULL;
  lines = NULL;
  nest = 0;
  nTinyChars = 0;
}

TextPage::~TextPage() {
  clear();
}

void TextPage::updateFont(GfxState *state) {
  GfxFont *font;
  double *fm;
  char *name;
  int code, mCode, letterCode, anyCode;
  double w;

  // adjust the font size
  fontSize = state->getTransformedFontSize();
  if ((font = state->getFont()) && font->getType() == fontType3) {
    // This is a hack which makes it possible to deal with some Type 3
    // fonts.  The problem is that it's impossible to know what the
    // base coordinate system used in the font is without actually
    // rendering the font.  This code tries to guess by looking at the
    // width of the character 'm' (which breaks if the font is a
    // subset that doesn't contain 'm').
    mCode = letterCode = anyCode = -1;
    for (code = 0; code < 256; ++code) {
      name = ((Gfx8BitFont *)font)->getCharName(code);
      if (name && name[0] == 'm' && name[1] == '\0') {
	mCode = code;
      }
      if (letterCode < 0 && name && name[1] == '\0' &&
	  ((name[0] >= 'A' && name[0] <= 'Z') ||
	   (name[0] >= 'a' && name[0] <= 'z'))) {
	letterCode = code;
      }
      if (anyCode < 0 && name && ((Gfx8BitFont *)font)->getWidth(code) > 0) {
	anyCode = code;
      }
    }
    if (mCode >= 0 &&
	(w = ((Gfx8BitFont *)font)->getWidth(mCode)) > 0) {
      // 0.6 is a generic average 'm' width -- yes, this is a hack
	fontSize *= w / 0.6;
    } else if (letterCode >= 0 &&
	       (w = ((Gfx8BitFont *)font)->getWidth(letterCode)) > 0) {
      // even more of a hack: 0.5 is a generic letter width
      fontSize *= w / 0.5;
    } else if (anyCode >= 0 &&
	       (w = ((Gfx8BitFont *)font)->getWidth(anyCode)) > 0) {
      // better than nothing: 0.5 is a generic character width
      fontSize *= w / 0.5;
    }
    fm = font->getFontMatrix();
    if (fm[0] != 0) {
      fontSize *= fabs(fm[3] / fm[0]);
    }
  }
}

void TextPage::beginString(GfxState *state, double x0, double y0) {
  // This check is needed because Type 3 characters can contain
  // text-drawing operations.
  if (curStr) {
    ++nest;
    return;
  }

  curStr = new TextString(state, x0, y0, fontSize);
}

void TextPage::addChar(GfxState *state, double x, double y,
		       double dx, double dy, Unicode *u, int uLen) {
    if (! curStr) return;

  double x1, y1, w1, h1, dx2, dy2;
  int n, i;

  state->transform(x, y, &x1, &y1);
  if (x1 < 0 || x1 > state->getPageWidth() ||
      y1 < 0 || y1 > state->getPageHeight()) {
    return;
  }
  state->textTransformDelta(state->getCharSpace() * state->getHorizScaling(),
			    0, &dx2, &dy2);
  dx -= dx2;
  dy -= dy2;
  state->transformDelta(dx, dy, &w1, &h1);
  if (!globalParams->getTextKeepTinyChars() &&
      fabs(w1) < 3 && fabs(h1) < 3) {
    if (++nTinyChars > 20000) {
      return;
    }
  }
  n = curStr->len;
  if (n > 0 && x1 - curStr->xRight[n-1] >
               0.1 * (curStr->yMax - curStr->yMin)) {
    // large char spacing is sometimes used to move text around
    endString();
    beginString(state, x, y);
  }
  if (uLen == 1 && u[0] == (Unicode)0x20 &&
      w1 > 0.5 * (curStr->yMax - curStr->yMin)) {
    // large word spacing is sometimes used to move text around
    return;
  }
  if (uLen != 0) {
    w1 /= uLen;
    h1 /= uLen;
  }
  for (i = 0; i < uLen; ++i) {
    curStr->addChar(state, x1 + i*w1, y1 + i*h1, w1, h1, u[i]);
  }
}

void TextPage::endString() {
  // This check is needed because Type 3 characters can contain
  // text-drawing operations.
  if (nest > 0) {
    --nest;
    return;
  }

  addString(curStr);
  curStr = NULL;
}

void TextPage::addString(TextString *str) {
    if (! str) return;

  TextString *p1, *p2;

  // throw away zero-length strings -- they don't have valid xMin/xMax
  // values, and they're useless anyway
  if (str->len == 0) {
    delete str;
    return;
  }

  // insert string in xy list
  if (rawOrder) {
    p1 = xyCur1;
    p2 = NULL;
  } else if ((!xyCur1 || xyBefore(xyCur1, str)) &&
	     (!xyCur2 || xyBefore(str, xyCur2))) {
    p1 = xyCur1;
    p2 = xyCur2;
  } else if (xyCur1 && xyBefore(xyCur1, str)) {
    for (p1 = xyCur1, p2 = xyCur2; p2; p1 = p2, p2 = p2->next) {
      if (xyBefore(str, p2)) {
	break;
      }
    }
    xyCur2 = p2;
  } else {
    for (p1 = NULL, p2 = xyStrings; p2; p1 = p2, p2 = p2->next) {
      if (xyBefore(str, p2)) {
	break;
      }
    }
    xyCur2 = p2;
  }
  xyCur1 = str;
  if (p1) {
    p1->next = str;
  } else {
    xyStrings = str;
  }
  str->next = p2;
}

void TextPage::coalesce() {
  TextLine *line, *line0;
  TextBlock *yxBlocks, *xyBlocks, *blk, *blk0, *blk1, *blk2;
  TextString *str0, *str1, *str2, *str3, *str4;
  TextString *str1prev, *str2prev, *str3prev;
  TextOutColumnEdge *edges;
  UnicodeMap *uMap;
  GBool isUnicode;
  char buf[8];
  int edgesLength, edgesSize;
  double x, yMin, yMax;
  double space, fit1, fit2, h;
  int col1, col2, d;
  int i, j;

#if 0 //~ for debugging
  for (str1 = xyStrings; str1; str1 = str1->next) {
    printf("x=%.2f..%.2f  y=%.2f..%.2f  size=%.2f '",
	   str1->xMin, str1->xMax, str1->yMin, str1->yMax,
	   (str1->yMax - str1->yMin));
    for (i = 0; i < str1->len; ++i) {
      fputc(str1->text[i] & 0xff, stdout);
    }
    printf("'\n");
  }
  printf("\n------------------------------------------------------------\n\n");
#endif

  // build the list of column edges
  edges = NULL;
  edgesLength = edgesSize = 0;
  if (!rawOrder) {
    for (str1prev = NULL, str1 = xyStrings;
	 str1;
	 str1prev = str1, str1 = str1->next) {
      if (str1->marked) {
	continue;
      }
      h = str1->yMax - str1->yMin;
      if (str1prev && (str1->xMin - str1prev->xMax) / h < textOutColSpace) {
	continue;
      }
      x = str1->xMin;
      yMin = str1->yMin;
      yMax = str1->yMax;
      for (str2prev = str1, str2 = str1->next;
	   str2;
	   str2prev = str2, str2 = str2->next) {
	h = str2->yMax - str2->yMin;
	if (!str2->marked &&
	    (str2->xMin - str2prev->xMax) / h > textOutColSpace &&
	    fabs(str2->xMin - x) < 0.5 &&
	    str2->yMin - yMax < 0.3 * h &&
	    yMin - str2->yMax < 0.3 * h) {
	  break;
	}
      }
      if (str2) {
	if (str2->yMin < yMin) {
	  yMin = str2->yMin;
	}
	if (str2->yMax > yMax) {
	  yMax = str2->yMax;
	}
	str2->marked = gTrue;
	for (str3prev = str1, str3 = str1->next;
	     str3;
	     str3prev = str3, str3 = str3->next) {
	  h = str3->yMax - str3->yMin;
	  if (!str3->marked &&
	      (str3->xMin - str3prev->xMax) / h > textOutColSpace &&
	      fabs(str3->xMin - x) < 0.5 &&
	      str3->yMin - yMax < 0.3 * h &&
	      yMin - str3->yMax < 0.3 * h) {
	    break;
	  }
	}
	if (str3) {
	  if (str3->yMin < yMin) {
	    yMin = str3->yMin;
	  }
	  if (str3->yMax > yMax) {
	    yMax = str3->yMax;
	  }
	  str3->marked = gTrue;
	  do {
	    for (str2prev = str1, str2 = str1->next;
		 str2;
		 str2prev = str2, str2 = str2->next) {
	      h = str2->yMax - str2->yMin;
	      if (!str2->marked &&
		  (str2->xMin - str2prev->xMax) / h > textOutColSpace &&
		  fabs(str2->xMin - x) < 0.5 &&
		  str2->yMin - yMax < 0.3 * h &&
		  yMin - str2->yMax < 0.3 * h) {
		if (str2->yMin < yMin) {
		  yMin = str2->yMin;
		}
		if (str2->yMax > yMax) {
		  yMax = str2->yMax;
		}
		str2->marked = gTrue;
		break;
	      }
	    }
	  } while (str2);
	  if (edgesLength == edgesSize) {
	    edgesSize = edgesSize ? 2 * edgesSize : 16;
	    edges = (TextOutColumnEdge *)
	      grealloc(edges, edgesSize * sizeof(TextOutColumnEdge));
	  }
	  edges[edgesLength].x = x;
	  edges[edgesLength].y0 = yMin;
	  edges[edgesLength].y1 = yMax;
	  ++edgesLength;
	} else {
	  str2->marked = gFalse;
	}
      }
      str1->marked = gTrue;
    }
  }

#if 0 //~ for debugging
  printf("column edges:\n");
  for (i = 0; i < edgesLength; ++i) {
    printf("%d: x=%.2f y0=%.2f y1=%.2f\n",
	   i, edges[i].x, edges[i].y0, edges[i].y1);
  }
  printf("\n------------------------------------------------------------\n\n");
#endif

  // build the blocks
  yxBlocks = NULL;
  blk1 = blk2 = NULL;
  while (xyStrings) {

    // build the block
    str0 = xyStrings;
    xyStrings = xyStrings->next;
    str0->next = NULL;
    blk = new TextBlock();
    blk->strings = str0;
    blk->xMin = str0->xMin;
    blk->xMax = str0->xMax;
    blk->yMin = str0->yMin;
    blk->yMax = str0->yMax;
    while (xyStrings) {
      str1 = NULL;
      str2 = xyStrings;
      fit1 = coalesceFit(str0, str2);
      if (!rawOrder) {
	// look for best-fitting string
	space = str0->yMax - str0->yMin;
	for (str3 = xyStrings, str4 = xyStrings->next;
	     str4 && str4->xMin - str0->xMax <= space;
	     str3 = str4, str4 = str4->next) {
	  fit2 = coalesceFit(str0, str4);
	  if (fit2 < fit1) {
	    str1 = str3;
	    str2 = str4;
	    fit1 = fit2;
	  }
	}
      }
      if (fit1 > 1) {
	// no fit - we're done with this block
	break;
      }

      // if we've hit a column edge we're done with this block
      if (fit1 > 0.2) {
	for (i = 0; i < edgesLength; ++i) {
	  if (str0->xMax < edges[i].x + 0.5 && edges[i].x - 0.5 < str2->xMin &&
	      str0->yMin < edges[i].y1 && str0->yMax > edges[i].y0 &&
	      str2->yMin < edges[i].y1 && str2->yMax > edges[i].y0) {
	    break;
	  }
	}
	if (i < edgesLength) {
	  break;
	}
      }

      if (str1) {
	str1->next = str2->next;
      } else {
	xyStrings = str2->next;
      }
      str0->next = str2;
      str2->next = NULL;
      if (str2->xMax > blk->xMax) {
	blk->xMax = str2->xMax;
      }
      if (str2->yMin < blk->yMin) {
	blk->yMin = str2->yMin;
      }
      if (str2->yMax > blk->yMax) {
	blk->yMax = str2->yMax;
      }
      str0 = str2;
    }

    // insert block on list
    if (!rawOrder) {
      // insert block on list in yx order
      for (blk1 = NULL, blk2 = yxBlocks;
	   blk2 && !yxBefore(blk, blk2);
	   blk1 = blk2, blk2 = blk2->next) ;
    }
    blk->next = blk2;
    if (blk1) {
      blk1->next = blk;
    } else {
      yxBlocks = blk;
    }
    blk1 = blk;
  }

  gfree(edges);

  // the strings are now owned by the lines/blocks tree
  xyStrings = NULL;

  // build the block text
  uMap = globalParams->getTextEncoding();
  isUnicode = uMap ? uMap->isUnicode() : gFalse;
  for (blk = yxBlocks; blk; blk = blk->next) {
    blk->len = 0;
    for (str1 = blk->strings; str1; str1 = str1->next) {
      blk->len += str1->len;
      if (str1->next && str1->next->xMin - str1->xMax >
	                textOutSpace * (str1->yMax - str1->yMin)) {
	str1->spaceAfter = gTrue;
	++blk->len;
      } else {
	str1->spaceAfter = gFalse;
      }
    }
    blk->text = (Unicode *)gmalloc(blk->len * sizeof(Unicode));
    blk->xRight = (double *)gmalloc(blk->len * sizeof(double));
    blk->col = (int *)gmalloc(blk->len * sizeof(int));
    i = 0;
    for (str1 = blk->strings; str1; str1 = str1->next) {
      for (j = 0; j < str1->len; ++j) {
	blk->text[i] = str1->text[j];
	blk->xRight[i] = str1->xRight[j];
	++i;
      }
      if (str1->spaceAfter) {
	blk->text[i] = (Unicode)0x0020;
	blk->xRight[i] = str1->next->xMin;
	++i;
      }
    }
    blk->convertedLen = 0;
    for (j = 0; j < blk->len; ++j) {
      blk->col[j] = blk->convertedLen;
      if (isUnicode) {
	++blk->convertedLen;
      } else if (uMap) {
	blk->convertedLen += uMap->mapUnicode(blk->text[j], buf, sizeof(buf));
      }
    }
  }
  if (uMap) {
    uMap->decRefCnt();
  }

#if 0 //~ for debugging
  for (blk = yxBlocks; blk; blk = blk->next) {
    printf("[block: x=%.2f..%.2f y=%.2f..%.2f len=%d]\n",
	   blk->xMin, blk->xMax, blk->yMin, blk->yMax, blk->len);
    TextString *str;
    for (str = blk->strings; str; str = str->next) {
      printf("    x=%.2f..%.2f  y=%.2f..%.2f  size=%.2f'",
	     str->xMin, str->xMax, str->yMin, str->yMax,
	     (str->yMax - str->yMin));
      for (i = 0; i < str->len; ++i) {
	fputc(str->text[i] & 0xff, stdout);
      }
      if (str->spaceAfter) {
	fputc(' ', stdout);
      }
      printf("'\n");
    }
  }
  printf("\n------------------------------------------------------------\n\n");
#endif

  // build the lines
  lines = NULL;
  line0 = NULL;
  while (yxBlocks) {
    blk0 = yxBlocks;
    yxBlocks = yxBlocks->next;
    blk0->next = NULL;
    line = new TextLine();
    line->blocks = blk0;
    line->yMin = blk0->yMin;
    line->yMax = blk0->yMax;
    while (yxBlocks) {

      // remove duplicated text (fake boldface, shadowed text)
      h = blk0->yMax - blk0->yMin;
      if (yxBlocks->len == blk0->len &&
	  !memcmp(yxBlocks->text, blk0->text,
		  yxBlocks->len * sizeof(Unicode)) &&
	  fabs(yxBlocks->yMin - blk0->yMin) / h < 0.2 &&
	  fabs(yxBlocks->yMax - blk0->yMax) / h < 0.2 &&
	  fabs(yxBlocks->xMin - blk0->xMin) / h < 0.2 &&
	  fabs(yxBlocks->xMax - blk0->xMax) / h < 0.2) {
	blk1 = yxBlocks;
	yxBlocks = yxBlocks->next;
	delete blk1;
	continue;
      }

      if (rawOrder && yxBlocks->yMax < blk0->yMin) {
	break;
      }
      if (yxBlocks->yMin > 0.2*blk0->yMin + 0.8*blk0->yMax ||
	  yxBlocks->xMin < blk0->xMax) {
	break;
      }
      blk1 = yxBlocks;
      yxBlocks = yxBlocks->next;
      blk0->next = blk1;
      blk1->next = NULL;
      if (blk1->yMin < line->yMin) {
	line->yMin = blk1->yMin;
      }
      if (blk1->yMax > line->yMax) {
	line->yMax = blk1->yMax;
      }
      blk0 = blk1;
      }
    if (line0) {
      line0->next = line;
    } else {
      lines = line;
    }
    line->next = NULL;
    line0 = line;
      }


  // sort the blocks into xy order
  xyBlocks = NULL;
  for (line = lines; line; line = line->next) {
    for (blk = line->blocks; blk; blk = blk->next) {
      for (blk1 = NULL, blk2 = xyBlocks;
	   blk2 && !xyBefore(blk, blk2);
	   blk1 = blk2, blk2 = blk2->xyNext) ;
      blk->xyNext = blk2;
      if (blk1) {
	blk1->xyNext = blk;
    } else {
	xyBlocks = blk;
      }
    }
  }

#if 0 //~ for debugging
  for (blk = xyBlocks; blk; blk = blk->xyNext) {
    printf("[block: x=%.2f..%.2f y=%.2f..%.2f len=%d]\n",
	   blk->xMin, blk->xMax, blk->yMin, blk->yMax, blk->len);
    TextString *str;
    for (str = blk->strings; str; str = str->next) {
      printf("    x=%.2f..%.2f  y=%.2f..%.2f  size=%.2f '",
	     str->xMin, str->xMax, str->yMin, str->yMax,
	     (str->yMax - str->yMin));
      for (i = 0; i < str->len; ++i) {
	fputc(str->text[i] & 0xff, stdout);
      }
      printf("'\n");
    }
  }
  printf("\n------------------------------------------------------------\n\n");
#endif

  // do column assignment
  for (blk1 = xyBlocks; blk1; blk1 = blk1->xyNext) {
    col1 = 0;
    for (blk2 = xyBlocks; blk2 != blk1; blk2 = blk2->xyNext) {
      if (blk1->xMin >= blk2->xMax) {
	d = (int)((blk1->xMin - blk2->xMax) /
		  (0.4 * (blk1->yMax - blk1->yMin)));
	if (d > 4) {
	  d = 4;
	}
	col2 = blk2->col[0] + blk2->convertedLen + d;
	if (col2 > col1) {
	  col1 = col2;
	}
      } else if (blk1->xMin > blk2->xMin) {
	for (i = 0; i < blk2->len && blk1->xMin >= blk2->xRight[i]; ++i) ;
	col2 = blk2->col[i];
	if (col2 > col1) {
	  col1 = col2;
	}
      }
    }
    for (j = 0; j < blk1->len; ++j) {
      blk1->col[j] += col1;
    }
  }

#if 0 //~ for debugging
  for (line = lines; line; line = line->next) {
    printf("[line]\n");
    for (blk = line->blocks; blk; blk = blk->next) {
      printf("[block: col=%d, len=%d]\n", blk->col[0], blk->len);
      TextString *str;
      for (str = blk->strings; str; str = str->next) {
	printf("    x=%.2f..%.2f  y=%.2f..%.2f  size=%.2f '",
	       str->xMin, str->xMax, str->yMin, str->yMax,
	       (str->yMax - str->yMin));
	for (i = 0; i < str->len; ++i) {
	  fputc(str->text[i] & 0xff, stdout);
	}
	if (str->spaceAfter) {
	  printf(" [space]\n");
	}
	printf("'\n");
      }
    }
  }
  printf("\n------------------------------------------------------------\n\n");
#endif
}


GBool TextPage::findText(Unicode *s, int len,
			 GBool top, GBool bottom,
			 double *xMin, double *yMin,
			 double *xMax, double *yMax) {
  TextLine *line;
  TextBlock *blk;
  Unicode *p;
  Unicode u1, u2;
  int m, i, j;
  double x0, x1, x;

  // scan all blocks on page
  for (line = lines; line; line = line->next) {
    for (blk = line->blocks; blk; blk = blk->next) {

    // check: above top limit?
      if (!top && (blk->yMax < *yMin ||
		   (blk->yMin < *yMin && blk->xMax <= *xMin))) {
      continue;
    }

    // check: below bottom limit?
      if (!bottom && (blk->yMin > *yMax ||
		      (blk->yMax > *yMax && blk->xMin >= *xMax))) {
      return gFalse;
    }

      // search each position in this block
      m = blk->len;
      for (i = 0, p = blk->text; i <= m - len; ++i, ++p) {

	x0 = (i == 0) ? blk->xMin : blk->xRight[i-1];
	x1 = blk->xRight[i];
	x = 0.5 * (x0 + x1);

      // check: above top limit?
	if (!top && blk->yMin < *yMin) {
	if (x < *xMin) {
	  continue;
	}
      }

      // check: below bottom limit?
	if (!bottom && blk->yMax > *yMax) {
	if (x > *xMax) {
	  return gFalse;
	}
      }

      // compare the strings
      for (j = 0; j < len; ++j) {
#if 1 //~ this lowercases Latin A-Z only -- this will eventually be
      //~ extended to handle other character sets
	if (p[j] >= 0x41 && p[j] <= 0x5a) {
	  u1 = p[j] + 0x20;
	} else {
	  u1 = p[j];
	}
	if (s[j] >= 0x41 && s[j] <= 0x5a) {
	  u2 = s[j] + 0x20;
	} else {
	  u2 = s[j];
	}
#endif
	if (u1 != u2) {
	  break;
	}
      }

      // found it
      if (j == len) {
	  *xMin = x0;
	  *xMax = blk->xRight[i + len - 1];
	  *yMin = blk->yMin;
	  *yMax = blk->yMax;
	return gTrue;
      }
    }
  }
  }

  return gFalse;
}

GString *TextPage::getText(double xMin, double yMin,
			   double xMax, double yMax) {
  GString *s;
  UnicodeMap *uMap;
  GBool isUnicode;
  char space[8], eol[16], buf[8];
  int spaceLen, eolLen, len;
  TextLine *line;
  TextBlock *blk;
  double x0, x1, y;
  int firstCol, col, i;
  GBool multiLine;

  s = new GString();

  // get the output encoding
  if (!(uMap = globalParams->getTextEncoding())) {
    return s;
  }
  isUnicode = uMap->isUnicode();
  spaceLen = uMap->mapUnicode(0x20, space, sizeof(space));
  eolLen = 0; // make gcc happy
  switch (globalParams->getTextEOL()) {
  case eolUnix:
    eolLen = uMap->mapUnicode(0x0a, eol, sizeof(eol));
    break;
  case eolDOS:
    eolLen = uMap->mapUnicode(0x0d, eol, sizeof(eol));
    eolLen += uMap->mapUnicode(0x0a, eol + eolLen, sizeof(eol) - eolLen);
    break;
  case eolMac:
    eolLen = uMap->mapUnicode(0x0d, eol, sizeof(eol));
    break;
  }

  // find the leftmost column
  multiLine = gFalse;
  firstCol = -1;
  for (line = lines; line; line = line->next) {
    if (line->yMin > yMax) {
      break;
    }
    if (line->yMax < yMin) {
      continue;
    }

    for (blk = line->blocks; blk && blk->xMax < xMin; blk = blk->next) ;
    if (!blk || blk->xMin > xMax) {
      continue;
    }

    y = 0.5 * (blk->yMin + blk->yMax);
    if (y < yMin || y > yMax) {
      continue;
    }

    if (firstCol >= 0) {
      multiLine = gTrue;
    }

    i = 0;
    while (1) {
      x0 = (i==0) ? blk->xMin : blk->xRight[i-1];
      x1 = blk->xRight[i];
      if (0.5 * (x0 + x1) > xMin) {
	  break;
	}
      ++i;
    }
    col = blk->col[i];

    if (firstCol < 0 || col < firstCol) {
      firstCol = col;
    }
      }

  // extract the text
  for (line = lines; line; line = line->next) {
    if (line->yMin > yMax) {
	  break;
	}
    if (line->yMax < yMin) {
      continue;
      }

    for (blk = line->blocks; blk && blk->xMax < xMin; blk = blk->next) ;
    if (!blk || blk->xMin > xMax) {
      continue;
    }

    y = 0.5 * (blk->yMin + blk->yMax);
    if (y < yMin || y > yMax) {
      continue;
    }

    i = 0;
    while (1) {
      x0 = (i==0) ? blk->xMin : blk->xRight[i-1];
      x1 = blk->xRight[i];
      if (0.5 * (x0 + x1) > xMin) {
	break;
	  }
      ++i;
	}

    col = firstCol;

    do {

      // line this block up with the correct column
      for (; col < blk->col[i]; ++col) {
	s->append(space, spaceLen);
      }

      // print the block
      for (; i < blk->len; ++i) {

	x0 = (i==0) ? blk->xMin : blk->xRight[i-1];
	x1 = blk->xRight[i];
	if (0.5 * (x0 + x1) > xMax) {
	  break;
      }

	len = uMap->mapUnicode(blk->text[i], buf, sizeof(buf));
	s->append(buf, len);
	col += isUnicode ? 1 : len;
    }
      if (i < blk->len) {
	break;
  }

      // next block
      blk = blk->next;
      i = 0;

    } while (blk && blk->xMin < xMax);

  if (multiLine) {
    s->append(eol, eolLen);
  }
  }

  uMap->decRefCnt();

  return s;
}

void TextPage::dump(void *outputStream, TextOutputFunc outputFunc) {
  UnicodeMap *uMap;
  char space[8], eol[16], eop[8], buf[8];
  int spaceLen, eolLen, eopLen, len;
  TextLine *line;
  TextBlock *blk;
  int col, d, i;

  // get the output encoding
  if (!(uMap = globalParams->getTextEncoding())) {
    return;
  }
  spaceLen = uMap->mapUnicode(0x20, space, sizeof(space));
  eolLen = 0; // make gcc happy
  switch (globalParams->getTextEOL()) {
  case eolUnix:
    eolLen = uMap->mapUnicode(0x0a, eol, sizeof(eol));
    break;
  case eolDOS:
    eolLen = uMap->mapUnicode(0x0d, eol, sizeof(eol));
    eolLen += uMap->mapUnicode(0x0a, eol + eolLen, sizeof(eol) - eolLen);
    break;
  case eolMac:
    eolLen = uMap->mapUnicode(0x0d, eol, sizeof(eol));
    break;
  }
  eopLen = uMap->mapUnicode(0x0c, eop, sizeof(eop));

  // output
  for (line = lines; line; line = line->next) {
    col = 0;
    for (blk = line->blocks; blk; blk = blk->next) {

      // line this block up with the correct column
      if (rawOrder && col == 0) {
	col = blk->col[0];
    } else {
	for (; col < blk->col[0]; ++col) {
	(*outputFunc)(outputStream, space, spaceLen);
      }
    }

      // print the block
      for (i = 0; i < blk->len; ++i) {
	len = uMap->mapUnicode(blk->text[i], buf, sizeof(buf));
	(*outputFunc)(outputStream, buf, len);
    }
      col += blk->convertedLen;
    }

      // print a return
      (*outputFunc)(outputStream, eol, eolLen);

      // print extra vertical space if necessary
    if (line->next) {
      d = (int)((line->next->yMin - line->yMax) /
		(line->blocks->strings->yMax - lines->blocks->strings->yMin)
		+ 0.5);
	// various things (weird font matrices) can result in bogus
	// values here, so do a sanity check
	if (rawOrder && d > 2) {
	  d = 2;
	} else if (!rawOrder && d > 5) {
	  d = 5;
	}
	for (; d > 0; --d) {
	  (*outputFunc)(outputStream, eol, eolLen);
	}
      }
  }

  // end of page
  (*outputFunc)(outputStream, eol, eolLen);
  (*outputFunc)(outputStream, eop, eopLen);
  (*outputFunc)(outputStream, eol, eolLen);

  uMap->decRefCnt();
}

// Returns true if <str1> should be inserted before <str2> in xy
// order.
GBool TextPage::xyBefore(TextString *str1, TextString *str2) {
  return str1->xMin < str2->xMin ||
	 (str1->xMin == str2->xMin && str1->yMin < str2->yMin);
}

// Returns true if <blk1> should be inserted before <blk2> in xy
// order.
GBool TextPage::xyBefore(TextBlock *blk1, TextBlock *blk2) {
  return blk1->xMin < blk2->xMin ||
	 (blk1->xMin == blk2->xMin && blk1->yMin < blk2->yMin);
}

// Returns true if <blk1> should be inserted before <blk2> in yx
// order, allowing a little slack for vertically overlapping text.
GBool TextPage::yxBefore(TextBlock *blk1, TextBlock *blk2) {
  double h1, h2, overlap;

  h1 = blk1->yMax - blk1->yMin;
  h2 = blk2->yMax - blk2->yMin;
  overlap = ((blk1->yMax < blk2->yMax ? blk1->yMax : blk2->yMax) -
	     (blk1->yMin > blk2->yMin ? blk1->yMin : blk2->yMin)) /
            (h1 < h2 ? h1 : h2);
  if (overlap > 0.6) {
    return blk1->xMin < blk2->xMin;
  }
  return blk1->yMin < blk2->yMin;
}

double TextPage::coalesceFit(TextString *str1, TextString *str2) {
  double h1, h2, w1, w2, r, overlap, spacing;

  h1 = str1->yMax - str1->yMin;
  h2 = str2->yMax - str2->yMin;
  w1 = str1->xMax - str1->xMin;
  w2 = str2->xMax - str2->xMin;
  r = h1 / h2;
  if (r < (1.0 / 3.0) || r > 3) {
    return 10;
  }
  overlap = ((str1->yMax < str2->yMax ? str1->yMax : str2->yMax) -
	     (str1->yMin > str2->yMin ? str1->yMin : str2->yMin)) /
            (h1 < h2 ? h1 : h2);
  if (overlap < 0.5) {
    return 10;
  }
  spacing = (str2->xMin - str1->xMax) / (h1 > h2 ? h1 : h2);
  if (spacing < -0.5) {
    return 10;
  }
  // separate text that overlaps - duplicated text (so that fake
  // boldface and shadowed text can be cleanly removed)
  if ((str2->xMin - str1->xMax) / (w1 < w2 ? w1 : w2) < -0.7) {
    return 10;
  }
  return spacing;
}

void TextPage::clear() {
  TextLine *p1, *p2;
  TextString *s1, *s2;

  if (curStr) {
    delete curStr;
    curStr = NULL;
  }
  if (lines) {
    for (p1 = lines; p1; p1 = p2) {
      p2 = p1->next;
    delete p1;
  }
  } else if (xyStrings) {
    for (s1 = xyStrings; s1; s1 = s2) {
      s2 = s1->next;
      delete s1;
    }
  }
  xyStrings = NULL;
  xyCur1 = xyCur2 = NULL;
  lines = NULL;
  nest = 0;
  nTinyChars = 0;
}

//------------------------------------------------------------------------
// TextOutputDev
//------------------------------------------------------------------------

static void outputToFile(void *stream, char *text, int len) {
  fwrite(text, 1, len, (FILE *)stream);
}

TextOutputDev::TextOutputDev(char *fileName, GBool rawOrderA, GBool append) {
  text = NULL;
  rawOrder = rawOrderA;
  ok = gTrue;

  // open file
  needClose = gFalse;
  if (fileName) {
    if (!strcmp(fileName, "-")) {
      outputStream = stdout;
    } else if ((outputStream = fopen(fileName, append ? "ab" : "wb"))) {
      needClose = gTrue;
    } else {
      error(-1, "Couldn't open text file '%s'", fileName);
      ok = gFalse;
      return;
    }
    outputFunc = &outputToFile;
  } else {
    outputStream = NULL;
  }

  // set up text object
  text = new TextPage(rawOrder);
}

TextOutputDev::TextOutputDev(TextOutputFunc func, void *stream,
			     GBool rawOrderA) {
  outputFunc = func;
  outputStream = stream;
  needClose = gFalse;
  rawOrder = rawOrderA;
  text = new TextPage(rawOrder);
  ok = gTrue;
}

TextOutputDev::~TextOutputDev() {
  if (needClose) {
#ifdef MACOS
    ICS_MapRefNumAndAssign((short)((FILE *)outputStream)->handle);
#endif
    fclose((FILE *)outputStream);
  }
  if (text) {
    delete text;
  }
}

void TextOutputDev::startPage(int pageNum, GfxState *state) {
  text->clear();
}

void TextOutputDev::endPage() {
  text->coalesce();
  if (outputStream) {
    text->dump(outputStream, outputFunc);
  }
}

void TextOutputDev::updateFont(GfxState *state) {
  text->updateFont(state);
}

void TextOutputDev::beginString(GfxState *state, GString *s) {
  text->beginString(state, state->getCurX(), state->getCurY());
}

void TextOutputDev::endString(GfxState *state) {
  text->endString();
}

void TextOutputDev::drawChar(GfxState *state, double x, double y,
			     double dx, double dy,
			     double originX, double originY,
			     CharCode c, Unicode *u, int uLen) {
  text->addChar(state, x, y, dx, dy, u, uLen);
}

GBool TextOutputDev::findText(Unicode *s, int len,
			      GBool top, GBool bottom,
			      double *xMin, double *yMin,
			      double *xMax, double *yMax) {
  return text->findText(s, len, top, bottom, xMin, yMin, xMax, yMax);
}

GString *TextOutputDev::getText(double xMin, double yMin,
				double xMax, double yMax) {
  return text->getText(xMin, yMin, xMax, yMax);
}

