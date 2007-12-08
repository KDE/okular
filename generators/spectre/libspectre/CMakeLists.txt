include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   ${CMAKE_CURRENT_BINARY_DIR}/..
)

set(spectreOkular_SRCS
   ps.c
   spectre-device.c
   spectre-document.c
   spectre-exporter.c
   spectre-exporter-pdf.c
   spectre-exporter-ps.c
   spectre-gs.c
   spectre-page.c
   spectre-render-context.c
   spectre-status.c
   spectre-utils.c
)

kde4_add_library(spectreOkular SHARED ${spectreOkular_SRCS})

target_link_libraries(spectreOkular ${LIBGS_LIBRARY} )

set_target_properties(spectreOkular PROPERTIES VERSION 1.0.0 SOVERSION 1 )

install(TARGETS spectreOkular DESTINATION ${LIB_INSTALL_DIR})
