/*
 * Copyright (c) 1994 Paul Vojta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * |||	to do:
 *	*interpreter resource
 *	-safer argument
 *	palette resource and arguments
 */

#include "oconfig.h"
#ifdef PS_GS /* whole file */

#include "kpathsea/c-pathmx.h"
#include <X11/Xatom.h>
#include <sys/time.h> /* for timeval */

#include <signal.h>

#ifdef NeXT
typedef int pid_t;
#endif
 
/* if POSIX O_NONBLOCK is not available, use O_NDELAY */
#if !defined(O_NONBLOCK) && defined(O_NDELAY)
#define	O_NONBLOCK O_NDELAY
#endif

/* Condition for retrying a write */
#include <errno.h>
#ifdef	EWOULDBLOCK
#ifdef	EAGAIN
#define	AGAIN_CONDITION	(errno == EWOULDBLOCK || errno == EAGAIN)
#else	/* EAGAIN */
#define	AGAIN_CONDITION	(errno == EWOULDBLOCK)
#endif	/* EAGAIN */
#else	/* EWOULDBLOCK */
#ifdef	EAGAIN
#define	AGAIN_CONDITION	(errno == EAGAIN)
#endif	/* EAGAIN */
#endif	/* EWOULDBLOCK */


#ifdef	STREAMSCONN
#include <poll.h>
#else
#ifdef _AIX
#include <sys/select.h> /* for fd_set stuff on RS/6000 */
#else
#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* for fd_set on ISC 4.0 */
#endif /* not <sys/bsdtypes.h> */
#endif /* not _AIX */
#endif /* not STREAMSCONN */

#if	HAS_SIGIO
#include <fcntl.h>
#include <signal.h>
#ifndef	FASYNC
#undef	HAS_SIGIO
#define	HAS_SIGIO 0
#endif
#endif

#ifdef	VFORK
#if	VFORK == include
#include <vfork.h>
#endif
#else
#define	vfork	fork
#endif

extern	_Xconst	char	psheader[];
extern	int		psheaderlen;

#define	postscript	_postscript
#define	fore_Pixel	_fore_Pixel
#define	back_Pixel	_back_Pixel

extern void qt_processEvents();

/* global procedures (besides initGS) */

static	void	toggle_gs ARGS((void));
static	void	destroy_gs ARGS((void));
static	void	interrupt_gs ARGS((void));
static	void	endpage_gs ARGS((void));
static	void	drawbegin_gs ARGS((int, int, char *));
static	void	drawraw_gs ARGS((char *));
static	void	drawfile_gs ARGS((char *));
static	void	drawend_gs ARGS((char *));

static	struct psprocs	gs_procs = {
	/* toggle */		toggle_gs,
	/* destroy */		destroy_gs,
	/* interrupt */		interrupt_gs,
	/* endpage */		endpage_gs,
	/* drawbegin */		drawbegin_gs,
	/* drawraw */		drawraw_gs,
	/* drawfile */		drawfile_gs,
	/* drawend */		drawend_gs};

static	int	std_in[2];
static	int	std_out[2];

#define	GS_in	(std_in[1])
#define	GS_out	(std_out[0])

static	char	*argv[]	= {"gs", "-sDEVICE=x11alpha", "-dNOPAUSE", "-dSAFER", "-q", "-", NULL};

static	pid_t		GS_pid;
static	unsigned int	GS_page_w;	/* how big our current page is */
static	unsigned int	GS_page_h;
static	int		GS_mag;		/* magnification currently in use */
static	int		GS_shrink;	/* shrink factor currently in use */
static	Boolean		GS_active;	/* if we've started a page yet */
static	int		GS_pending;	/* number of ack's we're expecting */
static	Boolean		GS_sending;	/* if we're in the middle of send() */
static	Boolean		GS_pending_int;	/* if interrupt rec'd while in send() */

static	Atom	gs_atom;
static	Atom	gs_colors_atom;
#define	XtPageOrientationPortrait 0

/*
 *	Our replacement for setenv(), which is not available on all systems.
 */

#ifndef	HAVE_SETENV	/* define this if you're a performance freak and
			   if your system has setenv. */
#define	setenv(var, str, repl)	_setenv(var, str)	/* repl always True */

extern	char	**environ;

static	void
_setenv(var, str)
	_Xconst	char	*var;
	_Xconst	char	*str;
{
	int		len1;
	int		len2;
	char		*newvar;
	char		**linep;
	static	Boolean	malloced = False;

	len1 = strlen(var);
	len2 = strlen(str) + 1;
	newvar = xmalloc((unsigned int) len1 + len2 + 1, "_setenv");
	(void) bcopy(var, newvar, len1);
	newvar[len1++] = '=';
	(void) bcopy(str, newvar + len1, len2);
	for (linep = environ; *linep != NULL; ++linep)
	    if (memcmp(*linep, newvar, len1) == 0) {
		*linep = newvar;
		return;
	    }
	len1 = linep - environ;
	if (malloced) {
	    environ = (char **) realloc((char *) environ,
		(unsigned int) (len1 + 2) * sizeof(char *));
	    if (environ == NULL)
		oops("! Cannot allocate %d bytes for string list in _setenv.\n",
		    (len1 + 2) * sizeof(char *));
	}
	else {
	    linep = (char **) xmalloc((unsigned int)(len1 + 2) * sizeof(char *),
		"string list in _setenv");
	    (void) bcopy((char *) environ, (char *) linep,
		len1 * sizeof(char *));
	    environ = linep;
	    malloced = True;
	}
	environ[len1] = newvar;
	environ[len1 + 1] = NULL;
}

#endif	/* HAVE_SETENV */

/*
 *	ghostscript I/O code.  This should send PS code to ghostscript,
 *	receive acknowledgements, and receive X events in the meantime.
 *	It also checks for SIGPIPE errors.
 */

#ifndef	STREAMSCONN
static	int		numfds;
static	fd_set		readfds;
static	fd_set		writefds;
#define	XDVI_ISSET(a, b, c)	FD_ISSET(a, b)
#else	/* STREAMSCONN */
struct pollfd		fds[3] = {{0, POLLOUT, 0},
				  {0, POLLIN, 0},
				  {0, POLLIN, 0}};
#define	XDVI_ISSET(a, b, c)	(fds[c].revents)
#endif	/* STREAMSCONN */

#define	LINELEN	81
static	char	line[LINELEN + 1];
static	char	*linepos	= line;
static	char	ackstr[]	= "\347\310\376";

static	void
read_from_gs() {
	int	bytes;
	char	*line_end;
	char	*p;

	bytes = read(GS_out, linepos, line + LINELEN - linepos);
	if (bytes < 0) return;
	line_end = linepos + bytes;
	/* Check for ack strings */
	for (p = line; p < line_end - 2; ++p) {
	    p = memchr(p, '\347', line_end - p - 2);
	    if (p == NULL) break;
	    if (memcmp(p, ackstr, 3) == 0) {
		--GS_pending;
		if (debug & DBG_PS)
		    Printf("Got GS ack; %d pending.\n", GS_pending);
		if (p > line) {
		    *p = '\0';
		    Printf("gs: %s\n", line);
		}
		p += 3;
		(void) bcopy(p, line, line_end - p);
		line_end -= p - line;
		linepos = p = line;
		--p;
	    }
	}
	for (;;) {
	    p = memchr(linepos, '\n', line_end - linepos);
	    if (p == NULL) break;
	    *p = '\0';
	    Printf("gs: %s\n", line);
	    ++p;
	    (void) bcopy(p, line, line_end - p);
	    line_end -= p - line;
	    linepos = line;
	}
	linepos = line_end;
	/*
	 * Normally we'd hold text until a newline character, but the buffer
	 * is full.  So we flush it, being careful not to cut up an ack string.
	 */
	if (linepos >= line + LINELEN) {
	    p = line + LINELEN;
	    if ((*--p != '\347' && *--p != '\347' && *--p != '\347')
		    || memcmp(p, ackstr, line + LINELEN - p) != 0)
		p = line + LINELEN;
	    *p = '\0';
	    Printf("gs: %s\n", line);
	    *p = '\347';
	    linepos = line;
	    while (p < line + LINELEN) *linepos++ = *p++;
	}
}

/*
 *	For handling of SIGPIPE signals from send()
 */

static	Boolean	sigpipe_error = False;

/* ARGSUSED */
static	void
gs_sigpipe_handler(sig, code, scp, addr)
	int	sig;
	int	code;
	struct sigcontext *scp;
	char	*addr;
{
	sigpipe_error = True;
}

#ifdef	_POSIX_SOURCE
static	struct sigaction sigpipe_handler_struct;
	/* initialized to {gs_sigpipe_handler, (sigset_t) 0, 0} in initGS */
#endif

/*
 *	This actually sends the bytes to ghostscript.
 */

static	void
send(cp, len)
	_Xconst	char	*cp;
	int		len;
{
	int	bytes;
#ifdef	_POSIX_SOURCE
	struct sigaction orig;
#else
	void		(*orig)();
#endif
#ifdef	STREAMSCONN
	int	retval;
#endif

	if (GS_pid < 0) return;
#ifdef	_POSIX_SOURCE
	(void) sigaction(SIGPIPE, &sigpipe_handler_struct, &orig);
#else
	orig = signal(SIGPIPE, gs_sigpipe_handler);
#endif
	sigpipe_error = False;
	GS_sending = True;

#if	HAS_SIGIO
	(void) fcntl(ConnectionNumber(DISP), F_SETFL,
	    fcntl(ConnectionNumber(DISP), F_GETFL, 0) & ~FASYNC);
#endif

#ifndef	STREAMSCONN
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
#endif

	for (;;) {

#ifndef	STREAMSCONN
	    FD_SET(ConnectionNumber(DISP), &readfds);
	    FD_SET(GS_in, &writefds);
	    FD_SET(GS_out, &readfds);

	    if (select(numfds, &readfds, &writefds, (fd_set *) NULL,
		    (struct timeval *) NULL) < 0 && errno != EINTR) {
		perror("select (gs_send)");
		break;
	    }
#else	/* STREAMSCONN */
	    for (;;) {
		retval = poll(fds, XtNumber(fds), -1);
		if (retval >= 0 || errno != EAGAIN) break;
	    }
	    if (retval < 0) {
		perror("poll (gs_send)");
		break;
	    }
#endif	/* STREAMSCONN */

	    if (XDVI_ISSET(GS_out, &readfds, 1))
		read_from_gs();
	    if (XDVI_ISSET(GS_in, &writefds, 0)) {
		bytes = write(GS_in, cp, len);
		if (bytes == -1) {
		    if (!AGAIN_CONDITION) perror("gs_send");
		}
		else {
		    cp += bytes;
		    len -= bytes;
		    if (len == 0) break;
		}
		if (sigpipe_error) break;
	    }
	    if (XDVI_ISSET(ConnectionNumber(DISP), &readfds, 2)) {
#ifndef KDVI
		allow_can = False;
		read_events(False);
		allow_can = True;
		if (GS_pid < 0) break;	/* if timeout occurred */
#else
		qt_processEvents();
#endif /* KDVI */
	    }
	}

#if	HAS_SIGIO
	(void) fcntl(ConnectionNumber(DISP), F_SETFL,
	    fcntl(ConnectionNumber(DISP), F_GETFL, 0) | FASYNC);
#endif

	/* put back generic handler for SIGPIPE */
#ifdef	_POSIX_SOURCE
	(void) sigaction(SIGPIPE, &orig, (struct sigaction *) NULL);
#else
	(void) signal(SIGPIPE, orig);
#endif
	GS_sending = False;

	if (sigpipe_error) {
	    Fputs("ghostscript died unexpectedly.\n", stderr);
	    destroy_gs();
	    draw_bbox();
	}

	if (GS_pending_int) {
	    GS_pending_int = False;
	    interrupt_gs();
	}
}

/*
 *	Wait for acknowledgement from gs.
 */

static	void
waitack(waittime)
	int	waittime;
{
	struct timeval	tv;
	struct timeval	tv2;
#ifndef	STREAMSCONN
	struct timeval	*timeout = (struct timeval *) NULL;
#else
	int		timeout	= -1;
	int		retval;
#endif
#if	HAS_SIGIO
	int		oldflags;
#endif

	if (GS_pending == 0) return;
#if	HAS_SIGIO
	oldflags = fcntl(ConnectionNumber(DISP), F_GETFL, 0);
	(void) fcntl(ConnectionNumber(DISP), F_SETFL, oldflags & ~FASYNC);
#endif
	if (waittime != 0) {
	    (void) gettimeofday(&tv, (struct timezone *) NULL);
	    tv.tv_sec += waittime;
#ifndef	STREAMSCONN
	    timeout = &tv2;
#endif
	}
#ifndef	STREAMSCONN
	FD_ZERO(&readfds);
#endif
	while (GS_pending > 0) {
	    if (waittime != 0) {
		(void) gettimeofday(&tv2, (struct timezone *) NULL);
#ifndef	STREAMSCONN
		if (timercmp(&tv2, &tv, >=)) {
		    destroy_gs();
		    break;
		}
		tv2.tv_sec = tv.tv_sec - tv2.tv_sec;
		tv2.tv_usec = tv.tv_usec + 1000000 - tv2.tv_usec;
		if (tv2.tv_usec >= 1000000) tv2.tv_usec -= 1000000;
		else --tv2.tv_sec;
#else
		timeout = 1000 * (int) (tv.tv_sec - tv2.tv_sec)
		    + ((long) tv.tv_usec - (long) tv2.tv_usec) / 1000;
		if (timeout <= 0) {
		    destroy_gs();
		    break;
		}
#endif
	    }
#ifndef	STREAMSCONN
	    FD_SET(ConnectionNumber(DISP), &readfds);
	    FD_SET(GS_out, &readfds);
	    if (select(numfds, &readfds, (fd_set *) NULL, (fd_set *) NULL,
		    timeout) < 0 && errno != EINTR) {
		perror("select (gs_waitack)");
		break;
	    }
#else	/* STREAMSCONN */
	    for (;;) {
		retval = poll(fds + 1, XtNumber(fds) - 1, timeout);
		if (retval >= 0 || errno != EAGAIN) break;
	    }
	    if (retval < 0) {
		perror("poll (gs_waitack)");
		break;
	    }
#endif	/* STREAMSCONN */


	    if (XDVI_ISSET(GS_out, &readfds, 1))
		read_from_gs();
	    if (XDVI_ISSET(ConnectionNumber(DISP), &readfds, 2)) {
#ifndef KDVI
		allow_can = False;
		read_events(False);
		allow_can = True;
#else
		qt_processEvents();
#endif /* KDVI */
	    }
	}
#if	HAS_SIGIO
	(void) fcntl(ConnectionNumber(DISP), F_SETFL, oldflags);
#endif
	/* If you bail out here, change the call in interrupt_gs(). */
}

/*
 *	Fork a process to run ghostscript.  This is done using the
 *	x11 device (which needs to be compiled in).  Normally the x11
 *	device uses ClientMessage events to communicate with the calling
 *	program, but we don't do this.  The reason for using the ClientMessage
 *	events is that otherwise ghostview doesn't know when a non-conforming
 *	postscript program calls showpage.   That doesn't affect us here,
 *	since in fact we disable showpage.
 */

Boolean
initGS()
{
	char	buf[100];
		/*
		 * This string reads chunks (delimited by %%xdvimark).
		 * The first character of a chunk tells whether a given chunk
		 * is to be done within save/restore or not.
		 * The `H' at the end tells it that the first group is a
		 * header; i.e., no save/restore.
		 * `execute' is unique to ghostscript.
		 */
	static	_Xconst	char	str1[]	= "\
/xdvi$run {$error /newerror false put {currentfile cvx execute} stopped pop} def \
/xdvi$ack (\347\310\376) def \
/xdvi$dslen countdictstack def \
{currentfile read pop 72 eq \
    {xdvi$run} \
    {xdvi$run \
      clear countdictstack xdvi$dslen sub {end} repeat } \
  ifelse \
  {(%%xdvimark) currentfile =string {readline} stopped \
    {clear} {pop eq {exit} if} ifelse }loop \
  flushpage xdvi$ack print flush \
}loop\nH";
	static	_Xconst	char	str2[]	= "matrix currentmatrix \
dup dup 4 get round 4 exch put \
dup dup 5 get round 5 exch put setmatrix\n\
stop\n%%xdvimark\n";

#ifndef KDVI
	gs_atom = XInternAtom(DISP, "GHOSTVIEW", False);
	/* send bpixmap, orientation, bbox (in pixels), and h & v resolution */
	Sprintf(buf, "%ld %d 0 0 %u %u 72 72",
	    None,			/* bpixmap */ 
	    XtPageOrientationPortrait,
	    GS_page_w = page_w, GS_page_h = page_h);
	XChangeProperty(DISP, mane.win, gs_atom, XA_STRING, 8,
	    PropModeReplace, (unsigned char *) buf, strlen(buf));

	gs_colors_atom = XInternAtom(DISP, "GHOSTVIEW_COLORS", False);
	Sprintf(buf, "%s %ld %ld", "Color", fore_Pixel, back_Pixel);
	XChangeProperty(DISP, mane.win, gs_colors_atom, XA_STRING, 8,
	    PropModeReplace, (unsigned char *) buf, strlen(buf));

	XSync(DISP, False);		/* update the window */

	if (xpipe(std_in) != 0 || xpipe(std_out) != 0) {
	    perror("pipe");
	    return False;
	}
	Fflush(stderr);		/* to avoid double flushing */
	GS_pid = vfork();
	if (GS_pid == 0) {		/* child */
	    Sprintf(buf, "%ld", mane.win);
	    setenv("GHOSTVIEW", buf, True);
	    setenv("DISPLAY", XDisplayString(DISP), True);
	    (void) close(std_in[1]);
	    (void) dup2(std_in[0], 0);
	    (void) close(std_in[0]);
	    (void) close(std_out[0]);
	    (void) dup2(std_out[1], 1);
	    (void) dup2(std_out[1], 2);
	    (void) close(std_out[1]);
	    (void) execvp(argv[0], argv);
	    Fprintf(stderr, "Execvp of %s failed.\n", argv[0]);
	    Fflush(stderr);
	    _exit(1);
	}
#else /* KDVI */
	extern int useAlpha;
	extern Window mainwin;

	gs_atom = XInternAtom(DISP, "GHOSTVIEW", False);
	/* send bpixmap, orientation, bbox (in pixels), and h & v resolution */
	Sprintf(buf, "%ld %d 0 0 %u %u 72 72",
	    None,			/* bpixmap */ 
	    XtPageOrientationPortrait,
	    GS_page_w = page_w, GS_page_h = page_h);
	XChangeProperty(DISP, mainwin, gs_atom, XA_STRING, 8,
	    PropModeReplace, (unsigned char *) buf, strlen(buf));

	gs_colors_atom = XInternAtom(DISP, "GHOSTVIEW_COLORS", False);
	Sprintf(buf, "%s %ld %ld", "Color", fore_Pixel, back_Pixel);
	XChangeProperty(DISP, mainwin, gs_colors_atom, XA_STRING, 8,
	    PropModeReplace, (unsigned char *) buf, strlen(buf));

	XSync(DISP, False);		/* update the window */

	if (xpipe(std_in) != 0 || xpipe(std_out) != 0) {
	    perror("pipe");
	    return False;
	}
	Fflush(stderr);		/* to avoid double flushing */
	GS_pid = vfork();
	if (GS_pid == 0) {		/* child */
	    Sprintf(buf, "%ld %ld", mainwin, mane.win);
	    setenv("GHOSTVIEW", buf, True);
	    setenv("DISPLAY", XDisplayString(DISP), True);
	    (void) close(std_in[1]);
	    (void) dup2(std_in[0], 0);
	    (void) close(std_in[0]);
	    (void) close(std_out[0]);
	    (void) dup2(std_out[1], 1);
	    (void) dup2(std_out[1], 2);
	    (void) close(std_out[1]);
	    argv[1] = useAlpha ? "-sDEVICE=x11alpha" : "-sDEVICE=x11";
	    (void) execvp(argv[0], argv);
	    Fprintf(stderr, "Execvp of %s failed.\n", argv[0]);
	    Fflush(stderr);
	    _exit(1);
	}
#endif /* KDVI */
	if (GS_pid == -1) {	/* error */
	    perror("vfork");
	    return False;
	}
	(void) close(std_in[0]);
	(void) close(std_out[1]);

	/* Set std_in for non-blocking I/O */
	(void) fcntl(std_in[1], F_SETFL,
	    fcntl(std_in[1], F_GETFL, 0) | O_NONBLOCK);

#ifdef	_POSIX_SOURCE
	sigpipe_handler_struct.sa_handler = gs_sigpipe_handler;
	sigemptyset(&sigpipe_handler_struct.sa_mask);
#endif

#ifndef	STREAMSCONN
	numfds = ConnectionNumber(DISP);
	if (numfds < std_in[1]) numfds = std_in[1];
	if (numfds < std_out[0]) numfds = std_out[0];
	++numfds;
#else	/* STREAMSCONN */
	fds[0].fd = std_in[1];
	fds[1].fd = std_out[0];
	fds[2].fd = ConnectionNumber(DISP);
#endif	/* STREAMSCONN */

	psp = gs_procs;
	GS_active = GS_sending = GS_pending_int = False;
	GS_pending = 1;
	GS_mag = GS_shrink = -1;

	send(str1, sizeof(str1) - 1);
	send(psheader, psheaderlen);
	Sprintf(buf, "[1 0 0 -1 0 %d] concat\n", page_h);
	send(buf, strlen(buf));
	send(str2, sizeof(str2) - 1);
	waitack(0);

	if (GS_pid < 0) {		/* if something happened */
	    destroy_gs();
	    return False;
	}
	if (!postscript) toggle_gs();	/* if we got a 'v' already */
	else {
#ifndef KDVI
	    canit = True;		/* ||| redraw the page */
	    longjmp(canit_env, 1);
#endif /* KDVI */
	}
	return True;
}

static	void
toggle_gs()
{
	if (debug & DBG_PS) Puts("Toggling GS on or off");
	if (postscript) psp.drawbegin = drawbegin_gs;
	else {
	    interrupt_gs();
	    psp.drawbegin = drawbegin_none;
	}
	    
}

static	void
destroy_gs()
{
	if (debug & DBG_PS) Puts("Destroying GS process");
	if (linepos > line) {
	    *linepos = '\0';
	    Printf("gs: %s\n", line);
	    linepos = line;
	}
	if (GS_pid >= 0) {
	    if (kill(GS_pid, SIGKILL) < 0 && errno != ESRCH)
		perror("destroy_gs");
	    GS_pid = -1;
	}
	(void) close(GS_in);
	(void) close(GS_out);
	GS_active = GS_sending = GS_pending_int = False;
	GS_pending = 0;
}

static	void
interrupt_gs()
{
	static	_Xconst	char	str[]	= " stop\n%%xdvimark\n";

	if (debug & DBG_PS) Puts("Running interrupt_gs()");
	if (GS_sending) GS_pending_int = True;
	else {
	    if (GS_active) {
		/*
		 * ||| what I'd really like to do here is cause gs to execute
		 * the interrupt routine in errordict.  But so far (gs 2.6.1)
		 * that has not been implemented in ghostscript.
		 */
		send(str, sizeof(str) - 1);
		GS_active = False;
	    }
	    psp.interrupt = NullProc;	/* prevent deep recursion in waitack */
	    waitack(5);
	    psp.interrupt = interrupt_gs;
	}
}

static	void
endpage_gs()
{
	static	_Xconst	char	str[]	= "stop\n%%xdvimark\n";

	if (debug & DBG_PS) Puts("Running endpage_gs()");
	if (GS_active) {
	    send(str, sizeof(str) - 1);
	    GS_active = False;
	    waitack(0);
	}
}

static	void
drawbegin_gs(xul, yul, cp)
	int	xul, yul;
	char	*cp;
{
	char	buf[100];
	static	_Xconst	char	str[]	= " TeXDict begin\n";

	/* check page_w and page_h to see that they haven't increased */
	if (page_w > GS_page_w || page_h > GS_page_h) {
	    /* It would be nice if we could just resize the window, but I
	     * don't see a convenient way to do that. */
	    destroy_gs();
	}

	if (GS_pid < 0)
	    (void) initGS();

	if (!GS_active) {
	    if (magnification != GS_mag) {
		Sprintf(buf, "H TeXDict begin /DVImag %d 1000 div def \
end stop\n%%%%xdvimark\n",
		    GS_mag = magnification);
		send(buf, strlen(buf));
		++GS_pending;
	    }
	    if (mane.shrinkfactor != GS_shrink) {
		Sprintf(buf,
		    "H TeXDict begin %d %d div dup \
/Resolution X /VResolution X \
end stop\n%%%%xdvimark\n",
		    pixels_per_inch, GS_shrink = mane.shrinkfactor);
		send(buf, strlen(buf));
		++GS_pending;
	    }
	    send(str, sizeof(str) - 1);
	    GS_active = True;
	    ++GS_pending;
	}

	/* This allows the X side to clear the page */
	XSync(DISP, False);

	Sprintf(buf, "%d %d moveto\n", xul, yul);
	send(buf, strlen(buf));
	if (debug & DBG_PS)
	    Printf("drawbegin at %d,%d:  sending `%s'\n", xul, yul, cp);
	send(cp, strlen(cp));
}

static	void
drawraw_gs(cp)
	char	*cp;
{
	int	len	= strlen(cp);

	if (!GS_active)
	    return;
	if (debug & DBG_PS) Printf("raw ps sent to context: %s\n", cp);
	cp[len] = '\n';
	send(cp, len + 1);
}

static	void
drawfile_gs(cp)
	char	*cp;
{
	char	buf[PATH_MAX + 7];

	if (!GS_active)
	    return;
	if (debug & DBG_PS) Printf("printing file %s\n", cp);
	Sprintf(buf, "(%s)run\n", cp);
	send(buf, strlen(buf));
}

static	void
drawend_gs(cp)
	char	*cp;
{
	if (!GS_active)
	    return;
	if (debug & DBG_PS) Printf("end ps: %s\n", cp);
	send(cp, strlen(cp));
	send("\n", 1);
}

#endif /* PS_GS */
