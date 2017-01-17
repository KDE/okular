# - Try to find the pdfium library
# Once done this will define
#
#  PDFIUM_FOUND - system has pdfium
#  PDFIUM_INCLUDE_DIR - the pdfium include directory
#  PDFIUM_LIBRARIES_STATIC - Link this to use pdfium
#
# Copyright (c) 2017, Gilbert Assaf, <gassaf@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (PDFIUM_LIBRARIES_STATIC AND PDFIUM_INCLUDE_DIR)
  # in cache already
  set(PDFIUM_FOUND TRUE)
else (PDFIUM_LIBRARIES_STATIC AND PDFIUM_INCLUDE_DIR)

  find_path(PDFIUM_INCLUDE_DIR fpdfview.h
    ${PDFIUM_ROOT_DIR}/public
  )

  set(PDFIUM_LIBRARIES_STATIC
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libpdfium.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfpdfapi.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfxge.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfpdfdoc.a 
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfxcrt.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libfx_agg.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfxcodec.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libfx_lpng.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libfx_libopenjpeg.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libfx_lcms2.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libfx_freetype.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libjpeg.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libfx_zlib.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfdrm.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libpdfwindow.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/third_party/libbigint.a  
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libformfiller.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libjavascript.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfxedit.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfpdftext.a
    ${PDFIUM_ROOT_DIR}/out/Debug/obj/libfxjs.a
  )

  find_library(PDFIUM_LIBRARIES_SHARED_v8_libbase NAMES 
    v8_libbase
    HINTS
    ${PDFIUM_ROOT_DIR}/out/Debug
  )
  find_library(PDFIUM_LIBRARIES_SHARED_v8_libplatform NAMES 
    v8_libplatform
    HINTS
    ${PDFIUM_ROOT_DIR}/out/Debug
  )

  find_library(PDFIUM_LIBRARIES_SHARED_v8 NAMES 
    v8
    HINTS
    ${PDFIUM_ROOT_DIR}/out/Debug
  )
 
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(PDFIUM DEFAULT_MSG PDFIUM_INCLUDE_DIR PDFIUM_LIBRARIES_STATIC)
  # ensure that they are cached
  set(PDFIUM_INCLUDE_DIR ${PDFIUM_INCLUDE_DIR} CACHE INTERNAL "The pdfium include path")
  set(PDFIUM_LIBRARIES_STATIC ${PDFIUM_LIBRARIES_STATIC} CACHE INTERNAL "The static libraries needed to use pdfium")
 

endif (PDFIUM_LIBRARIES_STATIC AND PDFIUM_INCLUDE_DIR)
