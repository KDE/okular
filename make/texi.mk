# texi.mk -- making .dvi and .info from .texi.
@MAINT@MAKEINFO = makeinfo
@MAINT@MAKEINFO_FLAGS = --paragraph-indent=2 -I$(srcdir)

@MAINT@TEXI2DVI = texi2dvi
@MAINT@# To make sure that generation of the dvi files succeeds on the
@MAINT@# maintainer's system.
@MAINT@TEXI2DVI = TEXMFCNF=$(web2cdir) texi2dvi

@MAINT@TEXI2HTML = texi2html
@MAINT@TEXI2HTML_FLAGS = -expandinfo -number -menu -split_chapter
# If you prefer one big .html file instead of several, remove
# -split-node or replace it by -split_chapter.

# For making normal text files out of Texinfo source.
@MAINT@one_info = --no-headers --no-split --no-validate

@MAINT@.SUFFIXES: .info .dvi .html .texi
@MAINT@.texi.info:
@MAINT@	$(MAKEINFO) $(MAKEINFO_FLAGS) $< -o $@
@MAINT@.texi.dvi:
@MAINT@	$(TEXI2DVI) $(TEXI2DVI_FLAGS) $<
@MAINT@.texi.html:
@MAINT@	$(TEXI2HTML) $(TEXI2HTML_FLAGS) $< 
# End of texi.mk.
