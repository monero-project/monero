# dnscrypt.m4

# dnsc_DNSCRYPT([action-if-true], [action-if-false])
# --------------------------------------------------------------------------
# Check for required dnscrypt libraries and add dnscrypt configure args.
AC_DEFUN([dnsc_DNSCRYPT],
[
  AC_ARG_ENABLE([dnscrypt],
    AS_HELP_STRING([--enable-dnscrypt],
                   [Enable dnscrypt support (requires libsodium)]),
    [opt_dnscrypt=$enableval], [opt_dnscrypt=no])

  if test "x$opt_dnscrypt" != "xno"; then
    AC_ARG_WITH([libsodium], AC_HELP_STRING([--with-libsodium=path],
    	[Path where libsodium is installed, for dnscrypt]), [
	CFLAGS="$CFLAGS -I$withval/include"
	LDFLAGS="$LDFLAGS -L$withval/lib"
    ])
    AC_SEARCH_LIBS([sodium_init], [sodium], [],
      AC_MSG_ERROR([The sodium library was not found. Please install sodium!]))
    $1
  else
    $2
  fi
])
