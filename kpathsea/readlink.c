/* readlink -- obtain contents of symlink.
   Copyright (C) 1998 Olaf Weber.

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

#include <stdio.h>
#include <kpathsea/config.h>
#include <kpathsea/c-pathmx.h>
#include <kpathsea/c-unistd.h>
#include <kpathsea/c-stat.h>

#ifdef WIN32
#include <string.h>
#endif

/*
 *      readlink name
 *      Returns 0 if name exists and is a symlink, 1 otherwise.  The contents
 *      of the link are printed on standard output.
 */

int main (argc, argv)
    int argc;
    char *argv[];
{
#ifdef S_ISLNK
    int status;
    char path[PATH_MAX];
#endif
    
    if (argc > 1 && strcmp (argv[1], "--help") == 0) {
        printf("Usage: %s FILE\n\
  If FILE exists and is a symlink, print the contents of the link and\n\
  exit successfully.  Otherwise print nothing and fail.\n\
\n\
--help      display this help and exit\n\
--version   output version information and exit\n\n", argv[0]);
        fputs ("Email bug reports to tex-k@mail.tug.org.\n", stdout);
        exit(0);
    } else if (argc > 1 && strcmp (argv[1], "--version") == 0) {
        printf ("%s (%s)\n\
Copyright (C) 1998 Olaf Weber.\n\
There is NO warranty.  You may redistribute this software\n\
under the terms of the GNU General Public License\n\
For more information about these matters, see the file named COPYING.\n\
Primary author of %s: Olaf Weber.\n",
argv[0], KPSEVERSION, argv[0]);
        exit (0);
    }

    /* insist on exactly one arg */
    if (argc != 2) {
        fprintf(stderr, "%s: Need exactly one argument.\n\
Try `%s --help' for more information.\n", argv[0], argv[0]);
        exit(1);
    }

#ifdef S_ISLNK
    status = readlink(argv[1], path, PATH_MAX);
    if (status != -1) {
        printf("%.*s\n", status, path);
        return 0;
    }
#endif
    return 1;
}
