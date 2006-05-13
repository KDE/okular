# install annotator xml tools description
install_files(${DATA_INSTALL_DIR}/oKular FILES
   tools.xml
)
# install annotation tool images
install_files(${DATA_INSTALL_DIR}/oKular/pics FILES
   tool_eraser.png
   tool_hl_orange.png
   tool_hl_pink.png
   tool_hl_yellow.png
   tool_ink_green.png
   tool_note.png
)
# install annotation page images
install_files(${DATA_INSTALL_DIR}/oKular/pics FILES
   checkmark.png
   circle.png
   comment.png
   cross.png
   help.png
   insert.png
   key.png
   newparagraph.png
   note.png
   paragraph.png
   rightarrow.png
   rightpointer.png
   star.png
   uparrow.png
   upleftarrow.png
)
# install misc images
install_files(${DATA_INSTALL_DIR}/oKular/pics FILES
   ghns.png
   ghns_star.png
   ghns_star_gray.png
)

#original Makefile.am contents follow:

## install annotation tool images
#toolpicsdir = $(kde_datadir)/oKular/pics
#toolpics_DATA = tool_eraser.png tool_hl_orange.png tool_hl_pink.png \
#                tool_hl_yellow.png tool_ink_green.png tool_note.png
#
## install annotation page images
#annpicsdir = $(kde_datadir)/oKular/pics
#annpics_DATA = checkmark.png circle.png comment.png cross.png help.png \
#               insert.png key.png newparagraph.png note.png paragraph.png \
#               rightarrow.png rightpointer.png star.png uparrow.png \
#               upleftarrow.png
#
## install misc images
#miscpicsdir = $(kde_datadir)/oKular/pics
#miscpics_DATA = ghns.png ghns_star.png ghns_star_gray.png
#
## install annotator xml tools description
#xmldir = $(kde_datadir)/oKular
#xml_DATA = tools.xml
