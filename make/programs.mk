# programs.mk -- used by Makefiles for executables only.

# Don't include $(CFLAGS), since ld -g under Linux forces
# static libraries, e.g., libc.a and libX*.a.
LDFLAGS = @LDFLAGS@ $(XLDFLAGS)

# proglib is for web2c; 
# XLOADLIBES is for the installer.
LIBS = @LIBS@
LOADLIBES = $(proglib) $(kpathsea) $(LIBS) -lm $(XLOADLIBES)

# May as well separate linking from compiling, just in case.
CCLD = $(CC)
link_command = $(CCLD) -o $@ $(LDFLAGS) 

# When we link with Kpathsea, have to take account that it might be a
# shared library, etc.
kpathsea_link = $(LIBTOOL) link $(link_command)
# End of programs.mk.
