//========================================================================
//
// TextOutputDev.cc
//
// Copyright 1997-2002 Glyph & Cog, LLC
//
//========================================================================

#ifdef __GNUC__
#pragma implementation
#endif

#include <aconf.h>
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
// TextString
//------------------------------------------------------------------------

TextString::TextString(GfxState *state, double fontSize) {
  GfxFont *font;
  double x, y;

  state->transform(state->getCurX(), state->getCurY(), &x, &y);
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
  col = 0;
  text = NULL;
  xRight = NULL;
  len = size = 0;
  yxNext = NULL;
  xyNext = NULL;
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
  yxStrings = NULL;
  xyStrings = NULL;
  yxCur1 = yxCur2 = NULL;
  nest = 0;
}

TextPage::~TextPage() {
  clear();
}

void TextPage::updateFont(GfxState *state) {
  GfxFont *font;
  double *fm;
  char *name;
  int code;
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
    for (code = 0; code < 256; ++code) {
      if ((name = ((Gfx8BitFont *)font)->getCharName(code)) &&
	  name[0] == 'm' && name[1] == '\0') {
	break;
      }
    }
    if (code < 256) {
      w = ((Gfx8BitFont *)font)->getWidth(code);
      if (w != 0) {
	// 600 is a generic average 'm' width -- yes, this is a hack
	fontSize *= w / 0.6;
      }
    }
    fm = font->getFontMatrix();
    if (fm[0] != 0) {
      fontSize *= fabs(fm[3] / fm[0]);
    }
  }
}

void TextPage::beginString(GfxState *state) {
  // This check is needed because Type 3 characters can contain
  // text-drawing operations.
  if (curStr) {
    ++nest;
    return;
  }

  curStr = new TextString(state, fontSize);
}

void TextPage::addChar(GfxState *state, double x, double y,
		       double dx, double dy, Unicode *u, int uLen) {
  double x1, y1, w1, h1, dx2, dy2;
  int n, i;

  state->transform(x, y, &x1, &y1);
  n = curStr->len;
  if (n > 0 &&
      x1 - curStr->xRight[n-1] > 0.1 * (curStr->yMax - curStr->yMin)) {
    endString();
    beginString(state);
  }
  state->textTransformDelta(state->getCharSpace() * state->getHorizScaling(),
			    0, &dx2, &dy2);
  dx -= dx2;
  dy -= dy2;
  state->transformDelta(dx, dy, &w1, &h1);
  if (uLen != 0) {
    w1 /= uLen;
    h1 /= uLen;
  }
  for (i = 0; i < uLen; ++i) {
    curStr->addChar(state, x1 + i*w1, y1 + i*h1, w1, h1, u[i]);
  }
}

void TextPage::endString() {
  TextString *p1, *p2;
  double h, y1, y2;

  // This check is needed because Type 3 characters can contain
  // text-drawing operations.
  if (nest > 0) {
    --nest;
    return;
  }

  // throw away zero-length strings -- they don't have valid xMin/xMax
  // values, and they're useless anyway
  if (curStr->len == 0) {
    delete curStr;
    curStr = NULL;
    return;
  }

  // insert string in y-major list
  h = curStr->yMax - curStr->yMin;
  y1 = curStr->yMin + 0.5 * h;
  y2 = curStr->yMin + 0.8 * h;
  if (rawOrder) {
    p1 = yxCur1;
    p2 = NULL;
  } else if ((!yxCur1 ||
	      (y1 >= yxCur1->yMin &&
	       (y2 >= yxCur1->yMax || curStr->xMax >= yxCur1->xMin))) &&
	     (!yxCur2 ||
	      (y1 < yxCur2->yMin ||
	       (y2 < yxCur2->yMax && curStr->xMax < yxCur2->xMin)))) {
    p1 = yxCur1;
    p2 = yxCur2;
  } else {
    for (p1 = NULL, p2 = yxStrings; p2; p1 = p2, p2 = p2->yxNext) {
      if (y1 < p2->yMin || (y2 < p2->yMax && curStr->xMax < p2->xMin)) {
	break;
      }
    }
    yxCur2 = p2;
  }
  yxCur1 = curStr;
  if (p1) {
    p1->yxNext = curStr;
  } else {
    yxStrings = curStr;
  }
  curStr->yxNext = p2;
  curStr = NULL;
}

void TextPage::coalesce() {
  TextString *str1, *str2;
  double space, d;
  GBool addSpace;
  int n, i;

#if 0 //~ for debugging
  for (str1 = yxStrings; str1; str1 = str1->yxNext) {
    printf("x=%3d..%3d  y=%3d..%3d  size=%2d '",
	   (int)str1->xMin, (int)str1->xMax, (int)str1->yMin, (int)str1->yMax,
	   (int)(str1->yMax - str1->yMin));
    for (i = 0; i < str1->len; ++i) {
      fputc(str1->text[i] & 0xff, stdout);
    }
    printf("'\n");
  }
  printf("\n------------------------------------------------------------\n\n");
#endif
  str1 = yxStrings;
  while (str1 && (str2 = str1->yxNext)) {
    space = str1->yMax - str1->yMin;
    d = str2->xMin - str1->xMax;
    if (((rawOrder &&
	  ((str2->yMin >= str1->yMin && str2->yMin <= str1->yMax) ||
	   (str2->yMax >= str1->yMin && str2->yMax <= str1->yMax))) ||
	 (!rawOrder && str2->yMin < str1->yMax)) &&
	d > -0.5 * space && d < space) {
      n = str1->len + str2->len;
      if ((addSpace = d > 0.1 * space)) {
	++n;
      }
      str1->size = (n + 15) & ~15;
      str1->text = (Unicode *)grealloc(str1->text,
				       str1->size * sizeof(Unicode));
      str1->xRight = (double *)grealloc(str1->xRight,
					str1->size * sizeof(double));
      if (addSpace) {
	str1->text[str1->len] = 0x20;
	str1->xRight[str1->len] = str2->xMin;
	++str1->len;
      }
      for (i = 0; i < str2->len; ++i) {
	str1->text[str1->len] = str2->text[i];
	str1->xRight[str1->len] = str2->xRight[i];
	++str1->len;
      }
      if (str2->xMax > str1->xMax) {
	str1->xMax = str2->xMax;
      }
      if (str2->yMax > str1->yMax) {
	str1->yMax = str2->yMax;
      }
      str1->yxNext = str2->yxNext;
      delete str2;
    } else {
      str1 = str2;
    }
  }
}

GBool TextPage::findText(Unicode *s, int len,
			 GBool top, GBool bottom,
			 double *xMin, double *yMin,
			 double *xMax, double *yMax) {
  TextString *str;
  Unicode *p;
  Unicode u1, u2;
  int m, i, j;
  double x;

  // scan all strings on page
  for (str = yxStrings; str; str = str->yxNext) {

    // check: above top limit?
    if (!top && (str->yMax < *yMin ||
		 (str->yMin < *yMin && str->xMax <= *xMin))) {
      continue;
    }

    // check: below bottom limit?
    if (!bottom && (str->yMin > *yMax ||
		    (str->yMax > *yMax && str->xMin >= *xMax))) {
      return gFalse;
    }

    // search each position in this string
    m = str->len;
    for (i = 0, p = str->text; i <= m - len; ++i, ++p) {

      // check: above top limit?
      if (!top && str->yMin < *yMin) {
	x = (((i == 0) ? str->xMin : str->xRight[i-1]) + str->xRight[i]) / 2;
	if (x < *xMin) {
	  continue;
	}
      }

      // check: below bottom limit?
      if (!bottom && str->yMax > *yMax) {
	x = (((i == 0) ? str->xMin : str->xRight[i-1]) + str->xRight[i]) / 2;
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
	*xMin = (i == 0) ? str->xMin : str->xRight[i-1];
	*xMax = str->xRight[i + len - 1];
	*yMin = str->yMin;
	*yMax = str->yMax;
	return gTrue;
      }
    }
  }
  return gFalse;
}

GString *TextPage::getText(double xMin, double yMin,
			   double xMax, double yMax) {
  GString *s;
  UnicodeMap *uMap;
  char space[8], eol[16], buf[8];
  int spaceLen, eolLen, n;
  TextString *str1;
  double x0, x1, x2, y;
  double xPrev, yPrev;
  int i1, i2, i;
  GBool multiLine;

  s = new GString();
  if (!(uMap = globalParams->getTextEncoding())) {
    return s;
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
  xPrev = yPrev = 0;
  multiLine = gFalse;
  for (str1 = yxStrings; str1; str1 = str1->yxNext) {
    y = 0.5 * (str1->yMin + str1->yMax);
    if (y > yMax) {
      break;
    }
    if (y > yMin && str1->xMin < xMax && str1->xMax > xMin) {
      x0 = x1 = x2 = str1->xMin;
      for (i1 = 0; i1 < str1->len; ++i1) {
	x0 = (i1==0) ? str1->xMin : str1->xRight[i1-1];
	x1 = str1->xRight[i1];
	if (0.5 * (x0 + x1) >= xMin) {
	  break;
	}
      }
      for (i2 = str1->len - 1; i2 > i1; --i2) {
	x1 = (i2==0) ? str1->xMin : str1->xRight[i2-1];
	x2 = str1->xRight[i2];
	if (0.5 * (x1 + x2) <= xMax) {
	  break;
	}
      }
      if (s->getLength() > 0) {
	if (x0 < xPrev || str1->yMin > yPrev) {
	  s->append(eol, eolLen);
	  multiLine = gTrue;
	} else {
	  for (i = 0; i < 4; ++i) {
	    s->append(space, spaceLen);
	  }
	}
      }
      for (i = i1; i <= i2; ++i) {
	n = uMap->mapUnicode(str1->text[i], buf, sizeof(buf));
	s->append(buf, n);
      }
      xPrev = x2;
      yPrev = str1->yMax;
    }
  }
  if (multiLine) {
    s->append(eol, eolLen);
  }
  uMap->decRefCnt();
  return s;
}

void TextPage::dump(void *outputStream, TextOutputFunc outputFunc) {
  UnicodeMap *uMap;
  char space[8], eol[16], eop[8], buf[8];
  int spaceLen, eolLen, eopLen, n;
  TextString *str1, *str2, *str3;
  double yMin, yMax;
  int col1, col2, d, i;

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

  // build x-major list
  xyStrings = NULL;
  for (str1 = yxStrings; str1; str1 = str1->yxNext) {
    for (str2 = NULL, str3 = xyStrings;
	 str3;
	 str2 = str3, str3 = str3->xyNext) {
      if (str1->xMin < str3->xMin ||
	  (str1->xMin == str3->xMin && str1->yMin < str3->yMin)) {
	break;
      }
    }
    if (str2) {
      str2->xyNext = str1;
    } else {
      xyStrings = str1;
    }
    str1->xyNext = str3;
  }

  // do column assignment
  for (str1 = xyStrings; str1; str1 = str1->xyNext) {
    col1 = 0;
    for (str2 = xyStrings; str2 != str1; str2 = str2->xyNext) {
      if (str1->xMin >= str2->xMax) {
	col2 = str2->col + str2->len + 4;
	if (col2 > col1) {
	  col1 = col2;
	}
      } else if (str1->xMin > str2->xMin) {
	col2 = str2->col +
	       (int)(((str1->xMin - str2->xMin) / (str2->xMax - str2->xMin)) *
		     str2->len);
	if (col2 > col1) {
	  col1 = col2;
	}
      }
    }
    str1->col = col1;
  }

#if 0 //~ for debugging
  fprintf((FILE *)outputStream, "~~~~~~~~~~\n");
  for (str1 = yxStrings; str1; str1 = str1->yxNext) {
    fprintf((FILE *)outputStream, "(%4d,%4d) - (%4d,%4d) [%3d] '",
	    (int)str1->xMin, (int)str1->yMin,
	    (int)str1->xMax, (int)str1->yMax, str1->col);
    for (i = 0; i < str1->len; ++i) {
      fputc(str1->text[i] & 0xff, stdout);
    }
    printf("'\n");
  }
  fprintf((FILE *)outputStream, "~~~~~~~~~~\n");
#endif

  // output
  col1 = 0;
  yMax = yxStrings ? yxStrings->yMax : 0;
  for (str1 = yxStrings; str1; str1 = str1->yxNext) {

    // line this string up with the correct column
    if (rawOrder && col1 == 0) {
      col1 = str1->col;
    } else {
      for (; col1 < str1->col; ++col1) {
	(*outputFunc)(outputStream, space, spaceLen);
      }
    }

    // print the string
    for (i = 0; i < str1->len; ++i) {
      if ((n = uMap->mapUnicode(str1->text[i], buf, sizeof(buf))) > 0) {
	(*outputFunc)(outputStream, buf, n);
      }
    }

    // increment column
    col1 += str1->len;

    // update yMax for this line
    if (str1->yMax > yMax) {
      yMax = str1->yMax;
    }

    // if we've hit the end of the line...
    if (!(str1->yxNext &&
	  !(rawOrder && str1->yxNext->yMax < str1->yMin) &&
	  str1->yxNext->yMin < 0.2*str1->yMin + 0.8*str1->yMax &&
	  str1->yxNext->xMin >= str1->xMax)) {

      // print a return
      (*outputFunc)(outputStream, eol, eolLen);

      // print extra vertical space if necessary
      if (str1->yxNext) {

	// find yMin for next line
	yMin = str1->yxNext->yMin;
	for (str2 = str1->yxNext; str2; str2 = str2->yxNext) {
	  if (str2->yMin < yMin) {
	    yMin = str2->yMin;
	  }
	  if (!(str2->yxNext && str2->yxNext->yMin < str2->yMax &&
		str2->yxNext->xMin >= str2->xMax))
	    break;
	}
	  
	// print the space
	d = (int)((yMin - yMax) / (str1->yMax - str1->yMin) + 0.5);
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

      // set up for next line
      col1 = 0;
      yMax = str1->yxNext ? str1->yxNext->yMax : 0;
    }
  }

  // end of page
  (*outputFunc)(outputStream, eol, eolLen);
  (*outputFunc)(outputStream, eop, eopLen);
  (*outputFunc)(outputStream, eol, eolLen);

  uMap->decRefCnt();
}

void TextPage::clear() {
  TextString *p1, *p2;

  if (curStr) {
    delete curStr;
    curStr = NULL;
  }
  for (p1 = yxStrings; p1; p1 = p2) {
    p2 = p1->yxNext;
    delete p1;
  }
  yxStrings = NULL;
  xyStrings = NULL;
  yxCur1 = yxCur2 = NULL;
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
  text->beginString(state);
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
