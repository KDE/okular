/* kpsexpand -- do Kpathsea variable expansion, pretending to be a given
   program.  For example,
     prompt$ kpsexpand latex2e '$TEXINPUTS'
   expands $TEXINPUTS for latex2e.

   Based on code from te@informatik.uni-hannover.de.  */

/* Not worth chance of collisions to #include anything but this.  */
#include <stdio.h> /* for stderr */

#include "tex-file.h"
extern char *kpse_var_expand ();
extern void kpse_set_progname ();
extern char *kpathsea_version_string;

int
main (argc, argv)
    int argc;
    char *argv[];
{
  if (argc < 3)
    {
      fprintf (stderr, "Usage: %s progname string [filename]\n", argv[0]);
      fprintf (stderr, "%s\n", kpathsea_version_string);
      fputs ("Sets the program name to `progname',\n", stderr);
      fputs ("then prints the variable expansion of `string'.\n", stderr);
      fputs ("If `filename' is present, does lookups of several types.\n",
             stderr);
      fputs ("Example: kpsexpand latex2e '$TEXFINPUTS'.\n", stderr);
      exit (1);
    }

  kpse_set_progname (argv[1]);
  printf ("%s\n", kpse_var_expand (argv[2]));

  if (argc == 4)
    {
      printf ("PICT: %s\n", kpse_find_pict (argv[3]));
      printf ("TEX: %s\n", kpse_find_tex (argv[3]));
      printf ("TFM: %s\n", kpse_find_tfm (argv[3]));
      printf ("VF: %s\n", kpse_find_vf (argv[3]));
    }
  return 0;
}
