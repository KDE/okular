# an own directory for multi-size icons retrieved by KIconLoader
add_subdirectory(icons)

# install annotator xml tools description
install(FILES
   tools.xml
   DESTINATION ${DATA_INSTALL_DIR}/okular)

# install annotation tool images
install(FILES
   tool-base-okular.png
   tool-highlighter-okular-colorizable.png
   tool-ink-okular-colorizable.png
   tool-note.png
   tool-note-okular-colorizable.png
   tool-note-inline.png
   tool-note-inline-okular-colorizable.png
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
   paperclip.png
   paragraph.png
   pushpin.png
   rightarrow.png
   rightpointer.png
   star.png
   uparrow.png
   upleftarrow.png
   DESTINATION ${DATA_INSTALL_DIR}/okular/pics)
# install annotation stamps
install(FILES
   stamps.svg
   DESTINATION ${DATA_INSTALL_DIR}/okular/pics)
# install misc images

# newstuff
if (FALSE)
install(FILES
   okular.knsrc
   DESTINATION ${CONFIG_INSTALL_DIR})
endif (FALSE)

