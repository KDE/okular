include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${CMAKE_BINARY_DIR}/okular
)

#add_definitions(-DDEBUG_DVIRENDERER -DDEBUG_RENDER -DDEBUG_PRESCAN)

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
   psheader.cpp   
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
)

kde4_automoc(${okularGenerator_dvi_SRCS})

kde4_add_plugin(okularGenerator_dvi WITH_PREFIX ${okularGenerator_dvi_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_dvi )

target_link_libraries(okularGenerator_dvi ${KDE4_KDECORE_LIBS} okularcore m ${KDE4_KDEPRINT_LIBS} ${KDE4_KDEUI_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} okularGenerator_dvi )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  libokularGenerator_dvi.desktop )

