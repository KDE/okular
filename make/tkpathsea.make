# tkpathsea.make -- remaking kpathsea.

makeargs = $(MFLAGS) CC='$(CC)' CFLAGS='$(CFLAGS)' $(XMAKEARGS)

$(kpathsea): $(kpathsea_srcdir)/*.c $(kpathsea_srcdir)/*.h \
	     $(kpathsea_srcdir)/texmf.cnf.in $(top_srcdir)/../make/paths.make
	cd $(kpathsea_dir); $(MAKE) $(makeargs)

# End of tkpathsea.make.
