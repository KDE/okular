/* acconfig.h -- used by autoheader when generating c-auto.in.

   If you're thinking of editing acconfig.h to fix a configuration
   problem, don't. Edit the c-auto.h file created by configure,
   instead.  Even better, fix configure to give the right answer.  */

/* Guard against double inclusion. */
#ifndef KPATHSEA_C_AUTO_H
#define KPATHSEA_C_AUTO_H

/* kpathsea: the version string. */
#define KPSEVERSION "REPLACE-WITH-KPSEVERSION"

/* kpathsea/configure.in tests for these functions with
   kb_AC_KLIBTOOL_REPLACE_FUNCS, and naturally Autoheader doesn't know
   about that macro.  Since the shared library stuff is all preliminary
   anyway, I decided not to change Autoheader, but rather to hack them
   in here.  */
#undef HAVE_PUTENV
#undef HAVE_STRCASECMP
#undef HAVE_STRTOL
#undef HAVE_STRSTR

@TOP@

/* Define if your compiler understands prototypes.  */
#undef HAVE_PROTOTYPES

/* Define if your putenv doesn't waste space when the same environment
   variable is assigned more than once, with different (malloced)
   values.  This is true only on NetBSD/FreeBSD, as far as I know. See
   xputenv.c.  */
#undef SMART_PUTENV

/* Define if getcwd if implemented using fork or vfork.  Let me know
   if you have to add this by hand because configure failed to detect
   it. */
#undef GETCWD_FORKS

/* Define if you are using GNU libc or otherwise have global variables
   `program_invocation_name' and `program_invocation_short_name'.  */
#undef HAVE_PROGRAM_INVOCATION_NAME

/* all: Define to enable running scripts when missing input files.  */
#define MAKE_TEX_MF_BY_DEFAULT 0
#define MAKE_TEX_PK_BY_DEFAULT 0
#define MAKE_TEX_TEX_BY_DEFAULT 0
#define MAKE_TEX_TFM_BY_DEFAULT 0
#define MAKE_OMEGA_OFM_BY_DEFAULT 0
#define MAKE_OMEGA_OCP_BY_DEFAULT 0

@BOTTOM@
#endif /* !KPATHSEA_C_AUTO_H */
