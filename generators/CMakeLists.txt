include (MacroLogFeature)

set(LIBGS_MINIMUM_VERSION "8.56")

macro_optional_find_package(Poppler)
macro_log_feature(POPPLER_FOUND "Poppler-Qt4" "A PDF rendering library" "http://poppler.freedesktop.org" FALSE "0.5.4" "Support for PDF files in okular.")

macro_optional_find_package(LIBGS)
macro_log_feature(LIBGS_FOUND "libgs, Ghostscript libraries" "A PostScript renderining library" "http://www.cs.wisc.edu/~ghost" FALSE "${LIBGS_MINIMUM_VERSION}" "Support for PS files in okular.")

macro_optional_find_package(CHM)
macro_log_feature(CHM_FOUND "CHM" "A library for dealing with Microsoft ITSS/CHM format files" "http://www.jedrea.com/chmlib" FALSE "" "Support CHM files in okular.")

macro_optional_find_package(DjVuLibre)
macro_log_feature(DJVULIBRE_FOUND "DjVuLibre" "A library for dealing with DjVu formatted files" "http://djvulibre.djvuzone.org" FALSE "3.5.17" "Support for DjVu files in okular.")

macro_optional_find_package(TIFF)
macro_log_feature(TIFF_FOUND "libTIFF" "A library for reading and writing TIFF formatted files," "http://www.remotesensing.org/libtiff" FALSE "" "Support for TIFF files in okular.")

macro_optional_find_package(Freetype)
macro_log_feature(FREETYPE_FOUND "FreeType" "A font rendering engine" "http://www.freetype.org" FALSE "" "Provides freetype font support in the okular DVI generator.")

#we look for JPEG in kdegraphics/CMakeLists.txt
#macro_optional_find_package(JPEG)
#macro_log_feature(JPEG_FOUND "JPEG" "A library for reading and writing JPEG image files." "http://www.ijg.org" FALSE "" "Support fof PalmDB documents in okular.")

# let's enable the generators properly configured

if(POPPLER_FOUND)
  add_subdirectory(poppler)
endif(POPPLER_FOUND)

if(LIBGS_FOUND)
  add_subdirectory(spectre)
endif(LIBGS_FOUND)

add_subdirectory( kimgio )

if(CHM_FOUND)
  add_subdirectory( chm )
endif(CHM_FOUND)

if(DJVULIBRE_FOUND)
  add_subdirectory(djvu)
endif(DJVULIBRE_FOUND)

add_subdirectory(dvi)

if(TIFF_FOUND)
  add_subdirectory(tiff)
endif(TIFF_FOUND)

add_subdirectory(xps)

add_subdirectory(ooo)

add_subdirectory(fictionbook)

add_subdirectory(comicbook)

if(JPEG_FOUND)
  add_subdirectory(plucker)
endif(JPEG_FOUND)
