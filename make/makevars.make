# makevars.make -- the directory names we pass.
# It's important that none of these values contain [ @%], for the sake
# of kpathsea/texmf.sed.
makevars = prefix=$(prefix) exec_prefix=$(exec_prefix) \
  platform=$(platform) \
  bindir=$(bindir) scriptdir=$(scriptdir) libdir=$(libdir) \
  datadir=$(datadir) infodir=$(infodir) includedir=$(includedir) \
  manext=$(manext) mandir=$(mandir) \
  texmf=$(texmf) web2cdir=$(web2cdir) \
  texinputdir=$(texinputdir) mfinputdir=$(mfinputdir) \
  mpinputdir=$(mpinputdir) \
  fontdir=$(fontdir) \
  fmtdir=$(fmtdir) basedir=$(basedir) \
  memdir=$(memdir) \
  texpooldir=$(texpooldir) mfpooldir=$(mfpooldir) \
  mppooldir=$(mppooldir) \
  install_fonts=$(install_fonts) \
  dvipsdir=$(dvipsdir) psheaderdir=$(psheaderdir) \
  default_texsizes='$(default_texsizes)'
# End of makevars.make.
