/* kpsestat -- show file permissions of a file in octal form.
   Copyright (C) 1997 Olaf Weber.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <kpathsea/config.h>
#include <kpathsea/c-stat.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <string.h>
#endif

/*
 *      kpsestat mode x
 *      Print stat bits of file x on stdout, as modified by mode.
 */

int main (argc, argv)
     int argc;
     char *argv[];
{
    char * mode_string;
    int to_set, to_keep, to_clear;
    int result;
    struct stat f;

    if (argc > 1 && strcmp (argv[1], "--help") == 0) {
        printf ("Usage: %s MODE FILE\n\
  Print octal permissions of FILE as modified by MODE on standard output.\n\
  MODE is a subset of the symbolic permissions accepted by chmod.\n\
  Use MODE = to obtain the unchanged permissions.\n\
\n\
--help      display this help and exit\n\
--version   output version information and exit\n\n", argv[0]);
        fputs ("Email bug reports to tex-k@mail.tug.org.\n", stdout);
        exit(0);
    } else if (argc > 1 && strcmp (argv[1], "--version") == 0) {
        printf ("%s (%s)\n\
Copyright (C) 1997 Olaf Weber.\n\
There is NO warranty.  You may redistribute this software\n\
under the terms of the GNU General Public License.\n\
For more information about these matters, see the file named COPYING.\n\
Primary author of %s: Olaf Weber.\n",
argv[0], KPSEVERSION, argv[0]);
        exit (0);
    }

    /* insist on exactly two args */
    if (argc != 3) {
        fprintf (stderr, "%s: Need exactly two arguments.\n\
Try `%s --help' for more information.\n", argv[0], argv[0]);
        exit(1);
    }

    mode_string = argv[1];
    to_set = to_keep = to_clear = 0;
    for (;;++mode_string) {
        int affected = 0;
        int action = 0;
        int value = 0;

        for (;;++mode_string)
            switch (*mode_string) {
            case 'u': affected |= 04700; break;
            case 'g': affected |= 02070; break;
            case 'o': affected |= 01007; break;
            case 'a': affected |= 07777; break;
            default: goto no_more_affected;
            }
    no_more_affected:
        if (affected == 0)
            affected = 07777;
        action = *mode_string;
        ++mode_string;
        for (;;++mode_string)
            switch (*mode_string) {
            case 'r': value |= 00444 & affected; break;
            case 'w': value |= 00222 & affected; break;
            case 'x': value |= 00111 & affected; break;
            case 's': value |= 06000 & affected; break;
            case 't': value |= 01000 & affected; break;
            default: goto no_more_values;
            }
    no_more_values:
        switch (action) {
        case '-': to_clear |= value; break;
        case '=': to_keep  |= value; break;
        case '+': to_set   |= value; break;
        default:
            fprintf(stderr, "%s: Invalid mode\n", argv[0]);
            exit(1);
        }
        if (*mode_string != ',')
            break;
    }
    if (*mode_string != 0) {
        fprintf(stderr, "%s: Invalid mode.\n", argv[0]);
        exit(1);
    }

    /* does the file exist? */
    if (stat (argv[2], &f) < 0) {
        fprintf(stderr, "%s: ", argv[0]);
        perror(argv[2]);
        return 1;
    }
   
    result = f.st_mode & 07777;
    result |= to_set;
    result |= to_keep & result;
    result &= ~to_clear;

    printf("%o\n", result);

    return 0;
}
