# misc.make -- cleaning, etc.
TAGS: *.c *.h
	if pwd | grep kpathsea >/dev/null; then \
	  etags *.c *.h; else etags -i $(kpathsea_dir)/TAGS *.c *.h; fi

mostlyclean::
	rm -f *.o $(program) $(programs) squeeze $(library).a

clean:: mostlyclean
	rm -f *.dvi *.lj

distclean:: clean
	rm -f Makefile MakeTeXPK *.pool
	rm -f config.status config.log config.cache c-auto.h 

# Although we can remake configure and c-auto.h.in, we don't remove
# them, since many people may lack Autoconf.  Use configclean for that.
realclean:: distclean
	rm -f TAGS *.info*

extraclean::
	rm -f *.aux *.bak *.bbl *.blg *.dvi *.log *.orig *.pl *.rej
	rm -f *.i *.s *.tfm *.vf *.vpl *\#* *gf *pk *~
	rm -f CONTENTS.tex a.out core mfput.* texput.*

configclean:
	rm -f configure c-auto.h.in c-auto.h

# Prevent GNU make 3.[59,63) from overflowing arg limit on system V.
.NOEXPORT:
# End of misc.make.
