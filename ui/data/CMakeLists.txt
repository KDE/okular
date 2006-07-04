# install annotator xml tools description
install(FILES
   tools.xml
   DESTINATION ${DATA_INSTALL_DIR}/okular)

# install annotation tool images
install(FILES
   tool_eraser.png
   tool_hl_orange.png
   tool_hl_pink.png
   tool_hl_yellow.png
   tool_ink_green.png
   tool_note.png
   DESTINATION ${DATA_INSTALL_DIR}/okular/pics)
# install annotation page images
install(FILES
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
   DESTINATION ${DATA_INSTALL_DIR}/okular/pics)
# install misc images
install(FILES
   ghns.png
   ghns_star.png
   ghns_star_gray.png
   DESTINATION ${DATA_INSTALL_DIR}/okular/pics)

#original Makefile.am contents follow:

## install annotation tool images
#toolpicsdir = $(kde_datadir)/okular/pics
#toolpics_DATA = tool_eraser.png tool_hl_orange.png tool_hl_pink.png \
#                tool_hl_yellow.png tool_ink_green.png tool_note.png
#
## install annotation page images
#annpicsdir = $(kde_datadir)/okular/pics
#annpics_DATA = checkmark.png circle.png comment.png cross.png help.png \
#               insert.png key.png newparagraph.png note.png paragraph.png \
#               rightarrow.png rightpointer.png star.png uparrow.png \
#               upleftarrow.png
#
## install misc images
#miscpicsdir = $(kde_datadir)/okular/pics
#miscpics_DATA = ghns.png ghns_star.png ghns_star_gray.png
#
## install annotator xml tools description
#xmldir = $(kde_datadir)/okular
#xml_DATA = tools.xml
