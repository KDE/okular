# programs.make -- used by Makefiles for executables only.
# Linking. Don't include $(CFLAGS), since ld -g under Linux forces
# static libraries, including libc.a and libX*.a
LDFLAGS = @LDFLAGS@ $(XLDFLAGS)
LIBS = @LIBS@
# proglib is for web2c; 
# XLOADLIBES is for the installer.
LOADLIBES= $(proglib) $(kpathsea) $(LIBS) -lm $(XLOADLIBES)

# Why separate CCLD from CC?  No particular reason.
CCLD = $(CXX)
link_command = $(CCLD) -o $@ $(LDFLAGS) 
# End of programs.make.
