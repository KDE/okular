remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_dvi")

#add_definitions(-DDEBUG_DVIRENDERER -DDEBUG_RENDER -DDEBUG_PRESCAN -DDEBUG_PSGS -DDEBUG_FONTPOOL)

if (FREETYPE_FOUND)
   add_definitions(-DHAVE_FREETYPE)
else (FREETYPE_FOUND)
   remove_definitions(-DHAVE_FREETYPE)
endif (FREETYPE_FOUND)

########### next target ###############

set(okularGenerator_dvi_SRCS
   generator_dvi.cpp
   bigEndianByteReader.cpp
   dviRenderer.cpp
   dviRenderer_draw.cpp
   dviRenderer_prescan.cpp
   dviRenderer_dr.cpp
   special.cpp
   dviFile.cpp
   dviPageInfo.cpp
   psgs.cpp
#   psheader.cpp        # already included in psgs.cpp
   glyph.cpp
   TeXFont.cpp
   TeXFontDefinition.cpp
   vf.cpp
   TeXFont_PFB.cpp
   TeXFont_PK.cpp
   TeXFont_TFM.cpp
   util.cpp
   pageSize.cpp
   simplePageSize.cpp
   length.cpp
   fontEncoding.cpp
   fontEncodingPool.cpp
   fontMap.cpp
   fontpool.cpp
   dvisourcesplitter.cpp
   dviexport.cpp
)


okular_add_generator(okularGenerator_dvi ${okularGenerator_dvi_SRCS})

target_link_libraries(okularGenerator_dvi okularcore KF6::I18n)
if (FREETYPE_FOUND)
   target_link_libraries(okularGenerator_dvi Freetype::Freetype)
endif (FREETYPE_FOUND)


########### install files ###############
if (BUILD_DESKTOP)
  install( PROGRAMS okularApplication_dvi.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()
if (BUILD_MOBILE)
  install( PROGRAMS org.kde.mobile.okular_dvi.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()
install( FILES org.kde.okular-dvi.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
