# man.mk: Makefile fragment for web2c manual pages.

#DITROFF = ditroff
DITROFF = groff

# The edited file always has extension .1; we change it when we install.
.SUFFIXES: .man .1 .txt .ps .dvi
.man.1:
	sed -f sedscript $< >$@
.1.dvi:
	$(DITROFF) -Tdvi -man $< >$@
.1.ps:
	$(DITROFF) -Tps -man $< >$@
.1.txt:
	$(DITROFF) -Tascii -man $< | col -b | expand >$@

all: $(manfiles)
.PHONY: dw

$(manfiles): sedscript

manfiles: $(manfiles)
dvi: $(manfiles:.1=.dvi)
ps: $(manfiles:.1=.ps)
txt: $(manfiles:.1=.txt)

# We do not depend on the top-level Makefile since the top-level
# Makefile can change for reasons that do not affect the man pages.
# At present, all but VERSION should be unused.
sedscript:
	cp /dev/null sedscript
	for f in $(kpathsea_dir)/paths.h; do \
	  sed -n -e '/^#define/s/#define[ 	][ 	]*\([A-Z_a-z][A-Z_a-z]*\)[ 	][ 	]*\(.*\)/s%@\1@%\2%/p' \
		$$f \
	  | sed -e 's/"//g' -e 's/[ 	]*\/\*[^*]*\*\///g' >>sedscript;\
	done
	echo 's%@VERSION@%$(version)%'		>>sedscript
	echo 's%@BINDIR@%$(bindir)%'		>>sedscript
	echo 's%@INFODIR@%$(infodir)%'		>>sedscript
	echo 's%@TEXINPUTDIR@%$(texinputdir)%'	>>sedscript
	echo 's%@MFINPUTDIR@%$(mfinputdir)%'	>>sedscript
	echo 's%@MPINPUTDIR@%$(mpinputdir)%'	>>sedscript
	echo 's%@FONTDIR@%$(fontdir)%'		>>sedscript
	echo 's%@FMTDIR@%$(fmtdir)%'		>>sedscript
	echo 's%@BASEDIR@%$(basedir)%'		>>sedscript
	echo 's%@MEMDIR@%$(memdir)%'		>>sedscript
	echo 's%@TEXPOOLDIR@%$(texpooldir)%'	>>sedscript
	echo 's%@MFPOOLDIR@%$(mfpooldir)%'	>>sedscript
	echo 's%@MPPOOLDIR@%$(mppooldir)%'	>>sedscript
	echo 's%@FONTMAPDIR@%$(dvipsdir)%'	>>sedscript
	echo 's%@LOCALMODES@%$(localmodes)%'	>>sedscript

install-man: manfiles
	$(top_srcdir)/../mkinstalldirs $(mandir)
	for nameone in $(manfiles); do					\
          name=`basename $${nameone} .1`;				\
          $(INSTALL_DATA) $${name}.1 $(mandir)/$${name}.$(manext);	\
        done

uninstall-man:
	for nameone in $(manfiles); do					\
	  name=`basename $${nameone} .1`;				\
	  rm -f $(mandir)/$${name}.$(manext);				\
	done

install-data: install-man
uninstall-data: uninstall-man

mostlyclean::
	rm -f *.1

clean::
	rm -f sedscript

# end of man.mk
