/* acconfig.h -- used by autoheader when generating c-auto.h.in.  */

/* Define if your putenv doesn't waste space when the same environment
   variable is assigned more than once, with different (malloced)
   values.  This is true only on NetBSD/FreeBSD, as far as I know. See
   xputenv.c.  */
#undef SMART_PUTENV

/* Define if you are using GNU libc or otherwise have global variables
   `program_invocation_name' and `program_invocation_short_name'.  */
#undef HAVE_PROGRAM_INVOCATION_NAME

/* Define if you get clashes concerning wchar_t, between X's include
   files and system includes.  */
#undef FOIL_X_WCHAR_T

/* Define if you have SIGIO, F_SETOWN, and FASYNC.  */
#undef HAVE_SIGIO
