# makevars.mk -- the directory names we pass.
# It's important that none of these values contain [ @%], for the sake
# of kpathsea/texmf.sed.
makevars = prefix=$(prefix) exec_prefix=$(exec_prefix) \
  bindir=$(bindir) scriptdir=$(scriptdir) libdir=$(libdir) \
  datadir=$(datadir) infodir=$(infodir) includedir=$(includedir) \
  manext=$(manext) mandir=$(mandir) \
  texmf=$(texmf) web2cdir=$(web2cdir) vartexfonts=$(vartexfonts)\
  texinputdir=$(texinputdir) mfinputdir=$(mfinputdir) mpinputdir=$(mpinputdir)\
  fontdir=$(fontdir) fmtdir=$(fmtdir) basedir=$(basedir) memdir=$(memdir) \
  texpooldir=$(texpooldir) mfpooldir=$(mfpooldir) mppooldir=$(mppooldir) \
  dvips_plain_macrodir=$(dvips_plain_macrodir) \
  dvilj_latex2e_macrodir=$(dvilj_latex2e_macrodir) \
  dvipsdir=$(dvipsdir) psheaderdir=$(psheaderdir) \
  default_texsizes='$(default_texsizes)'
# End of makevars.mk.
