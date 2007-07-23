include_directories(
   ${POPPLER_INCLUDE_DIR}
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_poppler_PART_SRCS
   generator_pdf.cpp
)

if (HAVE_POPPLER_0_6)
   set(okularGenerator_poppler_PART_SRCS
      ${okularGenerator_poppler_PART_SRCS}
      formfields.cpp
   )
endif (HAVE_POPPLER_0_6)


kde4_add_plugin(okularGenerator_poppler WITH_PREFIX ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler   ${POPPLER_LIBRARY} okularcore ${KDE4_KDEPRINT_LIBS} m )

install(TARGETS okularGenerator_poppler DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_poppler.desktop okularPoppler.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_pdf.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

