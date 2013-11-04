include (MacroLogFeature)

set(LIBSPECTRE_MINIMUM_VERSION "0.2")

macro_optional_find_package(Poppler)
macro_log_feature(HAVE_POPPLER_0_12_1 "Poppler-Qt4" "A PDF rendering library" "http://poppler.freedesktop.org" FALSE "0.12.1" "Support for PDF files in okular.")

macro_optional_find_package(LibSpectre)
macro_log_feature(LIBSPECTRE_FOUND "libspectre" "A PostScript rendering library" "http://libspectre.freedesktop.org/wiki/" FALSE "${LIBSPECTRE_MINIMUM_VERSION}" "Support for PS files in okular.")

macro_optional_find_package(Kexiv2)
macro_log_feature(KEXIV2_FOUND "LibKExiv2" "Wrapper around Exiv2 library" "http://www.digikam.org/sharedlibs" FALSE "" "Support for image files")

macro_optional_find_package(CHM)
macro_log_feature(CHM_FOUND "CHM" "A library for dealing with Microsoft ITSS/CHM format files" "http://www.jedrea.com/chmlib" FALSE "" "Support CHM files in okular.")

macro_optional_find_package(DjVuLibre)
macro_log_feature(DJVULIBRE_FOUND "DjVuLibre" "A library for dealing with DjVu formatted files" "http://djvulibre.djvuzone.org" FALSE "3.5.17" "Support for DjVu files in okular.")

macro_optional_find_package(TIFF)
macro_log_feature(TIFF_FOUND "libTIFF" "A library for reading and writing TIFF formatted files," "http://www.remotesensing.org/libtiff" FALSE "" "Support for TIFF files in okular.")

macro_optional_find_package(Freetype)
macro_log_feature(FREETYPE_FOUND "FreeType" "A font rendering engine" "http://www.freetype.org" FALSE "" "Provides freetype font support in the okular DVI generator.")

macro_optional_find_package(JPEG)
macro_log_feature(JPEG_FOUND "JPEG" "A library for reading and writing JPEG image files." "http://www.ijg.org" FALSE "" "Support fof PalmDB documents in okular.")
macro_optional_find_package(ZLIB)
macro_log_feature(ZLIB_FOUND "ZLib" "The Zlib compression library" "http://www.zlib.net" FALSE "" "Support for Plucker files in Okular.")

macro_optional_find_package(EPub)
macro_log_feature(EPUB_FOUND "libepub" "A library for reading EPub documents" "http://sourceforge.net/projects/ebook-tools" FALSE "" "Support for EPub documents in Okular.")

macro_optional_find_package(QMobipocket)
macro_log_feature(QMOBIPOCKET_FOUND "libqmobipocket" "A library for reading Mobipocket documents" "https://projects.kde.org/projects/kde/kdegraphics/kdegraphics-mobipocket" FALSE "" "Support for Mobipocket documents in Okular.")

# let's enable the generators properly configured

if(POPPLER_FOUND AND HAVE_POPPLER_0_12_1)
  add_subdirectory(poppler)
endif(POPPLER_FOUND AND HAVE_POPPLER_0_12_1)

if(LIBSPECTRE_FOUND)
  add_subdirectory(spectre)
endif(LIBSPECTRE_FOUND)

if(KEXIV2_FOUND)
  add_subdirectory( kimgio )
endif(KEXIV2_FOUND)

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

add_subdirectory(fax)

if(JPEG_FOUND AND ZLIB_FOUND)
  add_subdirectory(plucker)
endif(JPEG_FOUND AND ZLIB_FOUND)

if(EPUB_FOUND)
  add_subdirectory(epub)
endif(EPUB_FOUND)

add_subdirectory(txt)

if(QMOBIPOCKET_FOUND)
  add_subdirectory(mobipocket)
endif()
