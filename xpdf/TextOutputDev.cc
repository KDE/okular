//========================================================================
//
// TextOutputDev.cc
//
// Copyright 1997-2003 Glyph & Cog, LLC
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
#ifdef WIN32
#include <fcntl.h> // for O_BINARY
#include <io.h>    // for setmode
#endif
#include "gmem.h"
#include "GString.h"
#include "GList.h"
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
// parameters
//------------------------------------------------------------------------

// Minium and maximum inter-word spacing (as a fraction of the average
// character width).
#define wordMinSpaceWidth 0.3
#define wordMaxSpaceWidth 2.0

// Default min and max inter-word spacing (when the average character
// width is unknown).
#define wordDefMinSpaceWidth 0.2
#define wordDefMaxSpaceWidth 1.5

// Max difference in x,y coordinates (as a fraction of the font size)
// allowed for duplicated text (fake boldface, drop shadows) which is
// to be discarded.
#define dupMaxDeltaX 0.1
#define dupMaxDeltaY 0.2

// Min overlap (as a fraction of the font size) required for two
// lines to be considered vertically overlapping.
#define lineOverlapSlack 0.5

// Max difference in baseline y coordinates (as a fraction of the font
// size) allowed for words which are to be grouped into a line, not
// including sub/superscripts.
#define lineMaxBaselineDelta 0.1

// Max ratio of font sizes allowed for words which are to be grouped
// into a line, not including sub/superscripts.
#define lineMaxFontSizeRatio 1.4

// Min spacing (as a fraction of the font size) allowed between words
// which are to be grouped into a line.
#define lineMinDeltaX -0.5

// Minimum vertical overlap (as a fraction of the font size) required
// for superscript and subscript words.
#define lineMinSuperscriptOverlap 0.3
#define lineMinSubscriptOverlap   0.3

// Min/max ratio of font sizes allowed for sub/superscripts compared to
// the base text.
#define lineMinSubscriptFontSizeRatio   0.4
#define lineMaxSubscriptFontSizeRatio   1.01
#define lineMinSuperscriptFontSizeRatio 0.4
#define lineMaxSuperscriptFontSizeRatio 1.01

// Max horizontal spacing (as a fraction of the font size) allowed
// before sub/superscripts.
#define lineMaxSubscriptDeltaX   0.2
#define lineMaxSuperscriptDeltaX 0.2

// Maximum vertical spacing (as a fraction of the font size) allowed
// for lines which are to be grouped into a block.
#define blkMaxSpacing 2.0

// Max ratio of primary font sizes allowed for lines which are to be
// grouped into a block.
#define blkMaxFontSizeRatio 1.3

// Min overlap (as a fraction of the font size) required for two
// blocks to be considered vertically overlapping.
#define blkOverlapSlack 0.5

// Max vertical spacing (as a fraction of the font size) allowed
// between blocks which are 'adjacent' when sorted by reading order.
#define blkMaxSortSpacing 2.0

// Max vertical offset (as a fraction of the font size) of the top and
// bottom edges allowed for blocks which are to be grouped into a
// flow.
#define flowMaxDeltaY 1.0

//------------------------------------------------------------------------
// TextFontInfo
//------------------------------------------------------------------------

TextFontInfo::TextFontInfo(GfxState *state) {
  double *textMat;
  double t1, t2, avgWidth, w;
  int n, i;

  gfxFont = state->getFont();
  textMat = state->getTextMat();
  horizScaling = state->getHorizScaling();
  if ((t1 = fabs(textMat[0])) > 0.01 &&
      (t2 = fabs(textMat[3])) > 0.01) {
    horizScaling *= t1 / t2;
  }

  minSpaceWidth = horizScaling * wordDefMinSpaceWidth;
  maxSpaceWidth = horizScaling * wordDefMaxSpaceWidth;
  if (gfxFont && gfxFont->isCIDFont()) {
    //~ handle 16-bit fonts
  } else if (gfxFont && gfxFont->getType() != fontType3) {
    avgWidth = 0;
    n = 0;
    for (i = 0; i < 256; ++i) {
      w = ((Gfx8BitFont *)gfxFont)->getWidth(i);
      if (w > 0) {
	avgWidth += w;
	++n;
      }
    }
    if (n > 0) {
      avgWidth /= n;
      minSpaceWidth = horizScaling * wordMinSpaceWidth * avgWidth;
      maxSpaceWidth = horizScaling * wordMaxSpaceWidth * avgWidth;
    }
  }

}

TextFontInfo::~TextFontInfo() {
}

GBool TextFontInfo::matches(GfxState *state) {
  double *textMat;
  double t1, t2, h;

  textMat = state->getTextMat();
  h = state->getHorizScaling();
  if ((t1 = fabs(textMat[0])) > 0.01 &&
      (t2 = fabs(textMat[3])) > 0.01) {
    h *= t1 / t2;
  }
  return state->getFont() == gfxFont &&
         fabs(h - horizScaling) < 0.01;
}

//------------------------------------------------------------------------
// TextWord
//------------------------------------------------------------------------

TextWord::TextWord(GfxState *state, double x0, double y0, int charPosA,
		   TextFontInfo *fontA, double fontSizeA) {
  GfxFont *gfxFont;
  double x, y;

  charPos = charPosA;
  charLen = 0;
  font = fontA;
  fontSize = fontSizeA;
  state->transform(x0, y0, &x, &y);
  if ((gfxFont = font->gfxFont)) {
    yMin = y - gfxFont->getAscent() * fontSize;
    yMax = y - gfxFont->getDescent() * fontSize;
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
  yBase = y;
  text = NULL;
  xRight = NULL;
  len = size = 0;
  spaceAfter = gFalse;
  next = NULL;

}


TextWord::~TextWord() {
  gfree(text);
  gfree(xRight);
}

void TextWord::addChar(GfxState *state, double x, double y,
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

// Returns true if <this> comes before <word2> in xy order.
GBool TextWord::xyBefore(TextWord *word2) {
  return xMin < word2->xMin ||
	 (xMin == word2->xMin && yMin < word2->yMin);
}

// Merge another word onto the end of this one.
void TextWord::merge(TextWord *word2) {
  int i;

  xMax = word2->xMax;
  if (word2->yMin < yMin) {
    yMin = word2->yMin;
  }
  if (word2->yMax > yMax) {
    yMax = word2->yMax;
  }
  if (len + word2->len > size) {
    size = len + word2->len;
    text = (Unicode *)grealloc(text, size * sizeof(Unicode));
    xRight = (double *)grealloc(xRight, size * sizeof(double));
  }
  for (i = 0; i < word2->len; ++i) {
    text[len + i] = word2->text[i];
    xRight[len + i] = word2->xRight[i];
  }
  len += word2->len;
  charLen += word2->charLen;
}

//------------------------------------------------------------------------
// TextLine
//------------------------------------------------------------------------

TextLine::TextLine() {
  words = NULL;
  text = NULL;
  xRight = NULL;
  col = NULL;
  len = 0;
  hyphenated = gFalse;
  pageNext = NULL;
  next = NULL;
  flowNext = NULL;
}

TextLine::~TextLine() {
  TextWord *w1, *w2;

  for (w1 = words; w1; w1 = w2) {
    w2 = w1->next;
    delete w1;
  }
  gfree(text);
  gfree(xRight);
  gfree(col);
}

// Returns true if <this> comes before <line2> in yx order, allowing
// slack for vertically overlapping lines.
GBool TextLine::yxBefore(TextLine *line2) {
  double dy;

  dy = lineOverlapSlack * fontSize;

  // non-overlapping case
  if (line2->yMin > yMax - dy ||
      line2->yMax < yMin + dy) {
    return yMin < line2->yMin ||
           (yMin == line2->yMin && xMin < line2->xMin);
  }

  // overlapping case
  return xMin < line2->xMin;
}

// Merge another line's words onto the end of this line.
void TextLine::merge(TextLine *line2) {
  int newLen, i;

  xMax = line2->xMax;
  if (line2->yMin < yMin) {
    yMin = line2->yMin;
  }
  if (line2->yMax > yMax) {
    yMax = line2->yMax;
  }
  xSpaceR = line2->xSpaceR;
  lastWord->spaceAfter = gTrue;
  lastWord->next = line2->words;
  lastWord = line2->lastWord;
  line2->words = NULL;
  newLen = len + 1 + line2->len;
  text = (Unicode *)grealloc(text, newLen * sizeof(Unicode));
  xRight = (double *)grealloc(xRight, newLen * sizeof(double));
  text[len] = (Unicode)0x0020;
  xRight[len] = line2->xMin;
  for (i = 0; i < line2->len; ++i) {
    text[len + 1 + i] = line2->text[i];
    xRight[len + 1 + i] = line2->xRight[i];
  }
  len = newLen;
  convertedLen += line2->convertedLen;
  hyphenated = line2->hyphenated;
}

//------------------------------------------------------------------------
// TextBlock
//------------------------------------------------------------------------

TextBlock::TextBlock() {
  lines = NULL;
  next = NULL;
}

TextBlock::~TextBlock() {
  TextLine *l1, *l2;

  for (l1 = lines; l1; l1 = l2) {
    l2 = l1->next;
    delete l1;
  }
}

// Returns true if <this> comes before <blk2> in xy order, allowing
// slack for vertically overlapping blocks.
GBool TextBlock::yxBefore(TextBlock *blk2) {
  double dy;

  dy = blkOverlapSlack * lines->fontSize;

  // non-overlapping case
  if (blk2->yMin > yMax - dy ||
      blk2->yMax < yMin + dy) {
    return yMin < blk2->yMin ||
           (yMin == blk2->yMin && xMin < blk2->xMin);
  }

  // overlapping case
  return xMin < blk2->xMin;
}

// Merge another block's line onto the right of this one.
void TextBlock::mergeRight(TextBlock *blk2) {
  lines->merge(blk2->lines);
  xMax = lines->xMax;
  yMin = lines->yMin;
  yMax = lines->yMax;
  xSpaceR = lines->xSpaceR;
}

// Merge another block's lines onto the bottom of this block.
void TextBlock::mergeBelow(TextBlock *blk2) {
  TextLine *line;

  if (blk2->xMin < xMin) {
    xMin = blk2->xMin;
  }
  if (blk2->xMax > xMax) {
    xMax = blk2->xMax;
  }
  yMax = blk2->yMax;
  if (blk2->xSpaceL > xSpaceL) {
    xSpaceL = blk2->xSpaceL;
  }
  if (blk2->xSpaceR < xSpaceR) {
    xSpaceR = blk2->xSpaceR;
  }
  if (blk2->maxFontSize > maxFontSize) {
    maxFontSize = blk2->maxFontSize;
  }
  for (line = lines; line->next; line = line->next) ;
  line->next = line->flowNext = blk2->lines;
  blk2->lines = NULL;
}

//------------------------------------------------------------------------
// TextFlow
//------------------------------------------------------------------------

TextFlow::TextFlow() {
  blocks = NULL;
  next = NULL;
}

TextFlow::~TextFlow() {
  TextBlock *b1, *b2;

  for (b1 = blocks; b1; b1 = b2) {
    b2 = b1->next;
    delete b1;
  }
}


//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------

TextPage::TextPage(GBool rawOrderA) {
  rawOrder = rawOrderA;
  curWord = NULL;
  charPos = 0;
  font = NULL;
  fontSize = 0;
  nest = 0;
  nTinyChars = 0;
  words = wordPtr = NULL;
  lines = NULL;
  flows = NULL;
  fonts = new GList();
}

TextPage::~TextPage() {
  clear();
  delete fonts;
}

void TextPage::updateFont(GfxState *state) {
  GfxFont *gfxFont;
  double *fm;
  char *name;
  int code, mCode, letterCode, anyCode;
  double w;
  int i;

  // get the font info object
  font = NULL;
  for (i = 0; i < fonts->getLength(); ++i) {
    font = (TextFontInfo *)fonts->get(i);
    if (font->matches(state)) {
      break;
    }
    font = NULL;
  }
  if (!font) {
    font = new TextFontInfo(state);
    fonts->append(font);
  }

  // adjust the font size
  gfxFont = state->getFont();
  fontSize = state->getTransformedFontSize();
  if (gfxFont && gfxFont->getType() == fontType3) {
    // This is a hack which makes it possible to deal with some Type 3
    // fonts.  The problem is that it's impossible to know what the
    // base coordinate system used in the font is without actually
    // rendering the font.  This code tries to guess by looking at the
    // width of the character 'm' (which breaks if the font is a
    // subset that doesn't contain 'm').
    mCode = letterCode = anyCode = -1;
    for (code = 0; code < 256; ++code) {
      name = ((Gfx8BitFont *)gfxFont)->getCharName(code);
      if (name && name[0] == 'm' && name[1] == '\0') {
	mCode = code;
      }
      if (letterCode < 0 && name && name[1] == '\0' &&
	  ((name[0] >= 'A' && name[0] <= 'Z') ||
	   (name[0] >= 'a' && name[0] <= 'z'))) {
	letterCode = code;
      }
      if (anyCode < 0 && name &&
	  ((Gfx8BitFont *)gfxFont)->getWidth(code) > 0) {
	anyCode = code;
      }
    }
    if (mCode >= 0 &&
	(w = ((Gfx8BitFont *)gfxFont)->getWidth(mCode)) > 0) {
      // 0.6 is a generic average 'm' width -- yes, this is a hack
      fontSize *= w / 0.6;
    } else if (letterCode >= 0 &&
	       (w = ((Gfx8BitFont *)gfxFont)->getWidth(letterCode)) > 0) {
      // even more of a hack: 0.5 is a generic letter width
      fontSize *= w / 0.5;
    } else if (anyCode >= 0 &&
	       (w = ((Gfx8BitFont *)gfxFont)->getWidth(anyCode)) > 0) {
      // better than nothing: 0.5 is a generic character width
      fontSize *= w / 0.5;
    }
    fm = gfxFont->getFontMatrix();
    if (fm[0] != 0) {
      fontSize *= fabs(fm[3] / fm[0]);
    }
  }
}

void TextPage::beginWord(GfxState *state, double x0, double y0) {
  // This check is needed because Type 3 characters can contain
  // text-drawing operations (when TextPage is being used via
  // XOutputDev rather than TextOutputDev).
  if (curWord) {
    ++nest;
    return;
  }

  curWord = new TextWord(state, x0, y0, charPos, font, fontSize);
}

void TextPage::addChar(GfxState *state, double x, double y,
		       double dx, double dy,
		       CharCode c, Unicode *u, int uLen) {
  double x1, y1, w1, h1, dx2, dy2, sp;
  int n, i;

  // if the previous char was a space, addChar will have called
  // endWord, so we need to start a new word
  if (!curWord) {
    beginWord(state, x, y);
  }

  // throw away chars that aren't inside the page bounds
  state->transform(x, y, &x1, &y1);
  if (x1 < 0 || x1 > pageWidth ||
      y1 < 0 || y1 > pageHeight) {
    return;
  }

  // subtract char and word spacing from the dx,dy values
  sp = state->getCharSpace();
  if (c == (CharCode)0x20) {
    sp += state->getWordSpace();
  }
  state->textTransformDelta(sp * state->getHorizScaling(), 0, &dx2, &dy2);
  dx -= dx2;
  dy -= dy2;
  state->transformDelta(dx, dy, &w1, &h1);

  // check the tiny chars limit
  if (!globalParams->getTextKeepTinyChars() &&
      fabs(w1) < 3 && fabs(h1) < 3) {
    if (++nTinyChars > 20000) {
      return;
    }
  }

  // break words at space character
  if (uLen == 1 && u[0] == (Unicode)0x20) {
    ++curWord->charLen;
    ++charPos;
    endWord();
    return;
  }

  // large char spacing is sometimes used to move text around -- in
  // this case, break text into individual chars and let the coalesce
  // function deal with it later
  n = curWord->len;
  if (n > 0 && x1 - curWord->xRight[n-1] >
                    curWord->font->minSpaceWidth * curWord->fontSize) {
    endWord();
    beginWord(state, x, y);
  }

  // page rotation and/or transform matrices can cause text to be
  // drawn in reverse order -- in this case, swap the begin/end
  // coordinates and break text into individual chars
  if (w1 < 0) {
    endWord();
    beginWord(state, x + dx, y + dy);
    x1 += w1;
    y1 += h1;
    w1 = -w1;
    h1 = -h1;
  }

  // add the characters to the current word
  if (uLen != 0) {
    w1 /= uLen;
    h1 /= uLen;
  }
  for (i = 0; i < uLen; ++i) {
    curWord->addChar(state, x1 + i*w1, y1 + i*h1, w1, h1, u[i]);
  }
  ++curWord->charLen;
  ++charPos;
}

void TextPage::endWord() {
  // This check is needed because Type 3 characters can contain
  // text-drawing operations (when TextPage is being used via
  // XOutputDev rather than TextOutputDev).
  if (nest > 0) {
    --nest;
    return;
  }

  if (curWord) {
    addWord(curWord);
    curWord = NULL;
  }
}

void TextPage::addWord(TextWord *word) {
  TextWord *p1, *p2;

  // throw away zero-length words -- they don't have valid xMin/xMax
  // values, and they're useless anyway
  if (word->len == 0) {
    delete word;
    return;
  }

  // insert word in xy list
  if (rawOrder) {
    p1 = wordPtr;
    p2 = NULL;
  } else {
    if (wordPtr && wordPtr->xyBefore(word)) {
      p1 = wordPtr;
      p2 = wordPtr->next;
    } else {
      p1 = NULL;
      p2 = words;
    }
    for (; p2; p1 = p2, p2 = p2->next) {
      if (word->xyBefore(p2)) {
	break;
      }
    }
  }
  if (p1) {
    p1->next = word;
  } else {
    words = word;
  }
  word->next = p2;
  wordPtr = word;
}

void TextPage::coalesce(GBool physLayout) {
  TextWord *word0, *word1, *word2;
  TextLine *line0, *line1, *line2, *line3, *line4, *lineList;
  TextBlock *blk0, *blk1, *blk2, *blk3, *blk4, *blk5, *blk6;
  TextBlock *yxBlocks, *blocks, *blkStack;
  TextFlow *flow0, *flow1;
  double sz, xLimit, yLimit;
  double fit1, fit2, sp1, sp2;
  GBool found;
  UnicodeMap *uMap;
  GBool isUnicode;
  char buf[8];
  int col1, col2, d, i, j;

#if 0 // for debugging
  printf("*** initial word list ***\n");
  for (word0 = words; word0; word0 = word0->next) {
    printf("word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f: '",
	   word0->xMin, word0->xMax, word0->yMin, word0->yMax, word0->yBase);
    for (i = 0; i < word0->len; ++i) {
      fputc(word0->text[i] & 0xff, stdout);
    }
    printf("'\n");
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- discard duplicated text (fake boldface, drop shadows)

  word0 = words;
  while (word0) {
    sz = word0->fontSize;
    xLimit = word0->xMin + sz * dupMaxDeltaX;
    found = gFalse;
    for (word1 = word0, word2 = word0->next;
	 word2 && word2->xMin < xLimit;
	 word1 = word2, word2 = word2->next) {
      if (word2->len == word0->len &&
	  !memcmp(word2->text, word0->text, word0->len * sizeof(Unicode)) &&
	  fabs(word2->yMin - word0->yMin) < sz * dupMaxDeltaY &&
	  fabs(word2->yMax - word0->yMax) < sz * dupMaxDeltaY &&
	  fabs(word2->xMax - word0->xMax) < sz * dupMaxDeltaX) {
	found = gTrue;
	break;
      }
    }
    if (found) {
      word1->next = word2->next;
      delete word2;
    } else {
      word0 = word0->next;
    }
  }

#if 0 // for debugging
  printf("*** words after removing duplicate text ***\n");
  for (word0 = words; word0; word0 = word0->next) {
    printf("word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f: '",
	   word0->xMin, word0->xMax, word0->yMin, word0->yMax, word0->yBase);
    for (i = 0; i < word0->len; ++i) {
      fputc(word0->text[i] & 0xff, stdout);
    }
    printf("'\n");
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- merge words

  word0 = words;
  while (word0) {
    sz = word0->fontSize;

    // look for adjacent text which is part of the same word, and
    // merge it into this word
    xLimit = word0->xMax + sz * word0->font->minSpaceWidth;
    if (rawOrder) {
      word1 = word0;
      word2 = word0->next;
      found = word2 &&
	      word2->xMin < xLimit &&
	      word2->font == word0->font &&
	      fabs(word2->fontSize - sz) < 0.05 &&
	      fabs(word2->yBase - word0->yBase) < 0.05 &&
	      word2->charPos == word0->charPos + word0->charLen;
    } else {
      found = gFalse;
      for (word1 = word0, word2 = word0->next;
	   word2 && word2->xMin < xLimit;
	   word1 = word2, word2 = word2->next) {
	if (word2->font == word0->font &&
	    fabs(word2->fontSize - sz) < 0.05 &&
	    fabs(word2->yBase - word0->yBase) < 0.05 &&
	    word2->charPos == word0->charPos + word0->charLen) {
	  found = gTrue;
	  break;
	}
      }
    }
    if (found) {
      word0->merge(word2);
      word1->next = word2->next;
      delete word2;
      continue;
    }

    word0 = word0->next;
  }

#if 0 // for debugging
  printf("*** after merging words ***\n");
  for (word0 = words; word0; word0 = word0->next) {
    printf("word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f: '",
	   word0->xMin, word0->xMax, word0->yMin, word0->yMax, word0->yBase);
    for (i = 0; i < word0->len; ++i) {
      fputc(word0->text[i] & 0xff, stdout);
    }
    printf("'\n");
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- assemble words into lines

  lineList = line0 = NULL;
  while (words) {

    // remove the first word from the word list
    word0 = words;
    words = words->next;
    word0->next = NULL;

    // find the best line (if any) for the word
    if (rawOrder) {
      if (line0 && lineFit(line0, word0, &sp2) >= 0) {
	line1 = line0;
	sp1 = sp2;
      } else {
	line1 = NULL;
	sp1 = 0;
      }
    } else {
      line1 = NULL;
      fit1 = 0;
      sp1 = 0;
      for (line2 = lineList; line2; line2 = line2->next) {
	fit2 = lineFit(line2, word0, &sp2);
	if (fit2 >= 0 && (!line1 || fit2 < fit1)) {
	  line1 = line2;
	  fit1 = fit2;
	  sp1 = sp2;
	}
      }
    }

    // found a line: append the word
    if (line1) {
      word1 = line1->lastWord;
      word1->next = word0;
      line1->lastWord = word0;
      if (word0->xMax > line1->xMax) {
	line1->xMax = word0->xMax;
      }
      if (word0->yMin < line1->yMin) {
	line1->yMin = word0->yMin;
      }
      if (word0->yMax > line1->yMax) {
	line1->yMax = word0->yMax;
      }
      line1->len += word0->len;
      if (sp1 > line1->fontSize * line1->font->minSpaceWidth) {
	word1->spaceAfter = gTrue;
	++line1->len;
      }

    // didn't find a line: create a new line
    } else {
      line1 = new TextLine();
      line1->words = line1->lastWord = word0;
      line1->xMin = word0->xMin;
      line1->xMax = word0->xMax;
      line1->yMin = word0->yMin;
      line1->yMax = word0->yMax;
      line1->yBase = word0->yBase;
      line1->font = word0->font;
      line1->fontSize = word0->fontSize;
      line1->len = word0->len;
      if (line0) {
	line0->next = line1;
      } else {
	lineList = line1;
      }
      line0 = line1;
    }
  }

  // build the line text
  uMap = globalParams->getTextEncoding();
  isUnicode = uMap ? uMap->isUnicode() : gFalse;

  for (line1 = lineList; line1; line1 = line1->next) {
    line1->text = (Unicode *)gmalloc(line1->len * sizeof(Unicode));
    line1->xRight = (double *)gmalloc(line1->len * sizeof(double));
    line1->col = (int *)gmalloc(line1->len * sizeof(int));
    i = 0;
    for (word1 = line1->words; word1; word1 = word1->next) {
      for (j = 0; j < word1->len; ++j) {
	line1->text[i] = word1->text[j];
	line1->xRight[i] = word1->xRight[j];
	++i;
      }
      if (word1->spaceAfter && word1->next) {
	line1->text[i] = (Unicode)0x0020;
	line1->xRight[i] = word1->next->xMin;
	++i;
      }
    }
    line1->convertedLen = 0;
    for (j = 0; j < line1->len; ++j) {
      line1->col[j] = line1->convertedLen;
      if (isUnicode) {
	++line1->convertedLen;
      } else if (uMap) {
	line1->convertedLen +=
	  uMap->mapUnicode(line1->text[j], buf, sizeof(buf));
      }
    }

    // check for hyphen at end of line
    //~ need to check for other chars used as hyphens
    if (line1->text[line1->len - 1] == (Unicode)'-') {
      line1->hyphenated = gTrue;
    }

  }

  if (uMap) {
    uMap->decRefCnt();
  }

#if 0 // for debugging
  printf("*** lines in xy order ***\n");
  for (line0 = lineList; line0; line0 = line0->next) {
    printf("[line: x=%.2f..%.2f y=%.2f..%.2f base=%.2f len=%d]\n",
	   line0->xMin, line0->xMax, line0->yMin, line0->yMax,
	   line0->yBase, line0->len);
    for (word0 = line0->words; word0; word0 = word0->next) {
      printf("  word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f fontSz=%.2f space=%d: '",
	     word0->xMin, word0->xMax, word0->yMin, word0->yMax,
	     word0->yBase, word0->fontSize, word0->spaceAfter);
      for (i = 0; i < word0->len; ++i) {
	fputc(word0->text[i] & 0xff, stdout);
      }
      printf("'\n");
    }
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- column assignment

  for (line1 = lineList; line1; line1 = line1->next) {
    col1 = 0;
    for (line2 = lineList; line2 != line1; line2 = line2->next) {
      if (line1->xMin >= line2->xMax) {
	d = (int)((line1->xMin - line2->xMax) /
		  (line1->font->maxSpaceWidth * line1->fontSize));
	if (d > 4) {
	  d = 4;
	}
	col2 = line2->col[0] + line2->convertedLen + d;
	if (col2 > col1) {
	  col1 = col2;
	}
      } else if (line1->xMin > line2->xMin) {
	for (i = 0; i < line2->len && line1->xMin >= line2->xRight[i]; ++i) ;
	col2 = line2->col[i];
	if (col2 > col1) {
	  col1 = col2;
	}
      }
    }
    for (j = 0; j < line1->len; ++j) {
      line1->col[j] += col1;
    }
  }

#if 0 // for debugging
  printf("*** lines in xy order, after column assignment ***\n");
  for (line0 = lineList; line0; line0 = line0->next) {
    printf("[line: x=%.2f..%.2f y=%.2f..%.2f base=%.2f col=%d len=%d]\n",
	   line0->xMin, line0->xMax, line0->yMin, line0->yMax,
	   line0->yBase, line0->col[0], line0->len);
    for (word0 = line0->words; word0; word0 = word0->next) {
      printf("  word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f fontSz=%.2f space=%d: '",
	     word0->xMin, word0->xMax, word0->yMin, word0->yMax,
	     word0->yBase, word0->fontSize, word0->spaceAfter);
      for (i = 0; i < word0->len; ++i) {
	fputc(word0->text[i] & 0xff, stdout);
      }
      printf("'\n");
    }
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- assemble lines into blocks

  if (rawOrder) {

    lines = lineList;
    for (line1 = lines; line1; line1 = line1->next) {
      line1->xSpaceL = 0;
      line1->xSpaceR = pageWidth;
    }

  } else {

    // sort lines into yx order
    lines = NULL;
    while (lineList) {
      line0 = lineList;
      lineList = lineList->next;
      for (line1 = NULL, line2 = lines;
	   line2 && !line0->yxBefore(line2);
	   line1 = line2, line2 = line2->next) ;
      if (line1) {
	line1->next = line0;
      } else {
	lines = line0;
      }
      line0->next = line2;
    }

    // compute whitespace to left and right of each line
    line0 = lines;
    for (line1 = lines; line1; line1 = line1->next) {

      // find the first vertically overlapping line
      for (; line0 && line0->yMax < line1->yMin; line0 = line0->next) ;

      // check each vertically overlapping line -- look for the nearest
      // on each side
      line1->xSpaceL = 0;
      line1->xSpaceR = pageWidth;
      for (line2 = line0;
	   line2 && line2->yMin < line1->yMax;
	   line2 = line2->next) {
	if (line2->yMax > line1->yMin) {
	  if (line2->xMax < line1->xMin) {
	    if (line2->xMax > line1->xSpaceL) {
	      line1->xSpaceL = line2->xMax;
	    }
	  } else if (line2->xMin > line1->xMax) {
	    if (line2->xMin < line1->xSpaceR) {
	      line1->xSpaceR = line2->xMin;
	    }
	  }
	}
      }
    }
  } // (!rawOrder)

#if 0 // for debugging
  printf("*** lines in yx order ***\n");
  for (line0 = lines; line0; line0 = line0->next) {
    printf("[line: x=%.2f..%.2f y=%.2f..%.2f base=%.2f xSpaceL=%.2f xSpaceR=%.2f len=%d]\n",
	   line0->xMin, line0->xMax, line0->yMin, line0->yMax,
	   line0->yBase, line0->xSpaceL, line0->xSpaceR, line0->len);
    for (word0 = line0->words; word0; word0 = word0->next) {
      printf("  word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f fontSz=%.2f space=%d: '",
	     word0->xMin, word0->xMax, word0->yMin, word0->yMax,
	     word0->yBase, word0->fontSize, word0->spaceAfter);
      for (i = 0; i < word0->len; ++i) {
	fputc(word0->text[i] & 0xff, stdout);
      }
      printf("'\n");
    }
  }
  printf("\n");
  fflush(stdout);
#endif

  lineList = lines;
  yxBlocks = NULL;
  blk0 = NULL;
  while (lineList) {

    // build a new block object
    line0 = lineList;
    lineList = lineList->next;
    line0->next = NULL;
    blk1 = new TextBlock();
    blk1->lines = line0;
    blk1->xMin = line0->xMin;
    blk1->xMax = line0->xMax;
    blk1->yMin = line0->yMin;
    blk1->yMax = line0->yMax;
    blk1->xSpaceL = line0->xSpaceL;
    blk1->xSpaceR = line0->xSpaceR;
    blk1->maxFontSize = line0->fontSize;

    // find subsequent lines in the block
    while (lineList) {

      // look for the first horizontally overlapping line below this
      // one
      yLimit = line0->yMax + blkMaxSpacing * line0->fontSize;
      line3 = line4 = NULL;
      if (rawOrder) {
	if (lineList->yMin < yLimit &&
	    lineList->xMax > blk1->xMin &&
	    lineList->xMin < blk1->xMax) {
	  line3 = NULL;
	  line4 = lineList;
	}
      } else {
	for (line1 = NULL, line2 = lineList;
	     line2 && line2->yMin < yLimit;
	     line1 = line2, line2 = line2->next) {
	  if (line2->xMax > blk1->xMin &&
	      line2->xMin < blk1->xMax) {
	    line3 = line1;
	    line4 = line2;
	    break;
	  }
	}
      }

      // if there is an overlapping line and it fits in the block, add
      // it to the block
      if (line4 && blockFit(blk1, line4)) {
	if (line3) {
	  line3->next = line4->next;
	} else {
	  lineList = line4->next;
	}
	line0->next = line0->flowNext = line4;
	line4->next = NULL;
	if (line4->xMin < blk1->xMin) {
	  blk1->xMin = line4->xMin;
	} else if (line4->xMax > blk1->xMax) {
	  blk1->xMax = line4->xMax;
	}
	if (line4->yMax > blk1->yMax) {
	  blk1->yMax = line4->yMax;
	}
	if (line4->xSpaceL > blk1->xSpaceL) {
	  blk1->xSpaceL = line4->xSpaceL;
	}
	if (line4->xSpaceR < blk1->xSpaceR) {
	  blk1->xSpaceR = line4->xSpaceR;
	}
	if (line4->fontSize > blk1->maxFontSize) {
	  blk1->maxFontSize = line4->fontSize;
	}
	line0 = line4;

      // otherwise, we're done with this block
      } else {
	break;
      }
    }

    // insert block on list, in yx order
    if (rawOrder) {
      blk2 = blk0;
      blk3 = NULL;
      blk0 = blk1;
    } else {
      for (blk2 = NULL, blk3 = yxBlocks;
	   blk3 && !blk1->yxBefore(blk3);
	   blk2 = blk3, blk3 = blk3->next) ;
    }
    blk1->next = blk3;
    if (blk2) {
      blk2->next = blk1;
    } else {
      yxBlocks = blk1;
    }
  }

#if 0 // for debugging
  printf("*** blocks in yx order ***\n");
  for (blk0 = yxBlocks; blk0; blk0 = blk0->next) {
    printf("[block: x=%.2f..%.2f y=%.2f..%.2f]\n",
	   blk0->xMin, blk0->xMax, blk0->yMin, blk0->yMax);
    for (line0 = blk0->lines; line0; line0 = line0->next) {
      printf("  [line: x=%.2f..%.2f y=%.2f..%.2f base=%.2f len=%d]\n",
	     line0->xMin, line0->xMax, line0->yMin, line0->yMax,
	     line0->yBase, line0->len);
      for (word0 = line0->words; word0; word0 = word0->next) {
	printf("    word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f space=%d: '",
	       word0->xMin, word0->xMax, word0->yMin, word0->yMax,
	       word0->yBase, word0->spaceAfter);
	for (i = 0; i < word0->len; ++i) {
	  fputc(word0->text[i] & 0xff, stdout);
	}
	printf("'\n");
      }
    }
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- merge lines and blocks, sort blocks into reading order

  if (rawOrder) {
    blocks = yxBlocks;

  } else {
    blocks = NULL;
    blk0 = NULL;
    blkStack = NULL;
    while (yxBlocks) {

      // find the next two blocks:
      // - if the depth-first traversal stack is empty, take the first
      //   (upper-left-most) two blocks on the yx-sorted block list
      // - otherwise, find the two upper-left-most blocks under the top
      //   block on the stack
      if (blkStack) {
	blk3 = blk4 = blk5 = blk6 = NULL;
	for (blk1 = NULL, blk2 = yxBlocks;
	     blk2;
	     blk1 = blk2, blk2 = blk2->next) {
	  if (blk2->yMin > blkStack->yMin &&
	      blk2->xMax > blkStack->xMin &&
	      blk2->xMin < blkStack->xMax) {
	    if (!blk4 || blk2->yxBefore(blk4)) {
	      blk5 = blk3;
	      blk6 = blk4;
	      blk3 = blk1;
	      blk4 = blk2;
	    } else if (!blk6 || blk2->yxBefore(blk6)) {
	      blk5 = blk1;
	      blk6 = blk2;
	    }
	  }
	}
      } else {
	blk3 = NULL;
	blk4 = yxBlocks;
	blk5 = yxBlocks;
	blk6 = yxBlocks->next;
      }

      // merge case 1:
      //   |                     |           |
      //   |   blkStack          |           |   blkStack
      //   +---------------------+    -->    +--------------
      //   +------+ +------+                 +-----------+
      //   | blk4 | | blk6 | ...             | blk4+blk6 |
      //   +------+ +------+                 +-----------+
      yLimit = 0; // make gcc happy
      if (blkStack) {
	yLimit = blkStack->yMax + blkMaxSpacing * blkStack->lines->fontSize;
      }
      if (blkStack && blk4 && blk6 &&
	  !blk4->lines->next && !blk6->lines->next &&
	  lineFit2(blk4->lines, blk6->lines) &&
	  blk4->yMin < yLimit &&
	  blk4->xMin > blkStack->xSpaceL &&
	  blkStack->xMin > blk4->xSpaceL &&
	  blk6->xMax < blkStack->xSpaceR) {
	blk4->mergeRight(blk6);
	if (blk5) {
	  blk5->next = blk6->next;
	} else {
	  yxBlocks = blk6->next;
	}
	delete blk6;

      // merge case 2:
      //   |                     |           |                   |
      //   |   blkStack          |           |                   |
      //   +---------------------+    -->    |   blkStack+blk2   |
      //   +---------------------+           |                   |
      //   |   blk4              |           |                   |
      //   |                     |           |                   |
      } else if (blkStack && blk4 &&
		 blk4->yMin < yLimit &&
		 blockFit2(blkStack, blk4)) {
	blkStack->mergeBelow(blk4);
	if (blk3) {
	  blk3->next = blk4->next;
	} else {
	  yxBlocks = blk4->next;
	}
	delete blk4;

      // if any of:
      // 1. no block found
      // 2. non-fully overlapping block found
      // 3. large vertical gap above the overlapping block
      // then pop the stack and try again
      } else if (!blk4 ||
		 (blkStack && (blk4->xMin < blkStack->xSpaceL ||
			       blk4->xMax > blkStack->xSpaceR ||
			       blk4->yMin - blkStack->yMax >
			         blkMaxSortSpacing * blkStack->maxFontSize))) {
	blkStack = blkStack->stackNext;

      // add a block to the sorted list
      } else {

	// remove the block from the yx-sorted list
	if (blk3) {
	  blk3->next = blk4->next;
	} else {
	  yxBlocks = blk4->next;
	}
	blk4->next = NULL;

	// append the block to the reading-order list
	if (blk0) {
	  blk0->next = blk4;
	} else {
	  blocks = blk4;
	}
	blk0 = blk4;

	// push the block on the traversal stack
	if (!physLayout) {
	  blk4->stackNext = blkStack;
	  blkStack = blk4;
	}
      }
    }
  } // (!rawOrder)

#if 0 // for debugging
  printf("*** blocks in reading order (after merging) ***\n");
  for (blk0 = blocks; blk0; blk0 = blk0->next) {
    printf("[block: x=%.2f..%.2f y=%.2f..%.2f]\n",
	   blk0->xMin, blk0->xMax, blk0->yMin, blk0->yMax);
    for (line0 = blk0->lines; line0; line0 = line0->next) {
      printf("  [line: x=%.2f..%.2f y=%.2f..%.2f base=%.2f len=%d]\n",
	     line0->xMin, line0->xMax, line0->yMin, line0->yMax,
	     line0->yBase, line0->len);
      for (word0 = line0->words; word0; word0 = word0->next) {
	printf("    word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f space=%d: '",
	       word0->xMin, word0->xMax, word0->yMin, word0->yMax,
	       word0->yBase, word0->spaceAfter);
	for (i = 0; i < word0->len; ++i) {
	  fputc(word0->text[i] & 0xff, stdout);
	}
	printf("'\n");
      }
    }
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- assemble blocks into flows

  if (rawOrder) {

    // one flow per block
    flow0 = NULL;
    while (blocks) {
      flow1 = new TextFlow();
      flow1->blocks = blocks;
      flow1->lines = blocks->lines;
      flow1->yMin = blocks->yMin;
      flow1->yMax = blocks->yMax;
      blocks = blocks->next;
      flow1->blocks->next = NULL;
      if (flow0) {
	flow0->next = flow1;
      } else {
	flows = flow1;
      }
      flow0 = flow1;
    }

  } else {

    // compute whitespace above and below each block
    for (blk0 = blocks; blk0; blk0 = blk0->next) {
      blk0->ySpaceT = 0;
      blk0->ySpaceB = pageHeight;

      // check each horizontally overlapping block
      for (blk1 = blocks; blk1; blk1 = blk1->next) {
	if (blk1 != blk0 &&
	    blk1->xMin < blk0->xMax &&
	    blk1->xMax > blk0->xMin) {
	  if (blk1->yMax < blk0->yMin) {
	    if (blk1->yMax > blk0->ySpaceT) {
	      blk0->ySpaceT = blk1->yMax;
	    }
	  } else if (blk1->yMin > blk0->yMax) {
	    if (blk1->yMin < blk0->ySpaceB) {
	      blk0->ySpaceB = blk1->yMin;
	    }
	  }
	}
      }
    }

    flow0 = NULL;
    while (blocks) {

      // build a new flow object
      flow1 = new TextFlow();
      flow1->blocks = blocks;
      flow1->lines = blocks->lines;
      flow1->yMin = blocks->yMin;
      flow1->yMax = blocks->yMax;
      flow1->ySpaceT = blocks->ySpaceT;
      flow1->ySpaceB = blocks->ySpaceB;

      // find subsequent blocks in the flow
      for (blk1 = blocks, blk2 = blocks->next;
	   blk2 && flowFit(flow1, blk2);
	   blk1 = blk2, blk2 = blk2->next) {
	if (blk2->yMin < flow1->yMin) {
	  flow1->yMin = blk2->yMin;
	}
	if (blk2->yMax > flow1->yMax) {
	  flow1->yMax = blk2->yMax;
	}
	if (blk2->ySpaceT > flow1->ySpaceT) {
	  flow1->ySpaceT = blk2->ySpaceT;
	}
	if (blk2->ySpaceB < flow1->ySpaceB) {
	  flow1->ySpaceB = blk2->ySpaceB;
	}
	for (line1 = blk1->lines; line1->next; line1 = line1->next) ;
	line1->flowNext = blk2->lines;
      }

      // chop the block list
      blocks = blk1->next;
      blk1->next = NULL;

      // append the flow to the list
      if (flow0) {
	flow0->next = flow1;
      } else {
	flows = flow1;
      }
      flow0 = flow1;
    }
  }

#if 0 // for debugging
  printf("*** flows ***\n");
  for (flow0 = flows; flow0; flow0 = flow0->next) {
    printf("[flow]\n");
    for (blk0 = flow0->blocks; blk0; blk0 = blk0->next) {
      printf("  [block: x=%.2f..%.2f y=%.2f..%.2f ySpaceT=%.2f ySpaceB=%.2f]\n",
	     blk0->xMin, blk0->xMax, blk0->yMin, blk0->yMax,
	     blk0->ySpaceT, blk0->ySpaceB);
      for (line0 = blk0->lines; line0; line0 = line0->next) {
	printf("    [line: x=%.2f..%.2f y=%.2f..%.2f base=%.2f len=%d]\n",
	       line0->xMin, line0->xMax, line0->yMin, line0->yMax,
	       line0->yBase, line0->len);
	for (word0 = line0->words; word0; word0 = word0->next) {
	  printf("      word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f space=%d: '",
		 word0->xMin, word0->xMax, word0->yMin, word0->yMax,
		 word0->yBase, word0->spaceAfter);
	  for (i = 0; i < word0->len; ++i) {
	    fputc(word0->text[i] & 0xff, stdout);
	  }
	  printf("'\n");
	}
      }
    }
  }
  printf("\n");
  fflush(stdout);
#endif

  //----- sort lines into yx order

  // (the block/line merging process doesn't maintain the full-page
  // linked list of lines)

  lines = NULL;
  if (rawOrder) {
    line0 = NULL;
    for (flow0 = flows; flow0; flow0 = flow0->next) {
      for (line1 = flow0->lines; line1; line1 = line1->flowNext) {
	if (line0) {
	  line0->pageNext = line1;
	} else {
	  lines = line1;
	}
	line0 = line1;
      }
    }
  } else {
    for (flow0 = flows; flow0; flow0 = flow0->next) {
      for (line0 = flow0->lines; line0; line0 = line0->flowNext) {
	for (line1 = NULL, line2 = lines;
	     line2 && !line0->yxBefore(line2);
	     line1 = line2, line2 = line2->pageNext) ;
	if (line1) {
	  line1->pageNext = line0;
	} else {
	  lines = line0;
	}
	line0->pageNext = line2;
      }
    }
  }

#if 0 // for debugging
  printf("*** lines in yx order ***\n");
  for (line0 = lines; line0; line0 = line0->pageNext) {
    printf("[line: x=%.2f..%.2f y=%.2f..%.2f base=%.2f xSpaceL=%.2f xSpaceR=%.2f col=%d len=%d]\n",
	   line0->xMin, line0->xMax, line0->yMin, line0->yMax,
	   line0->yBase, line0->xSpaceL, line0->xSpaceR, line0->col[0],
	   line0->len);
    for (word0 = line0->words; word0; word0 = word0->next) {
      printf("  word: x=%.2f..%.2f y=%.2f..%.2f base=%.2f space=%d: '",
	     word0->xMin, word0->xMax, word0->yMin, word0->yMax,
	     word0->yBase, word0->spaceAfter);
      for (i = 0; i < word0->len; ++i) {
	fputc(word0->text[i] & 0xff, stdout);
      }
      printf("'\n");
    }
  }
  printf("\n");
  fflush(stdout);
#endif
}

// If <word> can be added the end of <line>, return the absolute value
// of the difference between <line>'s baseline and <word>'s baseline,
// and set *<space> to the horizontal space between the current last
// word in <line> and <word>.  A smaller return value indicates a
// better fit.  Otherwise, return a negative number.
double TextPage::lineFit(TextLine *line, TextWord *word, double *space) {
  TextWord *lastWord;
  double fontSize0, fontSize1;
  double dx, dxLimit;

  lastWord = line->lastWord;
  fontSize0 = line->fontSize;
  fontSize1 = word->fontSize;
  dx = word->xMin - lastWord->xMax;
  dxLimit = fontSize0 * lastWord->font->maxSpaceWidth;

  // check inter-word spacing
  if (dx < fontSize0 * lineMinDeltaX ||
      dx > dxLimit) {
    return -1;
  }

  if (
      // look for adjacent words with close baselines and close font sizes
      (fabs(line->yBase - word->yBase) < lineMaxBaselineDelta * fontSize0 &&
       fontSize0 < lineMaxFontSizeRatio * fontSize1 &&
       fontSize1 < lineMaxFontSizeRatio * fontSize0) ||

      // look for a superscript
      (fontSize1 > lineMinSuperscriptFontSizeRatio * fontSize0 &&
       fontSize1 < lineMaxSuperscriptFontSizeRatio * fontSize0 &&
       (word->yMax < lastWord->yMax ||
	word->yBase < lastWord->yBase) &&
       word->yMax - lastWord->yMin > lineMinSuperscriptOverlap * fontSize0 &&
       dx < fontSize0 * lineMaxSuperscriptDeltaX) ||

      // look for a subscript
      (fontSize1 > lineMinSubscriptFontSizeRatio * fontSize0 &&
       fontSize1 < lineMaxSubscriptFontSizeRatio * fontSize0 &&
       (word->yMin > lastWord->yMin ||
	word->yBase > lastWord->yBase) &&
       line->yMax - word->yMin > lineMinSubscriptOverlap * fontSize0 &&
       dx < fontSize0 * lineMaxSubscriptDeltaX)) {

    *space = dx;
    return fabs(word->yBase - line->yBase);
  }

  return -1;
}

// Returns true if <line0> and <line1> can be merged into a single
// line, ignoring max word spacing.
GBool TextPage::lineFit2(TextLine *line0, TextLine *line1) {
  double fontSize0, fontSize1;
  double dx;

  fontSize0 = line0->fontSize;
  fontSize1 = line1->fontSize;
  dx = line1->xMin - line0->xMax;

  // check inter-word spacing
  if (dx < fontSize0 * lineMinDeltaX) {
    return gFalse;
  }

  // look for close baselines and close font sizes
  if (fabs(line0->yBase - line1->yBase) < lineMaxBaselineDelta * fontSize0 &&
      fontSize0 < lineMaxFontSizeRatio * fontSize1 &&
      fontSize1 < lineMaxFontSizeRatio * fontSize0) {
    return gTrue;
  }

  return gFalse;
}

// Returns true if <line> can be added to <blk>.  Assumes the y
// coordinates are within range.
GBool TextPage::blockFit(TextBlock *blk, TextLine *line) {
  double fontSize0, fontSize1;

  // check edges
  if (line->xMin < blk->xSpaceL ||
      line->xMax > blk->xSpaceR ||
      blk->xMin < line->xSpaceL ||
      blk->xMax > line->xSpaceR) {
    return gFalse;
  }

  // check font sizes
  fontSize0 = blk->lines->fontSize;
  fontSize1 = line->fontSize;
  if (fontSize0 > blkMaxFontSizeRatio * fontSize1 ||
      fontSize1 > blkMaxFontSizeRatio * fontSize0) {
    return gFalse;
  }

  return gTrue;
}

// Returns true if <blk0> and <blk1> can be merged into a single
// block.  Assumes the y coordinates are within range.
GBool TextPage::blockFit2(TextBlock *blk0, TextBlock *blk1) {
  double fontSize0, fontSize1;

  // check edges
  if (blk1->xMin < blk0->xSpaceL ||
      blk1->xMax > blk0->xSpaceR ||
      blk0->xMin < blk1->xSpaceL ||
      blk0->xMax > blk1->xSpaceR) {
    return gFalse;
  }

  // check font sizes
  fontSize0 = blk0->lines->fontSize;
  fontSize1 = blk1->lines->fontSize;
  if (fontSize0 > blkMaxFontSizeRatio * fontSize1 ||
      fontSize1 > blkMaxFontSizeRatio * fontSize0) {
    return gFalse;
  }

  return gTrue;
}

// Returns true if <blk> can be added to <flow>.
GBool TextPage::flowFit(TextFlow *flow, TextBlock *blk) {
  double dy;

  // check whitespace above and below
  if (blk->yMin < flow->ySpaceT ||
      blk->yMax > flow->ySpaceB ||
      flow->yMin < blk->ySpaceT ||
      flow->yMax > blk->ySpaceB) {
    return gFalse;
  }

  // check that block top edge is within +/- dy of flow top edge,
  // and that block bottom edge is above flow bottom edge + dy
  dy = flowMaxDeltaY * flow->blocks->maxFontSize;
  return blk->yMin > flow->yMin - dy &&
         blk->yMin < flow->yMin + dy &&
         blk->yMax < flow->yMax + dy;
}


GBool TextPage::findText(Unicode *s, int len,
			 GBool top, GBool bottom,
			 double *xMin, double *yMin,
			 double *xMax, double *yMax) {
  TextLine *line;
  Unicode *p;
  Unicode u1, u2;
  int m, i, j;
  double x0, x1, x;

  // scan all text on the page
  for (line = lines; line; line = line->pageNext) {

    // check: above top limit?
    if (!top && (line->yMax < *yMin ||
		 (line->yMin < *yMin && line->xMax <= *xMin))) {
      continue;
    }

    // check: below bottom limit?
    if (!bottom && (line->yMin > *yMax ||
		    (line->yMax > *yMax && line->xMin >= *xMax))) {
      return gFalse;
    }

    // search each position in this line
    m = line->len;
    for (i = 0, p = line->text; i <= m - len; ++i, ++p) {

      x0 = (i == 0) ? line->xMin : line->xRight[i-1];
      x1 = line->xRight[i];
      x = 0.5 * (x0 + x1);

      // check: above top limit?
      if (!top && line->yMin < *yMin) {
	if (x < *xMin) {
	  continue;
	}
      }

      // check: below bottom limit?
      if (!bottom && line->yMax > *yMax) {
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
	*xMax = line->xRight[i + len - 1];
	*yMin = line->yMin;
	*yMax = line->yMax;
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
  GBool isUnicode;
  char space[8], eol[16], buf[8];
  int spaceLen, eolLen, len;
  TextLine *line, *prevLine;
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
  firstCol = -1;
  for (line = lines; line; line = line->pageNext) {
    if (line->yMin > yMax) {
      break;
    }
    if (line->yMax < yMin || 
	line->xMax < xMin ||
	line->xMin > xMax) {
      continue;
    }

    y = 0.5 * (line->yMin + line->yMax);
    if (y < yMin || y > yMax) {
      continue;
    }

    i = 0;
    while (i < line->len) {
      x0 = (i==0) ? line->xMin : line->xRight[i-1];
      x1 = line->xRight[i];
      if (0.5 * (x0 + x1) > xMin) {
	break;
      }
      ++i;
    }
    if (i == line->len) {
      continue;
    }
    col = line->col[i];

    if (firstCol < 0 || col < firstCol) {
      firstCol = col;
    }
  }

  // extract the text
  col = firstCol;
  multiLine = gFalse;
  prevLine = NULL;
  for (line = lines; line; line = line->pageNext) {
    if (line->yMin > yMax) {
      break;
    }
    if (line->yMax < yMin || 
	line->xMax < xMin ||
	line->xMin > xMax) {
      continue;
    }

    y = 0.5 * (line->yMin + line->yMax);
    if (y < yMin || y > yMax) {
      continue;
    }

    i = 0;
    while (i < line->len) {
      x0 = (i==0) ? line->xMin : line->xRight[i-1];
      x1 = line->xRight[i];
      if (0.5 * (x0 + x1) > xMin) {
	break;
      }
      ++i;
    }
    if (i == line->len) {
      continue;
    }

    // insert a return
    if (line->col[i] < col ||
	(prevLine &&
	 line->yMin >
	   prevLine->yMax - lineOverlapSlack * prevLine->fontSize)) {
      s->append(eol, eolLen);
      col = firstCol;
      multiLine = gTrue;
    }
    prevLine = line;

    // line this block up with the correct column
    for (; col < line->col[i]; ++col) {
      s->append(space, spaceLen);
    }

    // print the portion of the line
    for (; i < line->len; ++i) {

      x0 = (i==0) ? line->xMin : line->xRight[i-1];
      x1 = line->xRight[i];
      if (0.5 * (x0 + x1) > xMax) {
	break;
      }

      len = uMap->mapUnicode(line->text[i], buf, sizeof(buf));
      s->append(buf, len);
      col += isUnicode ? 1 : len;
    }
  }

  if (multiLine) {
    s->append(eol, eolLen);
  }

  uMap->decRefCnt();

  return s;
}

GBool TextPage::findCharRange(int pos, int length,
			      double *xMin, double *yMin,
			      double *xMax, double *yMax) {
  TextLine *line;
  TextWord *word;
  double x;
  GBool first;
  int i;

  //~ this doesn't correctly handle:
  //~ - ranges split across multiple lines (the highlighted region
  //~   is the bounding box of all the parts of the range)
  //~ - cases where characters don't convert one-to-one into Unicode
  first = gTrue;
  for (line = lines; line; line = line->pageNext) {
    for (word = line->words; word; word = word->next) {
      if (pos < word->charPos + word->charLen &&
	  word->charPos < pos + length) {
	i = pos - word->charPos;
	if (i < 0) {
	  i = 0;
	}
	x = (i == 0) ? word->xMin : word->xRight[i - 1];
	if (first || x < *xMin) {
	  *xMin = x;
	}
	i = pos + length - word->charPos;
	if (i >= word->len) {
	  i = word->len - 1;
	}
	x = word->xRight[i];
	if (first || x > *xMax) {
	  *xMax = x;
	}
	if (first || word->yMin < *yMin) {
	  *yMin = word->yMin;
	}
	if (first || word->yMax > *yMax) {
	  *yMax = word->yMax;
	}
	first = gFalse;
      }
    }
  }
  return !first;
}

void TextPage::dump(void *outputStream, TextOutputFunc outputFunc,
		    GBool physLayout) {
  UnicodeMap *uMap;
  char space[8], eol[16], eop[8], buf[8];
  int spaceLen, eolLen, eopLen, len;
  TextFlow *flow;
  TextLine *line;
  int col, d, n, i;

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

  // output the page, maintaining the original physical layout
  if (physLayout || rawOrder) {
    col = 0;
    for (line = lines; line; line = line->pageNext) {

      // line this block up with the correct column
      if (!rawOrder) {
	for (; col < line->col[0]; ++col) {
	  (*outputFunc)(outputStream, space, spaceLen);
	}
      }

      // print the line
      for (i = 0; i < line->len; ++i) {
	len = uMap->mapUnicode(line->text[i], buf, sizeof(buf));
	(*outputFunc)(outputStream, buf, len);
      }
      col += line->convertedLen;

      // print one or more returns if necessary
      if (rawOrder ||
	  !line->pageNext ||
	  line->pageNext->col[0] < col ||
	  line->pageNext->yMin >
	    line->yMax - lineOverlapSlack * line->fontSize) {

	// compute number of returns
	d = 1;
	if (line->pageNext) {
	  d += (int)((line->pageNext->yMin - line->yMax) /
		     line->fontSize + 0.5);
	}

	// various things (weird font matrices) can result in bogus
	// values here, so do a sanity check
	if (d < 1) {
	  d = 1;
	} else if (d > 5) {
	  d = 5;
	}
	for (; d > 0; --d) {
	  (*outputFunc)(outputStream, eol, eolLen);
	}

	col = 0;
      }
    }

  // output the page, "undoing" the layout
  } else {
    for (flow = flows; flow; flow = flow->next) {
      for (line = flow->lines; line; line = line->flowNext) {
	n = line->len;
	if (line->flowNext && line->hyphenated) {
	  --n;
	}
	for (i = 0; i < n; ++i) {
	  len = uMap->mapUnicode(line->text[i], buf, sizeof(buf));
	  (*outputFunc)(outputStream, buf, len);
	}
	if (line->flowNext && !line->hyphenated) {
	  (*outputFunc)(outputStream, space, spaceLen);
	}
      }
      (*outputFunc)(outputStream, eol, eolLen);
      (*outputFunc)(outputStream, eol, eolLen);
    }
  }

  // end of page
  (*outputFunc)(outputStream, eop, eopLen);
  (*outputFunc)(outputStream, eol, eolLen);

  uMap->decRefCnt();
}

void TextPage::startPage(GfxState *state) {
  clear();
  if (state) {
    pageWidth = state->getPageWidth();
    pageHeight = state->getPageHeight();
  } else {
    pageWidth = pageHeight = 0;
  }
}

void TextPage::clear() {
  TextWord *w1, *w2;
  TextFlow *f1, *f2;

  if (curWord) {
    delete curWord;
    curWord = NULL;
  }
  if (words) {
    for (w1 = words; w1; w1 = w2) {
      w2 = w1->next;
      delete w1;
    }
  } else if (flows) {
    for (f1 = flows; f1; f1 = f2) {
      f2 = f1->next;
      delete f1;
    }
  }
  deleteGList(fonts, TextFontInfo);

  curWord = NULL;
  charPos = 0;
  font = NULL;
  fontSize = 0;
  nest = 0;
  nTinyChars = 0;
  words = wordPtr = NULL;
  lines = NULL;
  flows = NULL;
  fonts = new GList();

}


//------------------------------------------------------------------------
// TextOutputDev
//------------------------------------------------------------------------

static void outputToFile(void *stream, char *text, int len) {
  fwrite(text, 1, len, (FILE *)stream);
}

TextOutputDev::TextOutputDev(char *fileName, GBool physLayoutA,
			     GBool rawOrderA, GBool append) {
  text = NULL;
  physLayout = physLayoutA;
  rawOrder = rawOrderA;
  ok = gTrue;

  // open file
  needClose = gFalse;
  if (fileName) {
    if (!strcmp(fileName, "-")) {
      outputStream = stdout;
#ifdef WIN32
      // keep DOS from munging the end-of-line characters
      setmode(fileno(stdout), O_BINARY);
#endif
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
  text = new TextPage(rawOrderA);
}

TextOutputDev::TextOutputDev(TextOutputFunc func, void *stream,
			     GBool physLayoutA, GBool rawOrderA) {
  outputFunc = func;
  outputStream = stream;
  needClose = gFalse;
  physLayout = physLayoutA;
  rawOrder = rawOrderA;
  text = new TextPage(rawOrderA);
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
  text->startPage(state);
}

void TextOutputDev::endPage() {
  text->coalesce(physLayout);
  if (outputStream) {
    text->dump(outputStream, outputFunc, physLayout);
  }
}

void TextOutputDev::updateFont(GfxState *state) {
  text->updateFont(state);
}

void TextOutputDev::beginString(GfxState *state, GString *s) {
  text->beginWord(state, state->getCurX(), state->getCurY());
}

void TextOutputDev::endString(GfxState *state) {
  text->endWord();
}

void TextOutputDev::drawChar(GfxState *state, double x, double y,
			     double dx, double dy,
			     double originX, double originY,
			     CharCode c, Unicode *u, int uLen) {
  text->addChar(state, x, y, dx, dy, c, u, uLen);
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

GBool TextOutputDev::findCharRange(int pos, int length,
				   double *xMin, double *yMin,
				   double *xMax, double *yMax) {
  return text->findCharRange(pos, length, xMin, yMin, xMax, yMax);
}


