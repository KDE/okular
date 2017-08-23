# an own directory for multi-size icons retrieved by KIconThemeLoader
add_subdirectory(icons)

# install annotator xml tools description
install(FILES
   tools.xml
   drawingtools.xml
   DESTINATION ${KDE_INSTALL_DATADIR}/okular)

# install annotation tool images
ecm_install_icons(ICONS
    32-actions-tool-base-okular.png
    64-actions-tool-base-okular.png
    32-actions-tool-highlighter-okular-colorizable.png
    64-actions-tool-highlighter-okular-colorizable.png
    32-actions-tool-ink-okular-colorizable.png
    64-actions-tool-ink-okular-colorizable.png
#   tool-note.png
    32-actions-tool-note-okular-colorizable.png
    64-actions-tool-note-okular-colorizable.png
    32-actions-tool-note-inline-okular.png
    64-actions-tool-note-inline-okular.png
    32-actions-tool-note-okular-colorizable.png
    64-actions-tool-note-inline-okular-colorizable.png
    DESTINATION ${DATA_INSTALL_DIR}/okular/icons)

# install annotation page images
ecm_install_icons(ICONS
    64-actions-checkmark.png
    128-actions-checkmark.png
    64-actions-circle.png
    128-actions-circle.png
    64-actions-comment.png
    128-actions-comment.png
    64-actions-cross.png
    128-actions-cross.png
    64-actions-help.png
    128-actions-help.png
    64-actions-insert.png
    128-actions-insert.png
    64-actions-key.png
    128-actions-key.png
    64-actions-newparagraph.png
    128-actions-newparagraph.png
    64-actions-note.png
    128-actions-note.png
#   paperclip.png
    64-actions-paragraph.png
    128-actions-paragraph.png
#   pushpin.png
    64-actions-rightarrow.png
    128-actions-rightarrow.png
    64-actions-rightpointer.png
    128-actions-rightpointer.png
    64-actions-star.png
    128-actions-star.png
    64-actions-uparrow.png
    128-actions-uparrow.png
    64-actions-upleftarrow.png
    128-actions-upleftarrow.png
    DESTINATION ${DATA_INSTALL_DIR}/okular/icons)

# install annotation stamps
install(FILES
   stamps.svg
   DESTINATION ${KDE_INSTALL_DATADIR}/okular/pics)
# install misc images

# newstuff
if (FALSE)
install(FILES
   okular.knsrc
   DESTINATION ${KDE_INSTALL_CONFDIR})
endif (FALSE)

