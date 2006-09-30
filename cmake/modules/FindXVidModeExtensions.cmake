# cmake macro to test if we have xvidmode extensions - xf86
#
#  XVIDMODEEXTENSIONS_FOUND - system has XVIDMODEEXTENSIONS libs

FIND_PATH(XVIDMODEEXTENSIONS_INCLUDE_DIR X11/extensions/xf86vmode.h
   /usr/include
   /usr/local/include
   /usr/X11R6/include
)

if (XVIDMODEEXTENSIONS_INCLUDE_DIR)
   set(XVIDMODEEXTENSIONS_FOUND TRUE)
   message(STATUS "Found X VidMode extensions: ${XVIDMODEEXTENSIONS_INCLUDE_DIR}")
else (XVIDMODEEXTENSIONS_INCLUDE_DIR)
   set(XVIDMODEEXTENSIONS_FOUND FALSE)
   message(STATUS "Did not find X VidMode extensions")
endif (XVIDMODEEXTENSIONS_INCLUDE_DIR)

