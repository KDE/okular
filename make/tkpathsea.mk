# tkpathsea.mk -- target for remaking kpathsea.
makeargs = $(MFLAGS) CC='$(CC)' CFLAGS='$(CFLAGS)' $(XMAKEARGS)

# This is wrong: the library doesn't depend on kpsewhich.c or
# acconfig.h.  But what to do?
$(kpathsea): $(kpathsea_srcdir)/*.c $(kpathsea_srcdir)/*.h \
	     $(kpathsea_srcdir)/texmf.in $(top_srcdir)/../make/paths.mk
	cd $(kpathsea_dir) && $(MAKE) $(makeargs)
# End of tkpathsea.mk.
