dnl $Id: xorg-macros.m4,v 1.8 2005/12/09 15:28:41 kem Exp $
dnl
dnl Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
dnl 
dnl Permission to use, copy, modify, distribute, and sell this software and its
dnl documentation for any purpose is hereby granted without fee, provided that
dnl the above copyright notice appear in all copies and that both that
dnl copyright notice and this permission notice appear in supporting
dnl documentation.
dnl 
dnl The above copyright notice and this permission notice shall be included
dnl in all copies or substantial portions of the Software.
dnl 
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
dnl OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
dnl IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
dnl OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
dnl ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
dnl OTHER DEALINGS IN THE SOFTWARE.
dnl 
dnl Except as contained in this notice, the name of the copyright holders shall
dnl not be used in advertising or otherwise to promote the sale, use or
dnl other dealings in this Software without prior written authorization
dnl from the copyright holders.
dnl 

# XORG_PROG_RAWCPP()
# ------------------
# Find cpp program and necessary flags for use in pre-processing text files
# such as man pages and config files
AC_DEFUN([XORG_PROG_RAWCPP],[
AC_REQUIRE([AC_PROG_CPP])
AC_PATH_PROGS(RAWCPP, [cpp], [${CPP}], 
   [$PATH:/bin:/usr/bin:/usr/lib:/usr/libexec:/usr/ccs/lib:/usr/ccs/lbin:/lib])

# Check for flag to avoid builtin definitions - assumes unix is predefined,
# which is not the best choice for supporting other OS'es, but covers most
# of the ones we need for now.
AC_MSG_CHECKING([if $RAWCPP requires -undef])
AC_LANG_CONFTEST([Does cpp redefine unix ?])
if test `${RAWCPP} < conftest.$ac_ext | grep -c 'unix'` -eq 1 ; then
	AC_MSG_RESULT([no])
else
	if test `${RAWCPP} -undef < conftest.$ac_ext | grep -c 'unix'` -eq 1 ; then
		RAWCPPFLAGS=-undef
		AC_MSG_RESULT([yes])
	else
		AC_MSG_ERROR([${RAWCPP} defines unix with or without -undef.  I don't know what to do.])
	fi
fi
rm -f conftest.$ac_ext

AC_MSG_CHECKING([if $RAWCPP requires -traditional])
AC_LANG_CONFTEST([Does cpp preserve   "whitespace"?])
if test `${RAWCPP} < conftest.$ac_ext | grep -c 'preserve   \"'` -eq 1 ; then
	AC_MSG_RESULT([no])
else
	if test `${RAWCPP} -traditional < conftest.$ac_ext | grep -c 'preserve   \"'` -eq 1 ; then
		RAWCPPFLAGS="${RAWCPPFLAGS} -traditional"
		AC_MSG_RESULT([yes])
	else
		AC_MSG_ERROR([${RAWCPP} does not preserve whitespace with or without -traditional.  I don't know what to do.])
	fi
fi
rm -f conftest.$ac_ext
AC_SUBST(RAWCPPFLAGS)
]) # XORG_PROG_RAWCPP

# XORG_MANPAGE_SECTIONS()
# -----------------------
# Determine which sections man pages go in for the different man page types
# on this OS - replaces *ManSuffix settings in old Imake *.cf per-os files.
# Not sure if there's any better way than just hardcoding by OS name.
# Override default settings by setting environment variables

AC_DEFUN([XORG_MANPAGE_SECTIONS],[
AC_REQUIRE([AC_CANONICAL_HOST])

if test x$APP_MAN_SUFFIX = x    ; then
    case $host_os in
	linux*)	APP_MAN_SUFFIX=1x ;;
	*)	APP_MAN_SUFFIX=1  ;;
    esac
fi
if test x$APP_MAN_DIR = x    ; then
    case $host_os in
	linux*)	APP_MAN_DIR='$(mandir)/man1' ;;
	*)	APP_MAN_DIR='$(mandir)/man$(APP_MAN_SUFFIX)' ;;
    esac
fi

if test x$LIB_MAN_SUFFIX = x    ; then
    case $host_os in
	linux*)	LIB_MAN_SUFFIX=3x ;;
	*)	LIB_MAN_SUFFIX=3  ;;
    esac
fi
if test x$LIB_MAN_DIR = x    ; then
    case $host_os in
	linux*)	LIB_MAN_DIR='$(mandir)/man3' ;;
	*)	LIB_MAN_DIR='$(mandir)/man$(LIB_MAN_SUFFIX)' ;;
    esac
fi

if test x$FILE_MAN_SUFFIX = x    ; then
    case $host_os in
	linux*)		FILE_MAN_SUFFIX=5x ;;
	solaris*)	FILE_MAN_SUFFIX=4  ;;
	*)		FILE_MAN_SUFFIX=5  ;;
    esac
fi
if test x$FILE_MAN_DIR = x    ; then
    case $host_os in
	linux*)	FILE_MAN_DIR='$(mandir)/man5' ;;
	*)	FILE_MAN_DIR='$(mandir)/man$(FILE_MAN_SUFFIX)' ;;
    esac
fi

# In Imake's linux.cf, the misc man suffix & dir was only changed for 
# LinuxDebian, not other Linuxes, so we leave it unchanged here
if test x$MISC_MAN_SUFFIX = x    ; then
    case $host_os in
#	linux*)		MISC_MAN_SUFFIX=7x ;;
	solaris*)	MISC_MAN_SUFFIX=5  ;;
	*)		MISC_MAN_SUFFIX=7  ;;
    esac
fi
if test x$MISC_MAN_DIR = x    ; then
    case $host_os in
#	linux*)	MISC_MAN_DIR='$(mandir)/man7' ;;
	*)	MISC_MAN_DIR='$(mandir)/man$(MISC_MAN_SUFFIX)' ;;
    esac
fi

# In Imake's linux.cf, the driver man suffix & dir was only changed for 
# LinuxDebian, not other Linuxes, so we leave it unchanged here
if test x$DRIVER_MAN_SUFFIX = x    ; then
    case $host_os in
#	linux*)		DRIVER_MAN_SUFFIX=4x ;;
	solaris*)	DRIVER_MAN_SUFFIX=7  ;;
	*)		DRIVER_MAN_SUFFIX=4  ;;
    esac
fi
if test x$DRIVER_MAN_DIR = x    ; then
    case $host_os in
#	linux*)	DRIVER_MAN_DIR='$(mandir)/man4' ;;
	*)	DRIVER_MAN_DIR='$(mandir)/man$(DRIVER_MAN_SUFFIX)' ;;
    esac
fi

if test x$ADMIN_MAN_SUFFIX = x    ; then
    case $host_os in
	solaris*)	ADMIN_MAN_SUFFIX=1m ;;
	*)		ADMIN_MAN_SUFFIX=8  ;;
    esac
fi
if test x$ADMIN_MAN_DIR = x    ; then
    ADMIN_MAN_DIR='$(mandir)/man$(ADMIN_MAN_SUFFIX)'
fi


AC_SUBST([APP_MAN_SUFFIX])
AC_SUBST([LIB_MAN_SUFFIX])
AC_SUBST([FILE_MAN_SUFFIX])
AC_SUBST([MISC_MAN_SUFFIX])
AC_SUBST([DRIVER_MAN_SUFFIX])
AC_SUBST([ADMIN_MAN_SUFFIX])
AC_SUBST([APP_MAN_DIR])
AC_SUBST([LIB_MAN_DIR])
AC_SUBST([FILE_MAN_DIR])
AC_SUBST([MISC_MAN_DIR])
AC_SUBST([DRIVER_MAN_DIR])
AC_SUBST([ADMIN_MAN_DIR])
]) # XORG_MANPAGE_SECTIONS

# XORG_CHECK_LINUXDOC
# -------------------
# Defines the variable MAKE_TEXT if the necessary tools and
# files are found. $(MAKE_TEXT) blah.sgml will then produce blah.txt.
# Whether or not the necessary tools and files are found can be checked
# with the AM_CONDITIONAL "BUILD_LINUXDOC"
AC_DEFUN([XORG_CHECK_LINUXDOC],[
AC_CHECK_FILE(
	[$prefix/share/X11/sgml/defs.ent], 
	[DEFS_ENT_PATH=$prefix/share/X11/sgml],
	[DEFS_ENT_PATH=]
)

AC_PATH_PROG(LINUXDOC, linuxdoc)
AC_PATH_PROG(PS2PDF, ps2pdf)

AC_MSG_CHECKING([Whether to build documentation])

if test x$DEFS_ENT_PATH != x && test x$LINUXDOC != x ; then
   BUILDDOC=yes
else
   BUILDDOC=no
fi

AM_CONDITIONAL(BUILD_LINUXDOC, [test x$BUILDDOC = xyes])

AC_MSG_RESULT([$BUILDDOC])

AC_MSG_CHECKING([Whether to build pdf documentation])

if test x$PS2PDF != x ; then
   BUILDPDFDOC=yes
else
   BUILDPDFDOC=no
fi

AM_CONDITIONAL(BUILD_PDFDOC, [test x$BUILDPDFDOC = xyes])

AC_MSG_RESULT([$BUILDPDFDOC])

MAKE_TEXT="SGML_SEARCH_PATH=$DEFS_ENT_PATH GROFF_NO_SGR=y $LINUXDOC -B txt"
MAKE_PS="SGML_SEARCH_PATH=$DEFS_ENT_PATH $LINUXDOC -B latex --papersize=letter --output=ps"
MAKE_PDF="$PS2PDF"
MAKE_HTML="SGML_SEARCH_PATH=$DEFS_ENT_PATH $LINUXDOC  -B html --split=0"

AC_SUBST(MAKE_TEXT)
AC_SUBST(MAKE_PS)
AC_SUBST(MAKE_PDF)
AC_SUBST(MAKE_HTML)
]) # XORG_CHECK_LINUXDOC

# XORG_CHECK_MALLOC_ZERO
# ----------------------
# Defines {MALLOC,XMALLOC,XTMALLOC}_ZERO_CFLAGS appropriately if
# malloc(0) returns NULL.  Packages should add one of these cflags to
# their AM_CFLAGS (or other appropriate *_CFLAGS) to use them.
AC_DEFUN([XORG_CHECK_MALLOC_ZERO],[
AC_ARG_ENABLE(malloc0returnsnull,
	AC_HELP_STRING([--enable-malloc0returnsnull],
		       [malloc(0) returns NULL (default: auto)]),
	[MALLOC_ZERO_RETURNS_NULL=$enableval],
	[MALLOC_ZERO_RETURNS_NULL=auto])

AC_MSG_CHECKING([whether malloc(0) returns NULL])
if test "x$MALLOC_ZERO_RETURNS_NULL" = xauto; then
	AC_RUN_IFELSE([
char *malloc();
char *realloc();
char *calloc();
main() {
    char *m0, *r0, *c0, *p;
    m0 = malloc(0);
    p = malloc(10);
    r0 = realloc(p,0);
    c0 = calloc(0);
    exit(m0 == 0 || r0 == 0 || c0 == 0 ? 0 : 1);
}],
		[MALLOC_ZERO_RETURNS_NULL=yes],
		[MALLOC_ZERO_RETURNS_NULL=no])
fi
AC_MSG_RESULT([$MALLOC_ZERO_RETURNS_NULL])

if test "x$MALLOC_ZERO_RETURNS_NULL" = xyes; then
	MALLOC_ZERO_CFLAGS="-DMALLOC_0_RETURNS_NULL"
	XMALLOC_ZERO_CFLAGS=$MALLOC_ZERO_CFLAGS
	XTMALLOC_ZERO_CFLAGS="$MALLOC_ZERO_CFLAGS -DXTMALLOC_BC"
else
	MALLOC_ZERO_CFLAGS=""
	XMALLOC_ZERO_CFLAGS=""
	XTMALLOC_ZERO_CFLAGS=""
fi

AC_SUBST([MALLOC_ZERO_CFLAGS])
AC_SUBST([XMALLOC_ZERO_CFLAGS])
AC_SUBST([XTMALLOC_ZERO_CFLAGS])
]) # XORG_CHECK_MALLOC_ZERO
