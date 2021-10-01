# install annotator xml tools description
install(FILES
   tools.xml
   toolsQuick.xml
   drawingtools.xml
   DESTINATION ${KDE_INSTALL_DATADIR}/okular)

# install annotation tool images
install(FILES
   tool-base-okular.png
   tool-base-okular@2x.png
   tool-highlighter-okular-colorizable.png
   tool-highlighter-okular-colorizable@2x.png
   tool-ink-okular-colorizable.png
   tool-ink-okular-colorizable@2x.png
   tool-note.png
   tool-note-okular-colorizable.png
   tool-note-okular-colorizable@2x.png
   tool-note-inline.png
   tool-note-inline-okular-colorizable.png
   tool-note-inline-okular-colorizable@2x.png
   tool-typewriter-okular-colorizable.png
   tool-typewriter-okular-colorizable@2x.png
   DESTINATION ${KDE_INSTALL_DATADIR}/okular/pics)
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
   DESTINATION ${KDE_INSTALL_DATADIR}/okular/pics)
# install misc images
