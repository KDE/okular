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

# newstuff
install(FILES
   okular.knsrc
   DESTINATION ${CONFIG_INSTALL_DIR})

