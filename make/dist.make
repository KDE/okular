# dist.make -- how to make the distribution tar file.

top_distdir = $(distname)-$(version)
top_files = ChangeLog FTP Makefile.in configure configure.in README \
  $(HOME)/gnu/gnuorg/COPYING* $(HOME)/gnu/gnuorg/install-sh \
  $(HOME)/bin/mkdirchain \
  $(plain)/texinfo.tex
distdir = $(top_distdir)/$(distname)
kpathsea_distdir = ../$(distname)/$(top_distdir)/kpathsea
ln_files = AUTHORS ChangeLog INSTALL NEWS README TAGS *.in *.h *.c \
  configure *.make .gdbinit stamp-auto

dist: depend.make TAGS pre-dist-$(distname)
	rm -rf $(top_distdir)*
	mkdir -p $(distdir)
	cd ..; make Makefile ./configure
	cd ..; cp -p $(top_files) $(distname)/$(top_distdir)
	ln -s $(gnu)/share/autoconf/acsite.m4 $(top_distdir)/aclocal.m4
	-ln $(ln_files) $(distdir)
	ln $(program_files) $(distdir)
	cd $(kpathsea_dir); $(MAKE) distdir=$(kpathsea_distdir) \
	  ln_files='$(ln_files)' distdir
	cp -rp ../make $(top_distdir)
	ungnumake $(distdir)/Makefile.in $(kpathsea_distdir)/Makefile.in \
	  $(top_distdir)/Makefile.in $(top_distdir)/make/*.make
# Remove the extra files our patterns got us.
	cd $(top_distdir); rm -f */depend.make */c-auto.h */Makefile
	$(MAKE) post-dist-$(distname)
	cd $(distdir); add-version $(version) $(version_files)
	cd $(distdir); test ! -r *.info || touch *.info*
	chmod -R a+rwX $(top_distdir)
	GZIP=-9 tar chzf $(top_distdir).tar.gz $(top_distdir)
	rm -rf $(top_distdir)
