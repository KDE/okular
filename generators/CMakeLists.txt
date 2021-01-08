
function(okular_add_generator _target)
  kcoreaddons_add_plugin(${_target}
    JSON "lib${_target}.json"
    INSTALL_NAMESPACE "okular/generators"
    SOURCES ${ARGN}
  )
  set_target_properties(${_target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/okular")
endfunction()

set(LIBSPECTRE_MINIMUM_VERSION "0.2")
find_package(LibSpectre "${LIBSPECTRE_MINIMUM_VERSION}")
set_package_properties(LibSpectre PROPERTIES
        DESCRIPTION  "A PostScript rendering library"
        URL "https://libspectre.freedesktop.org"
        TYPE RECOMMENDED
        PURPOSE "Support for PS files in okular.")

find_package(KF5KExiv2 CONFIG)
set_package_properties("LibKExiv2" PROPERTIES
        DESCRIPTION  "Wrapper around Exiv2 library"
        URL "https://commits.kde.org/libkexiv2"
        TYPE RECOMMENDED
        PURPOSE "Support for image files")

find_package(CHM)
set_package_properties("CHM" PROPERTIES
        DESCRIPTION  "A library for dealing with Microsoft ITSS/CHM format files"
        URL "http://www.jedrea.com/chmlib"
        TYPE RECOMMENDED
        PURPOSE "Support CHM files in okular.")

find_package(KF5KHtml CONFIG)
set_package_properties("KF5KHtml" PROPERTIES
        DESCRIPTION "HTML rendering library"
        TYPE RECOMMENDED
        PURPOSE "Support CHM files in okular.")

find_package(LibZip)
set_package_properties("LibZip" PROPERTIES
        DESCRIPTION  "A library for reading, creating, and modifying zip archives"
        URL "https://libzip.org/"
        TYPE RECOMMENDED
        PURPOSE "Support CHM files in okular.")

find_package(DjVuLibre "3.5.17")
set_package_properties("DjVuLibre" PROPERTIES
        DESCRIPTION  "A library for dealing with DjVu formatted files"
        URL "https://djvulibre.djvuzone.org"
        TYPE RECOMMENDED
        PURPOSE "Support for DjVu files in okular.")

find_package(TIFF)
set_package_properties("libTIFF" PROPERTIES
        DESCRIPTION  "A library for reading and writing TIFF formatted files,"
        URL "http://www.libtiff.org"
        TYPE RECOMMENDED
        PURPOSE "Support for TIFF files in okular.")

find_package(Freetype)
set_package_properties("FreeType" PROPERTIES
        DESCRIPTION  "A font rendering engine"
        URL "https://www.freetype.org"
        TYPE RECOMMENDED
        PURPOSE "Provides freetype font support in the okular DVI generator.")

find_package(JPEG)
set_package_properties("JPEG" PROPERTIES
        DESCRIPTION  "A library for reading and writing JPEG image files."
        URL "https://www.ijg.org"
        TYPE RECOMMENDED
        PURPOSE "Support for PalmDB documents in okular.")

find_package(ZLIB)
set_package_properties("ZLib" PROPERTIES
        DESCRIPTION  "The Zlib compression library"
        URL "https://www.zlib.net"
        TYPE RECOMMENDED
        PURPOSE "Support for Plucker files in Okular.")

find_package(EPub)
set_package_properties("libepub" PROPERTIES
        DESCRIPTION  "A library for reading EPub documents"
        URL "http://sourceforge.net/projects/ebook-tools"
        TYPE RECOMMENDED
        PURPOSE "Support for EPub documents in Okular.")

find_package(QMobipocket "2" CONFIG)
set_package_properties("libqmobipocket" PROPERTIES
        DESCRIPTION  "A library for reading Mobipocket documents"
        URL "https://commits.kde.org/kdegraphics-mobipocket"
        TYPE RECOMMENDED
        PURPOSE "Support for Mobipocket documents in Okular.")
        
find_package(Discount)
set_package_properties("Discount" PROPERTIES
        DESCRIPTION "A library that gives you formatting functions suitable for marking down entire documents or lines of text"
        URL "https://www.pell.portland.or.us/~orc/Code/discount/"
        TYPE RECOMMENDED
        PURPOSE "Support for Markdown documents in Okular.")

# let's enable the generators properly configured

if(Poppler_Qt5_FOUND)
  add_subdirectory(poppler)
endif(Poppler_Qt5_FOUND)

if(LIBSPECTRE_FOUND)
  add_subdirectory(spectre)
endif(LIBSPECTRE_FOUND)

if(KF5KExiv2_FOUND)
  add_subdirectory( kimgio )
endif()

if(CHM_FOUND AND KF5KHtml_FOUND AND LIBZIP_FOUND)
  add_subdirectory( chm )
endif()

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

if(JPEG_FOUND AND ZLIB_FOUND)
  add_subdirectory(plucker)
endif(JPEG_FOUND AND ZLIB_FOUND)

if(EPUB_FOUND)
  add_subdirectory(epub)
endif(EPUB_FOUND)

add_subdirectory(txt)

if(QMobipocket_FOUND)
  add_subdirectory(mobipocket)
endif()

if(Discount_FOUND)
  add_subdirectory(markdown)
endif()
