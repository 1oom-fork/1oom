AC_INIT(moo1_patcher, 0.1)
AM_INIT_AUTOMAKE([-Wall foreign])
AM_SILENT_RULES

user_cflags=$CFLAGS

AC_PROG_CC

if test -z "$user_cflags" ; then
    dnl Check for the type of compiler first.
    if test x"$GCC" = "xyes" ; then
        warnings_c="-Wall -Wextra -std=c99 -pedantic -Wstrict-prototypes -Wno-unused-parameter"
	CFLAGS="$warnings_c"
    fi
    dnl (If not on GCC, just keep the defaults, which are very conservative).
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT


