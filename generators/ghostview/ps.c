/*
 * ps.c -- Postscript scanning and copying routines.
 * Copyright (C) 1992  Timothy O. Theisen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *   Author: Tim Theisen           Systems Programmer
 * Internet: tim@cs.wisc.edu       Department of Computer Sciences
 *     UUCP: uwvax!tim             University of Wisconsin-Madison
 *    Phone: (608)262-0438         1210 West Dayton Street
 *      FAX: (608)262-9777         Madison, WI   53706
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ps.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

/* length calculates string length at compile time */
/* can only be used with character constants */
#define length(a) (sizeof(a)-1)
#define iscomment(a, b)	(strncmp(a, b, length(b)) == 0)
#define DSCcomment(a) (a[0] == '%' && a[1] == '%')

/*
 *	pscopy -- copy lines of Postscript from a section of one file
 *		  to another file.
 *                Automatically switch to binary copying whenever
 *                %%BeginBinary/%%EndBinary or %%BeginData/%%EndData
 *		  comments are encountered.
 */

void
pscopy(from, to, begin, end)
    FILE *from;
    FILE *to;
    long begin;			/* set negative to avoid initial seek */
    long end;
{
    char line[PSLINELENGTH];	/* 255 characters + 1 newline + 1 NULL */
    char text[PSLINELENGTH];	/* Temporary storage for text */
    unsigned int num;
    unsigned int i;
    char buf[BUFSIZ];

    if (begin >= 0) fseek(from, begin, SEEK_SET);
    while (ftell(from) < end) {

	fgets(line, sizeof line, from);
	fputs(line, to);

	if (!(DSCcomment(line) && iscomment(line+2, "Begin"))) {
	    /* Do nothing */
	} else if (iscomment(line+7, "Data:")) {
            int rc = 0;
	    text[0] = '\0';
	    rc = sscanf(line+length("%%BeginData:"), "%d %*s %256s", &num,text);
            text[256] = '\0';
            if (rc >= 1) {
		if (strcmp(text, "Lines") == 0) {
		    for (i=0; i < num; i++) {
			fgets(line, sizeof line, from);
			fputs(line, to);
		    }
		} else {
		    while (num > BUFSIZ) {
			fread(buf, sizeof (char), BUFSIZ, from);
			fwrite(buf, sizeof (char), BUFSIZ, to);
			num -= BUFSIZ;
		    }
		    fread(buf, sizeof (char), num, from);
		    fwrite(buf, sizeof (char), num, to);
		}
	    }
	} else if (iscomment(line+7, "Binary:")) {
	    if(sscanf(line+length("%%BeginBinary:"), "%d", &num) == 1) {
		while (num > BUFSIZ) {
		    fread(buf, sizeof (char), BUFSIZ, from);
		    fwrite(buf, sizeof (char), BUFSIZ, to);
		    num -= BUFSIZ;
		}
		fread(buf, sizeof (char), num, from);
		fwrite(buf, sizeof (char), num, to);
	    }
	}
    }
}

/*
 *	pscopyuntil -- copy lines of Postscript from a section of one file
 *		       to another file until a particular comment is reached.
 *                     Automatically switch to binary copying whenever
 *                     %%BeginBinary/%%EndBinary or %%BeginData/%%EndData
 *		       comments are encountered.
 */

char *
pscopyuntil(from, to, begin, end, comment)
    FILE *from;
    FILE *to;
    long begin;			/* set negative to avoid initial seek */
    long end;
    const char *comment;
{
    char line[PSLINELENGTH];	/* 255 characters + 1 newline + 1 NULL */
    char text[PSLINELENGTH];	/* Temporary storage for text */
    unsigned int num;
    unsigned int i;
    int comment_length;
    char buf[BUFSIZ];
    char *cp;

    comment_length = strlen(comment);
    if (begin >= 0) fseek(from, begin, SEEK_SET);
    while (ftell(from) < end) {

	fgets(line, sizeof line, from);

	/* iscomment cannot be used here,
	 * because comment_length is not known at compile time. */
	if (strncmp(line, comment, comment_length) == 0) {
	    cp = (char *) malloc(strlen(line)+1);
	    if (cp == NULL) {
		fprintf(stderr, "Fatal Error: Dynamic memory exhausted.\n");
		exit(-1);
	    }
	    strcpy(cp, line);
	    return cp;
	}
	fputs(line, to);
	if (!(DSCcomment(line) && iscomment(line+2, "Begin"))) {
	    /* Do nothing */
	} else if (iscomment(line+7, "Data:")) {
            int rc = 0;
	    text[0] = '\0';
	    rc = sscanf(line+length("%%BeginData:"), "%d %*s %256s", &num,text);
            text[256] = '\0';
            if (rc >= 1) {
		if (strcmp(text, "Lines") == 0) {
		    for (i=0; i < num; i++) {
			fgets(line, sizeof line, from);
			fputs(line, to);
		    }
		} else {
		    while (num > BUFSIZ) {
			fread(buf, sizeof (char), BUFSIZ, from);
			fwrite(buf, sizeof (char), BUFSIZ, to);
			num -= BUFSIZ;
		    }
		    fread(buf, sizeof (char), num, from);
		    fwrite(buf, sizeof (char), num, to);
		}
	    }
	} else if (iscomment(line+7, "Binary:")) {
	    if(sscanf(line+length("%%BeginBinary:"), "%d", &num) == 1) {
		while (num > BUFSIZ) {
		    fread(buf, sizeof (char), BUFSIZ, from);
		    fwrite(buf, sizeof (char), BUFSIZ, to);
		    num -= BUFSIZ;
		}
		fread(buf, sizeof (char), num, from);
		fwrite(buf, sizeof (char), num, to);
	    }
	}
    }
    return NULL;
}

