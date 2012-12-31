include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}/okular
)

#add_definitions(-DDEBUG_DVIRENDERER -DDEBUG_RENDER -DDEBUG_PRESCAN -DDEBUG_PSGS -DDEBUG_FONTPOOL)

if (FREETYPE_FOUND)
   add_definitions(-DHAVE_FREETYPE)
   include_directories(${FREETYPE_INCLUDE_DIRS})
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
   fontprogress.cpp
   dvisourcesplitter.cpp
   dviexport.cpp
)


kde4_add_plugin(okularGenerator_dvi ${okularGenerator_dvi_SRCS})

target_link_libraries(okularGenerator_dvi okularcore ${KDE4_KIO_LIBS} ${MATH_LIB})
if (FREETYPE_FOUND)
   target_link_libraries(okularGenerator_dvi ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)

install(TARGETS okularGenerator_dvi DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_dvi.desktop okularDvi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_dvi.desktop active-documentviewer_dvi.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

