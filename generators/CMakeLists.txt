
function(okular_add_generator _target)
  kcoreaddons_add_plugin(${_target}
    INSTALL_NAMESPACE "okular_generators"
    SOURCES ${ARGN}
  )
  get_property(aux_list GLOBAL PROPERTY okular_generator_list)
  list(APPEND aux_list ${_target})
  set_property(GLOBAL PROPERTY okular_generator_list ${aux_list})
endfunction()

# let's enable the generators properly configured

if(Poppler_Qt6_FOUND)
  add_subdirectory(poppler)
endif(Poppler_Qt6_FOUND)

if(LIBSPECTRE_FOUND)
  add_subdirectory(spectre)
endif(LIBSPECTRE_FOUND)

add_subdirectory( kimgio )

if(DJVULIBRE_FOUND)
  add_subdirectory(djvu)
endif(DJVULIBRE_FOUND)

add_subdirectory(dvi)

if(TIFF_FOUND)
  add_subdirectory(tiff)
endif(TIFF_FOUND)

add_subdirectory(xps)

add_subdirectory(fictionbook)

add_subdirectory(comicbook)

add_subdirectory(fax)

if(EPUB_FOUND)
  add_subdirectory(epub)
endif(EPUB_FOUND)

add_subdirectory(txt)

if(QMobipocket6_FOUND)
  add_subdirectory(mobipocket)
endif()

if(Discount_FOUND)
  add_subdirectory(markdown)
endif()
